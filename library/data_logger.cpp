#include "Arduino.h"
#include <Sodaq_PcInt.h>

#include "data_logger.h"
#include "char_stream.h"
#include "command_parser.h"
#include "file_transfer.h"
#include "utils.h"


namespace coweeta {


// List of all the scheduled events (_num_events of them)
static const EventSchedule* _schedule;
static uint8_t _num_events;

// time of next event (since epoch)  TODO: change to since midnight.
static uint32_t _next_time;

// Current time (since epoch)  TODO: change to since midnight.
static uint32_t _now;

static uint16_t _triggered_events;
static uint16_t _forced_events;
static File log_file;
static int file_number = 0;
static File _download_file;
static uint16_t _event_enabled = 0xFFFF;

// Arduino hardware pin addresses for indicator LEDs, the indicator buzzer and
// the user control button.  Set on startup.
static uint8_t good_led_pin_ = 0;
static uint8_t bad_led_pin_ = 0;
static uint8_t beeper_pin_ = 0;
static uint8_t button_pin_ = 0;


static uint8_t logger_cs_pin_ = 0;
static uint32_t usb_usart_baud_rate_ = 0;

// Flags to control where the log output is to be sent.
static bool log_to_file_;
static bool log_to_term_;

// The buffer where log output is placed.
static const int BUF_SIZE = 250;
char char_buf[BUF_SIZE];
CharStream char_stream(char_buf, BUF_SIZE);

static uint8_t sd_card_state_;

static DataLogger *_logger;
SdFat sd_card_;  // The SD initialization


// Called on a fatal error.
//TODO: probably don't use this in production...
static void die(const char* error_str) {
  Serial.print("# ERROR: ");
  Serial.println(error_str);
  while(1) {
      digitalWrite(bad_led_pin_, HIGH);
      delay(500);
      digitalWrite(bad_led_pin_, LOW);
      delay(500);
  }

}


// Returns the time that the specified event is due to occur.
static uint32_t next_time_for_event(const EventSchedule* schedule)
{
  const int16_t interval = schedule->interval;
  const int16_t offset = schedule->offset;
  const int32_t next = (((_now - offset) / interval) + 1) * interval + offset;
  return next;
}


// Looks at the list of scheduled events and determines which ones are due next
// and what that time is.  Sets the file scope variables _next_time and
// _triggered_events.
//
// If _forced_events is set then nothing is done: _next_time and
// _triggered_events will already have been set.
//
// If no events are scheduled then _next_time is set to 0xFFFFFFFF
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


 //TEMP!!!
// Transmits the named file from the SD card down the serial link to the
// management software.
// static void send_file(const char *filename)
// {
//   _download_file = sd_card_.open(filename, FILE_READ);
//   if (!_download_file) {
//     Serial.print(filename);
//     Serial.print(filename);
//     ("X Can't open file");
//     return;
//   }
//
//   Serial.print(_download_file.size());
//   while(true) {
//     if (Serial.available()) {
//       Serial.read();
//       Serial.print("XXX\n");     //TEMP!!!
//       break;
//     }
//     if (not _download_file.available()) {
//       break;
//     }
//     const uint8_t bytes_read = _download_file.readBytes(char_buf, BUF_SIZE);
//     Serial.write(char_buf, bytes_read);
//   }
//   _download_file.close();
// }


// Reports a parser error back down the USB link to the management software.
static void report_error(const CommandParser &parser)
{
  Serial.print("Xp");
  Serial.print(parser.error());
  Serial.print('\n');
}


uint16_t get_next_file_number(void) {
  for (uint16_t file_num = 0; file_num < 1000; file_num++) {
    const char *filename = build_filename(file_num);
    if (!sd_card_.exists(filename)) {
      return file_num;
    }
  }
  die("Ran out of filenames.");
}


File open_log_file(uint16_t file_num, int mode)
{
  const char *filename = build_filename(file_num);
  File log_file = sd_card_.open(filename, mode);
  if (!log_file) {
    die("Couldn't create file.");
  }
  return log_file;
}


// The management software is trying to either enable/disable particular events,
// or it is trying to force some events to trigger immediately.  (Determined
// by whether event_bits is a reference to _triggered_events or _forced_events.
//
// If the command is bad (e.g. the event mask passed is too big) then the
// routine returns false and an error is reported back up the USB.
// Otherwise the appropriate flag variable is set.
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


// The user (management software) wants some events to trigger immediately.
static void manual_event_trigger(CommandParser &parser)
{
  if (set_event_bits(parser, _forced_events)) {
    Serial.print("e\n");
  }
}


// The user (management software) is enabling and disabling particular events,
// as determine by the event mask.
static void event_enabling(CommandParser &parser)
{
  if (set_event_bits(parser, _event_enabled)) {
     Serial.print("E\n");
  }
}


// Transfer a file from the SD card up the USB to the management software.
// Immediately send the file size (in bytes), and then start the transfer
// operation.
//TODO: Currently the datalogger is locked until the file is sent.  Change this so that it can continue in the background.
static void file_transfer(CommandParser &parser)
{
  const char *filename = parser.get_word();
  const bool okay = parser.check_complete();
  if (!okay) {
    report_error(parser);
    return;
  }
  FileTransfer ft = FileTransfer(sd_card_, filename);
  Serial.print("G");
  Serial.print(ft.file_size());
  Serial.print("\n");
  while (! ft.finished()) {
    ft.transfer_line();
  }
}

// Remove the named file from the SD card.
//TODO: error checking on operation.
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


// We believe a command has arrived from the management software.  Parse it and
// act on it.
static void process_command(void)
{
  const size_t BUF_SIZE = 100;  //TODO: yuck
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
        //TODO: split out
        const uint8_t file_num = parser.get_uint32(0, 99);
        const bool okay = parser.check_complete();
        if (!okay) {
          report_error(parser);
          return;
        }
        log_file.close();
        log_file = open_log_file(file_num, FILE_WRITE);
        log_file.print("# Coweeta log file\n");    //TEMP!!!
        log_file.flush();

        Serial.print("N\n");
      }
      return;

