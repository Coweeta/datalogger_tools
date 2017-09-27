#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
  #include "WConstants.h"
#endif


#include "data_logger.h"
#include "char_stream.h"

#include "junk.h"

static const int BUF_SIZE = 250;
char char_buf[BUF_SIZE];
CharStream char_stream(char_buf, BUF_SIZE);

SdFat SD;  // The SD initialization

namespace coweeta {


enum {WAITING, FILE_LOG, TERM_LOG};


static const EventSchedule* _schedule;
static uint8_t _num_events;
static uint32_t _next_time;
static uint32_t _now;
static bool _to_file;
static uint16_t _triggered_events;
static uint8_t _state;
static File log_file;
static int file_number = 0;
static File _download_file;
static bool _show_prompt;
static uint16_t _event_enabled = 0xFFFF;

static DataLogger *_logger;

static uint32_t next_time_for_event(const EventSchedule* schedule)
{
  const int16_t interval = schedule->interval;
  const int16_t offset = schedule->offset;
  const int32_t next = (((_now - offset) / interval) + 1) * interval + offset;
  return next;
}


static uint32_t compute_next_time(void)
{
  _next_time = 0xFFFFFFFF;
  _triggered_events = 0x0000;
  uint16_t mask = 0x0001;
  for (uint8_t i = 0; i < _num_events; i++, mask <<= 1) {
    if ((_event_enabled & mask) != 0) {
      const uint32_t candidate = next_time_for_event(&_schedule[i]);
      if (candidate < _next_time) {
        _next_time = candidate;
        _triggered_events = mask;
      } else if (candidate == _next_time) {
        _triggered_events |= mask;
      }
    }
  }
}


static void send_file(const char *filename)
{
  _download_file = SD.open(filename, FILE_READ);
  if (!_download_file) {
    Serial.print(filename);
    junk::die("Can't open file");
    return;
  }

  Serial.println(_download_file.size());
  while(true) {
    if (Serial.available()) {
      Serial.read();
      Serial.println("XXX");     //TEMP!!!
      break;
    }
    if (not _download_file.available()) {
      break;
    }
    const uint8_t bytes_read = _download_file.readBytes(char_buf, BUF_SIZE);
    Serial.write(char_buf, bytes_read);
  }
  _download_file.close();
}


static void process_commands(void)
{
  _show_prompt = true; // by default
  const int command = Serial.read();
  switch(command) {
  case 'v':
    {
      // protocol version
      Serial.println("COW0.0");
    }
    break;
  case 'e':
    {
      // trigger_event()
      _triggered_events = junk::get_int(0, (1 << _num_events) - 1);
      _state = TERM_LOG;
      _show_prompt = false;
    }
    break;

  case 'G':
    {
      const size_t SIZE = 30;
      char filename[SIZE];
      const int bytes_read = Serial.readBytesUntil('|', filename, SIZE - 1);
      filename[bytes_read] = '\0';
      send_file(filename);
    }
    break;

  case 'A':
    // get_active_file_num()
    Serial.println(file_number, DEC);
    break;

  case 'R':
    // remove as part of download_file()
    {
      const size_t SIZE = 30;
      char filename[SIZE];
      const int bytes_read = Serial.readBytesUntil('|', filename, SIZE - 1);
      filename[bytes_read] = '\0';
      SD.remove(filename);
      Serial.println();
    }
    break;
    // log_file.close();

  case 'N':
    {
      // new_active_file() switch to new file
      const int file_num = junk::get_int(0, 99);
      log_file.close();
      log_file = junk::set_log_file(file_num, FILE_WRITE);
      log_file.println("# Coweeta log file");
      log_file.flush();

      Serial.println();
    }
    break;

  case 's':
    {
      // set time for sync_time()
      const uint32_t unix_time = Serial.parseInt();
      Serial.println(unix_time);
      _logger->set_unix_time(unix_time);
      compute_next_time();
      Serial.println();
    }
    break;

  case 't':
    {
      // get time for sync_time()
      Serial.println(_now, DEC);
    }
    break;

  case 'L':
    // list_files()
    junk::print_root_directory();
    break;

  case 'n':
    // list event names
    for (uint8_t i = 0; i < _num_events; i++) {
      Serial.println(_schedule[i].name);
    }
    break;

  case 'w':
    // report wait for next event
    Serial.println(_next_time - _now);
    Serial.println(_triggered_events);
    Serial.println(_event_enabled);
    break;

  case 'E':
    // enable schedule for events
    _event_enabled = junk::get_int(0, (1 << _num_events) - 1);
    compute_next_time();
    break;

  default:
    Serial.print("?");
    Serial.print(command);
    Serial.println("?");

  }
}


