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
  static const int led_pin = 8;


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




  static void process_commands(void)
  {

    const int command = Serial.read();
    switch(command) {
    case 'e':
      {
        // trigger_event()
        _triggered_events = junk::get_int(0, (1 << _num_events) - 1);
        _state = TERM_LOG;
      }
      break;

    case 'G':
      {
        // download_file()
        const int file_num = junk::get_int(0, 99);
        // log_file.close();

        File dataFile = junk::set_log_file(file_num, FILE_READ);

        // if the file is available, write to it:
        if (dataFile) {
          while (dataFile.available()) {
            Serial.write(dataFile.read());
          }
          dataFile.close();
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening datalog.txt");
        }

      }
      break;

    case 'A':
      // get_active_file_num()
      Serial.println(file_number, DEC);
      break;

    case 'R':
      // remove as part of download_file()
      {
        const int file_num = junk::get_int(0, 99);
        char fn[] = "LOGGER00.CSV";
        fn[6] = file_num / 10 + '0';
        fn[7] = file_num % 10 + '0';
        SD.remove(fn);
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
        Serial.println();
      }
      break;

    case 's':
      {
        // set time for sync_time()
        const uint32_t unix_time = Serial.parseInt();
        Serial.println(unix_time);
        rtc.adjust(DateTime(unix_time));
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

    pinMode(led_pin, OUTPUT);

    Serial.begin(115200);
    Serial.println("Coweeta Hydrologic Lab Datalogger");


    // connect to RTC
    Wire.begin();

    if (!rtc.begin()) {
      junk::die("RTC failed");
    }

    pinMode(logger_cs_pin, OUTPUT);
    if (!SD.begin(logger_cs_pin)) {
      junk::die("Data logger card failed, or not present.");
    }

    file_number = junk::get_next_file_number();
    log_file = junk::set_log_file(file_number, FILE_WRITE);

    for (uint8_t i = 0; i < 8; i++) {
      delay(50);
      digitalWrite(led_pin, HIGH);
      delay(50);
      digitalWrite(led_pin, LOW);
    }

  }


  void DataLogger::set_schedule(const EventSchedule *schedule, uint8_t num_events)
  {
    _schedule = schedule;
    _num_events = num_events;
  }


  void DataLogger::wait_for_event(void)
  {
    _now = junk::current_time();
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