    case 's':
      {
        //TODO: split out
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
    print_root_directory(sd_card_);
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


// On a successful start up fire up the bells and whistles for a few seconds.
//
// Called by DataLogger::setup() only.
static void say_hello(void)
{
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


// Called from the Arduino standard setup() function, handles all of the
// setup specific to the Datalogger core:
//
// - set up the indicator pins as outputs
// - set up USB port
// - set up the SD card
// - do a little beep/LED flicker thing to say all's well
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

  if (!sd_card_.begin(logger_cs_pin_)) {
    die("Data logger card failed, or not present.");
  }

  file_number = get_next_file_number();

  log_file = open_log_file(file_number, FILE_WRITE);
  log_file.print("# Coweeta log file\n");   //TODO make variable and delay file write.
  log_file.flush();

  log_to_file_ = true;
  log_to_term_ = false;

  _forced_events = 0x0000;

  say_hello();

}

// Called from the Arduino setup() routine to register all of the events covered.
//
// This should be called after DataLogger::setup()
// schedule_list is a pointer to an array of schedule structures.  It should be
// static (or on the heap) as we don't copy it, just set a pointer to it.
// num_events should equal the number of items in the array.
//
// Each event's enable bit is set according to the category field in the
// corresponding event's structure.
void DataLogger::set_schedule(const EventSchedule *schedule_list, uint8_t num_events)
{
  _schedule = schedule_list;
  _num_events = num_events;
  for (uint8_t i = 0; i < num_events; i++) {
    if (schedule_list[i].category == Disabled) {
      _event_enabled &= ~(1 << i);
    }
  }
}


// Called from the Arduino loop() function, pauses the processor until needed.
//
// When any of the registered events triggers, or is forced by the management
// software, it returns.
//
// The DataLogger::is_event() method is used by the client code to determine
// which events have been triggered.
//
// Also extinguishes the active (good) LED while we are waiting, reigniting it
// when we are done.  I.e. the active light is on only when we are active.
//
void DataLogger::wait_for_event(void)
{

  _now = _logger->get_unix_time();
  digitalWrite(good_led_pin_, LOW);
  compute_next_time();
  while ((_now < _next_time) && !_forced_events) {
    if (Serial.available()) {
      process_command();
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


// Return true if any of the events in the mask has just triggered.
//
// Called from the Arduino loop() function after DataLogger::wait_for_event()
// has returned.
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


// Registers the Arduino pin number for the LEDs and the sd_card.
//
// Would be called by the board specific implementation of the DataLogger class.
//
//TODO: split into one method per pin
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
