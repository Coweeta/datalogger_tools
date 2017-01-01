// Coweeta's Arduino Data Logger code

// Based on Adafruit's lighttemplogger.c

#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include "str_util.h"
#include "logger_core.h"


void print_root_directory() {
  File dir = SD.open("/");
  dir.rewindDirectory();
  while (true) {

    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
  dir.close();
}



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



int get_next_file_number(void) {

  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (!SD.exists(filename)) {
      return i;
    }
     
  }
  die("Ran out of filenames.");
    
}


File set_log_file(int file_number, int mode) {
  char filename[] = "LOGGER00.CSV";
  filename[6] = file_number / 10 + '0';
  filename[7] = file_number % 10 + '0';
  File log_file = SD.open(filename, mode);
  if (!log_file) {
    die("Couldn't create file.");
  } 
  return log_file;
  
}


static File log_file;
static int file_number = 0;


void setup() {
  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("Coweeta Hydrologic Lab Data Logger");

  
  // connect to RTC
  Wire.begin();  
  if (!rtc.begin()) {
    die("RTC failed");
  }

  pinMode(logger_cs_pin, OUTPUT);
  if (!SD.begin(logger_cs_pin)) {
    die("Data logger card failed, or not present.");
  }  

  file_number = get_next_file_number();
  log_file = set_log_file(file_number, FILE_WRITE);

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
  static bool log_to_serial = false;
  static bool log_to_file = true;
  
  delay(100);

  digitalWrite(green_led_pin, LOW);
  if (Serial.available()) {
    const int command = Serial.read();
    
    switch(command) {
      case 'G':
      {
        // get file
        const int file_num = get_int(0, 99);
        // log_file.close();
        
        File dataFile = set_log_file(file_num, FILE_READ);
      
        // if the file is available, write to it:
        if (dataFile) {
          while (dataFile.available()) {
            Serial.write(dataFile.read());
          }
          dataFile.close();
          Serial.println("");
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening datalog.txt");
        }
      
        }
        // dump
        break;

      case 'R':
        // remove
        {
          const int file_num = get_int(0, 99);
          char fn[] = "LOGGER00.CSV";
          fn[6] = file_num / 10 + '0';
          fn[7] = file_num % 10 + '0';
          SD.remove(fn);
        }
        break;
        // log_file.close();
          
      case 't':
        //Serial.println(xxx, DEC);
        // set time
        break;

      case 's':
        // show time
        break;

      case 'L':
        // list files
        print_root_directory();
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
  digitalWrite(green_led_pin, HIGH);
  new_log_row(time_stamp);

  logger_loop(time_stamp);

  write_char(0);
  
  const char *line = get_row();
  if (log_to_serial) {
    Serial.println(line);
  }

  if (log_to_file && log_file) {
    log_file.println(line);
    log_file.flush();
  }
  

}  