 //TEMP!!!
DataLogger::DataLogger()
{
  _logger = this;
}


void DataLogger::setup()
{

  pinMode(good_led_pin_, OUTPUT);
  pinMode(bad_led_pin_, OUTPUT);
  if (beeper_pin_) {
    pinMode(beeper_pin_, OUTPUT);
  }

  Serial.begin(57600);  //TODO make adjustable
  Serial.println("Coweeta Hydrologic Lab Datalogger");

  _show_prompt = true;

  // connect to RTC
  Wire.begin();

  if (!rtc.begin()) {
    junk::die("RTC failed");
  }

  if (!SD.begin(logger_cs_pin_)) {
    junk::die("Data logger card failed, or not present.");
  }

  file_number = junk::get_next_file_number();

  log_file = junk::set_log_file(file_number, FILE_WRITE);
  log_file.println("# Coweeta log file");   //TODO make variable and delay file write.
  log_file.flush();

  digitalWrite(bad_led_pin_, HIGH);
  for (uint8_t i = 0; i < 8; i++) {
    delay(50);
    digitalWrite(good_led_pin_, HIGH);
    digitalWrite(bad_led_pin_, LOW);
    delay(50);
    digitalWrite(good_led_pin_, LOW);
    digitalWrite(bad_led_pin_, HIGH);
  }
  digitalWrite(bad_led_pin_, LOW);

  if (beeper_pin_) {
    for (uint8_t i = 0; i < 100; i++) {
      delay(1);
      digitalWrite(beeper_pin_, HIGH);
      delay(1);
      digitalWrite(beeper_pin_, LOW);
    }
  }
}


void DataLogger::set_schedule(const EventSchedule *schedule, uint8_t num_events)
{
  _schedule = schedule;
  _num_events = num_events;
  for (uint8_t i = 0; i < num_events; i++) {
    if (schedule[i].category == Disabled) {
      _event_enabled &= ~(1 << i);
    }
  }
}


void DataLogger::wait_for_event(void)
{

  _now = _logger->get_unix_time();
  digitalWrite(good_led_pin_, LOW);
  _state = WAITING;
  compute_next_time();
  while (_state == WAITING) {
    _now = _logger->get_unix_time();
    if (_show_prompt) {
      Serial.print(">");
      _show_prompt = false;
    }
    if (_now == _next_time) {
      _state = FILE_LOG;
    } else if (Serial.available()) {
      process_commands();
    } else {
      wait_a_while();
    }
  }
  digitalWrite(good_led_pin_, HIGH);
}


bool DataLogger::is_event(uint16_t event)
{
  return (event & _triggered_events) != 0;
}


void DataLogger::new_log_line(void)
{
  char_stream.reset();
  write_timestamp(char_stream);
}


void DataLogger::log_string(const char *string)
{
  char_stream.print(',');
  char_stream.print(string);
}


void DataLogger::log_int(int value)
{
  char_stream.print(',');
  char_stream.print(value);
}


void DataLogger::log_float(double value, uint8_t dec_places)
{
  char_stream.print(',');
  char_stream.print(value, dec_places);
}


void DataLogger::skip_entries(uint8_t count)
{
  for (uint8_t i = 0; i < count; i++) {
    char_stream.print(',');
  }

}


void DataLogger::end_log_line(void)
{
  if (char_stream.bytes_written()) {

    if (_state == FILE_LOG && log_file) {
      char_stream.dump(log_file);
      log_file.println("");
      log_file.flush();
    } else if (_state == TERM_LOG) {
      char_stream.dump(Serial);
      Serial.println("");
      _show_prompt = true;
    }

  }

}


void DataLogger::get_time(int *hour, int *minute, int *second)
{

}

void DataLogger::enable_events(uint16_t events)
{
  _event_enabled |= events;
}

void DataLogger::disable_events(uint16_t events)
{
  _event_enabled &= ~events;
}


}  // namespace coweeta