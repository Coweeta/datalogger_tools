// Coweeta's Arduino Data Logger code

// Based on Adafruit's lighttemplogger.c

#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include "str_util.h"
#include "logger_core.h"

static RTC_DS1307 rtc; // define the Real Time Clock object


// for the data logging shield, we use digital pin 10 for the SD cs line
const int logger_cs_pin = 10;
const int red_led_pin = 2;
const int green_led_pin = 3;
const int log_interval = 10;

static void die(const char* error_str) {
  Serial.print("ERROR: ");
  Serial.println(error_str);
  digitalWrite(red_led_pin, HIGH);
  while(1);
}



void new_log_row(const DateTime &now) {

  new_row();
  const int year = now.year();

  write_uint(now.year());
  write_char('-');
  two_digit(now.month());
  write_char('-');
  two_digit(now.day());
  write_char(' ');
  two_digit(now.hour());
  write_char(':');
  two_digit(now.minute());
  write_char(':');
  two_digit(now.second());
}



File set_log_file(const DateTime *date_time) {

  File log_file;
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (!SD.exists(filename)) {
      // only open a new file if it doesn't exist
      File log_file = SD.open(filename, FILE_WRITE);
      if (!log_file) {
        die("Couldn't create file.");
      } 
      Serial.print("Logging to ");
      Serial.println(filename);
      return log_file;
      
    }
  }
  die("Ran out of filenames.");
    
}


static File log_file;

void setup() {
  Serial.begin(115200);
  Serial.println("Coweeta Hydrologic Lab Data Logger");

  
  // connect to RTC
  Wire.begin();  
  if (!rtc.begin()) {
    die("RTC failed");
  }

  const DateTime now = rtc.now();

  new_log_row(now);
  write_char(',');
  write_uint(123);
  write_char(',');
  write_sint(-123);
   
  Serial.println(get_row());
  

  pinMode(logger_cs_pin, OUTPUT);
//  if (!SD.begin(logger_cs_pin)) {
  if (!SD.begin(logger_cs_pin)) {
    die("Data logger card failed, or not present.");
  }  
  
  const DateTime time_stamp = rtc.now();

  log_file = set_log_file(&time_stamp);

  logger_setup();
}



int get_int(int min, int max) {
  while (true) {
    const int val = Serial.parseInt();
    if ((val >= min) && (val <= max)) {
      return val;
    }
    Serial.print("bad; retry: ");
  }
}




void loop() {
  static uint32_t last_time = 0;
  static int xxx;
  static bool log_to_serial = false;
  static bool log_to_file = true;
  
  delay(100);

  if (Serial.available()) {
    const int command = Serial.read();
    
    switch(command) {
      case 'g':
      {
        // get file
        const int file_num = get_int(0, 99);
        log_file.close();
        
        File dataFile = SD.open("datalog.txt");
      
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
        // dump
        break;
        
      case 't':
        Serial.println(xxx, DEC);
        // set time
        break;

      case 's':
        // show time
        break;

      case 'l':
      // list files
      break;

      case 'S':
        log_to_serial = get_int(0, 1);
        break;
        
      case 'F':
        log_to_file = get_int(0, 1);
        break;
        
      
    }
    
  }
  const DateTime time_stamp = rtc.now();
  const uint32_t current_time = time_stamp.unixtime();

  if (current_time == last_time) {
    return;
  }

  last_time = current_time;  
  if (current_time % log_interval != 0) {
    return;
  }

  new_log_row(time_stamp);

  logger_loop(time_stamp);

  write_char(0);
  
  const char *line = get_row();
  if (log_to_serial) {
    Serial.println(line);
  }

  if (log_to_file && log_file) {
    log_file.println(line);
  }
  

}  



