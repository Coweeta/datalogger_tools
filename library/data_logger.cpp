#include "Arduino.h"
#include <Sodaq_PcInt.h>

#include "data_logger.h"
#include "char_stream.h"
#include "command_parser.h"
#include "junk.h"
#include "file_transfer.h"

SdFat sd_card_;  // The SD initialization

namespace coweeta {



static const EventSchedule* _schedule;
static uint8_t _num_events;
static uint32_t _next_time;
static uint32_t _now;
static bool _to_file;
static uint16_t _triggered_events;
static uint16_t _forced_events;
static File log_file;
static int file_number = 0;
static File _download_file;
static uint16_t _event_enabled = 0xFFFF;

static uint8_t good_led_pin_ = 0;
static uint8_t bad_led_pin_ = 0;
static uint8_t beeper_pin_ = 0;
static uint8_t button_pin_ = 0;

static uint8_t logger_cs_pin_ = 0;
static uint32_t usb_usart_baud_rate_ = 0;

static bool log_to_file_;
static bool log_to_term_;

static const int BUF_SIZE = 250;
char char_buf[BUF_SIZE];
CharStream char_stream(char_buf, BUF_SIZE);

static uint8_t sd_card_state_;

static DataLogger *_logger;



static uint32_t next_time_for_event(const EventSchedule* schedule)
{
  const int16_t interval = schedule->interval;
  const int16_t offset = schedule->offset;
  const int32_t next = (((_now - offset) / interval) + 1) * interval + offset;
  return next;
}


static void compute_next_time()
{
  if (_forced_events) {
      return;
  }

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
  _download_file = sd_card_.open(filename, FILE_READ);
  if (!_download_file) {
    Serial.print(filename);
    junk::die("Can't open file");
    return;
  }

  Serial.print(_download_file.size());
  while(true) {
    if (Serial.available()) {
      Serial.read();
      Serial.print("XXX\n");     //TEMP!!!
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


static void report_error(const CommandParser &parser)
{
  Serial.print("Xp");
  Serial.print(parser.error());
  Serial.print('\n');
}


static bool set_event_bits(CommandParser &parser, uint16_t &event_bits)
{
  // trigger_event()
  const uint16_t max_event_val = (1 << _num_events) - 1;
  const uint16_t events = parser.get_uint32(0, max_event_val);
  const bool okay = parser.check_complete();
  if (!okay) {
    report_error(parser);
    return false;
  }
  event_bits = events;
  return true;
}


static void manual_event_trigger(CommandParser &parser)
{
  if (set_event_bits(parser, _forced_events)) {
    Serial.print("e\n");
  }
}


static void event_enabling(CommandParser &parser)
{
  if (set_event_bits(parser, _event_enabled)) {
     Serial.print("E\n");
  }
}


static void file_transfer(CommandParser &parser)
{
  const char *filename = parser.get_word();
  const bool okay = parser.check_complete();
  if (!okay) {
    report_error(parser);
    return;
  }
  Serial.print("G\n");
  FileTransfer ft = FileTransfer(sd_card_, filename);
  while (! ft.finished()) {
    ft.transfer_line();
  }
}


static void file_delete(CommandParser &parser)
{
  const char *filename = parser.get_word();
  const bool okay = parser.check_complete();
  if (!okay) {
    report_error(parser);
    return;
  }
  sd_card_.remove(filename);
  Serial.print("R\n");
}


static void process_commands(void)
{
  const size_t BUF_SIZE = 100;
  char buffer[BUF_SIZE];
  const size_t size = Serial.readBytes(buffer, BUF_SIZE);

  CommandParser parser = CommandParser(buffer, size);
  const int command = parser.get_char();

  if (parser.error()) {
    report_error(parser);
    return;
  }

  switch(command) {
    case 'e':
      manual_event_trigger(parser);
      return;

    case 'E':
      event_enabling(parser);
      return;

    case 'G':
      file_transfer(parser);
      return;

    case 'o':
      {
        const uint8_t mode = parser.get_uint32(0, 3);
        const bool okay = parser.check_complete();
        if (!okay) {
          report_error(parser);
          return;
        }
        log_to_term_ = mode == 1;
        Serial.print("o\n");
      }
      return;

    case 'R':
      file_delete(parser);
      return;

    case 'N':
      {
        // new_active_file() switch to new file
        const uint8_t file_num = parser.get_uint32(0, 99);
        const bool okay = parser.check_complete();
        if (!okay) {
          report_error(parser);
          return;
        }
        log_file.close();
        log_file = junk::set_log_file(file_num, FILE_WRITE);
        log_file.print("# Coweeta log file\n");    //TEMP!!!
        log_file.flush();

        Serial.print("N\n");
      }
      return;

    case 's':
      {
        // set time for sync_time()
        const int32_t seconds = parser.get_int32(0, 0x7FFFFFFF);
        const bool okay = parser.check_complete();
        if (!okay) {
          report_error(parser);
          return;
        }
        _logger->set_unix_time(seconds);
        Serial.print("s\n");
      }
      return;

    default:
      break;
  }

  // The remaining commands take no args.
  const bool okay = parser.check_complete();
  if (!okay) {
    report_error(parser);
    return;
  }

  switch(command) {
  case 'v':
    // protocol version
    Serial.print("v COW0.0\n");
    return;

  case 'A':
    // get_active_file_num()
    Serial.print("A");
    Serial.print(file_number, DEC);
    Serial.print('\n');

    break;

  case 'n':
    // list event names
    Serial.print("n ");
    for (uint8_t i = 0; i < _num_events; i++) {
      Serial.print(_schedule[i].name);
      Serial.print(" ");
    }
    Serial.print("\n");
    return;

  case 't':
    // get time for sync_time()
    Serial.print("t");
    Serial.print(_now, DEC);
    Serial.print('\n');

    return;

  case 'L':
    // list_files()
    junk::print_root_directory(sd_card_);
    return;

  case 'w':
    // report wait for next event
    Serial.print('w');
    Serial.print(_next_time - _now);
    Serial.print(' ');
    Serial.print(_triggered_events);
    Serial.print(' ');
    Serial.print(_event_enabled);
    Serial.print('\n');
    return;

  default:
    Serial.print("Ec?\n");
    return;

  }
}


DataLogger::DataLogger()
{
   _logger = this;
}

static void button_press_irc(void)
{
}

void DataLogger::setup()
{

  pinMode(good_led_pin_, OUTPUT);
  pinMode(bad_led_pin_, OUTPUT);
  if (beeper_pin_) {
    pinMode(beeper_pin_, OUTPUT);
  }
  if (button_pin_) {
    pinMode(button_pin_, INPUT);
    PcInt::attachInterrupt(button_pin_, button_press_irc);
  }

  Serial.begin(usb_usart_baud_rate_);
  Serial.print("# Coweeta Hydrologic Lab Datalogger\n");

  // connect to RTC
  Wire.begin();

  if (!rtc.begin()) {
    junk::die("RTC failed");
  }

  if (!sd_card_.begin(logger_cs_pin_)) {
    junk::die("Data logger card failed, or not present.");
  }

  file_number = junk::get_next_file_number();

  log_file = junk::set_log_file(file_number, FILE_WRITE);
  log_file.println("# Coweeta log file");   //TODO make variable and delay file write.
  log_file.flush();

  log_to_file_ = true;
  log_to_term_ = false;

  _forced_events = 0x0000;

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
  compute_next_time();
  while ((_now < _next_time) && !_forced_events) {
    if (Serial.available()) {
      process_commands();
      compute_next_time();
    } else {
      wait_a_while();
    }
    _now = _logger->get_unix_time();
  }
  if (_forced_events) {
    _triggered_events = _forced_events;
    _forced_events = 0x0000;
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
  // check if we have a log file
  // check if we have an sd card

  if (char_stream.bytes_written()) {

    if (log_to_file_) {
      char_stream.dump(log_file);
      log_file.print("\n");
      log_file.flush();
    }
    if (log_to_term_) {
      Serial.print("!");
      char_stream.dump(Serial);
      Serial.print("\n");
    }

  }

}


 //TEMP!!! remove??
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


void DataLogger::set_device_pins(uint8_t good_led, uint8_t bad_led, uint8_t sd_card)
{
  good_led_pin_ = good_led;
  bad_led_pin_ = bad_led;
  logger_cs_pin_ = sd_card;
}


void DataLogger::set_beeper_pin(uint8_t pin)
{
  beeper_pin_ = pin;
}


void DataLogger::set_button_pin(uint8_t pin)
{
  button_pin_ = pin;
}


void DataLogger::set_usb_baud_rate(uint32_t rate)
{
  usb_usart_baud_rate_ = rate;
}



}  // namespace coweeta