#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "pins_arduino.h"
#include "WConstants.h"
#endif


#include "data_logger.h"
#include "junk.h"
#include "str_util.h"

char char_buf[256];   //TEMP!!!  global for now: used by str_util.cpp

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
static RTC_DS1307 rtc; // define the Real Time Clock object
static const int logger_cs_pin = 10;
static const int bad_led_pin = 3;
static const int good_led_pin = 4;
static File _download_file;

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
  _triggered_events = 0;
  for (uint8_t i = 0; i < _num_events; i++) {
    const uint32_t candidate = next_time_for_event(&_schedule[i]);
    if (candidate < _next_time) {
      _next_time = candidate;
      _triggered_events = (1 << i);
    } else if (candidate == _next_time) {
      _triggered_events |= (1 << i);
    }
  }
}





static void output_any_line()
{
  if (!empty()) {
    const char *line = get_row();

    if (_state == FILE_LOG && log_file) {
      log_file.println(line);
      log_file.flush();
    } else if (_state == TERM_LOG) {
      Serial.println(line);
    }

  }
}


static void send_file(const char *filename)
{
  _download_file = SD.open(filename, FILE_READ);
  if (!_download_file) {
    return;
  }

  Serial.println(_download_file.size());
  while(true) {
    if (Serial.available()) {
      Serial.read();
      Serial.println("XXX");
      break;
    }
    if (not _download_file.available()) {
      break;
    }
    const uint8_t bytes_read = _download_file.readBytes(char_buf, 128);
    Serial.write(char_buf, bytes_read);
  }
  _download_file.close();
}


static const char *read_serial_string_to_buf(void)
{
  const int bytes_read = Serial.readBytesUntil('|', char_buf, 128 - 1);
  char_buf[bytes_read] = '\0';
  return char_buf;
}


static void process_commands(void)
{

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
    }
    break;

  case 'G':
    {
      const char *filename = read_serial_string_to_buf();
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
      const char *filename = read_serial_string_to_buf();
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
      rtc.adjust(DateTime(unix_time));
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
    break;

  default:
    Serial.print("?");
    Serial.print(command);
    Serial.println("?");



  }
  Serial.print(">");
}

DataLogger::DataLogger()
{

}


void DataLogger::setup(void)
{

  pinMode(good_led_pin, OUTPUT);
  pinMode(bad_led_pin, OUTPUT);

  Serial.begin(230400);
  Serial.println("Coweeta Hydrologic Lab Datalogger");


  // connect to RTC
  Wire.begin();

  if (!rtc.begin()) {
    junk::die("RTC failed");
  }

  pinMode(logger_cs_pin, OUTPUT);
  if (!SD.begin(logger_cs_pin, 11, 12, 13)) {
    junk::die("Data logger card failed, or not present.");
  }

  file_number = junk::get_next_file_number();
  log_file = junk::set_log_file(file_number, FILE_WRITE);
  log_file.println("# Coweeta log file");
  log_file.flush();

  digitalWrite(bad_led_pin, HIGH);
  for (uint8_t i = 0; i < 8; i++) {
    delay(50);
    digitalWrite(good_led_pin, HIGH);
    digitalWrite(bad_led_pin, LOW);
    delay(50);
    digitalWrite(good_led_pin, LOW);
    digitalWrite(bad_led_pin, HIGH);
  }
  digitalWrite(bad_led_pin, LOW);

}


void DataLogger::set_schedule(const EventSchedule *schedule, uint8_t num_events)
{
  _schedule = schedule;
  _num_events = num_events;
}


void DataLogger::wait_for_event(void)
{
  _now = junk::current_time();
  digitalWrite(good_led_pin, LOW);
  _state = WAITING;
  compute_next_time();
  while (_state == WAITING) {
    _now = junk::current_time();
    if (_now == _next_time) {
      _state = FILE_LOG;
    } else if (Serial.available()) {
      process_commands();
    } else {
      delay(100);
    }
  }
  digitalWrite(good_led_pin, HIGH);
}


bool DataLogger::is_event(uint16_t event)
{
  return (event & _triggered_events) != 0;
}


// This is used inside events.
//
//
// void DataLogger::wait_for_division(int division)
// {
//     _state = WAITING;
//     compute_next_time();
//     while (_state == WAITING) {
//         _now = junk::current_time();
//         if (_now == _next_time) {
//             _state = FILE_LOG;
//         } else if (Serial.available()) {
//             process_commands();
//         } else {
//             delay(100);
//         }
//     }
//
// }


void DataLogger::new_log_line(void)
{
  junk::new_log_row(_now);
}


void DataLogger::log_string(const char *string)
{
  write_char(',');
  for (const char *ch = string; *ch != '\0'; ch++) {
    write_char(*ch);
  }
}


void DataLogger::log_int(int value)
{
  write_char(',');
  write_sint(value);
}


void DataLogger::log_float(double value, uint8_t dec_places)
{
  write_char(',');
  if (value < 0.0) {
    write_char('-');
    value = -value;
  }
  const int int_part = (int)value;
  double dec_part = value - int_part;
  write_sint(int_part);
  write_char('.');
  for (uint8_t i = 0; i < dec_places; i++) {
    dec_part = dec_part * 10.0;
    const uint8_t digit = (int)dec_part;
    dec_part -= digit;
    write_uint(digit);
  }
}


void DataLogger::skip_entries(uint8_t count)
{
  for (uint8_t i = 0; i < count; i++) {
    write_char(',');
  }

}


void DataLogger::end_log_line(void)
{
  output_any_line();
}


void DataLogger::get_time(int *hour, int *minute, int *second)
{

}



}  // namespace coweeta