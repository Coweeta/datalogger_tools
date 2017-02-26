
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
      Serial.print("\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
  dir.close();
}



static RTC_DS1307 rtc; // define the Real Time Clock object


// for the data logging shield, we use digital pin 10 for the SD cs line
const int logger_cs_pin = 10;
const int led_pin = 8;
const int log_interval = 10;


static void die(const char* error_str) {
  Serial.print("ERROR: ");
  Serial.println(error_str);
  digitalWrite(led_pin, HIGH);
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
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) {
      return i;
    }
     
  }
  die("Ran out of filenames.");
    
}

static File log_file;
static int file_number = 0;


File set_log_file(int file_num, int mode) {
  file_number = file_num;
  char filename[] = "LOGGER00.CSV";
  filename[6] = file_number / 10 + '0';
  filename[7] = file_number % 10 + '0';
  File log_file = SD.open(filename, mode);
  if (!log_file) {
    die("Couldn't create file.");
  }
  log_file.println("# Coweeta log file");
  return log_file;
  
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


static uint32_t current_time()  {
  const DateTime time_stamp = rtc.now();
  return time_stamp.unixtime();
}

static uint32_t next_time;
#define OFFSET 1
#define ALIGN 2

void data_logger_setup(uint16_t alignment) {
  pinMode(led_pin, OUTPUT);
  
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

  for (uint8_t i = 0; i < 8; i++) {
    delay(50);
    digitalWrite(led_pin, HIGH);
    delay(50);
    digitalWrite(led_pin, LOW);
  }

  next_time = current_time();
  next_time = next_time - (next_time % alignment) + alignment;
}

void data_logger_wait(uint16_t duration, uint8_t mode, bool permit_usb) {
  
  static bool log_to_serial = false;
  static bool log_to_file = true;

  digitalWrite(led_pin, LOW);

  if (not empty()) {
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
  
  switch (mode) {
    case OFFSET:
      next_time = next_time + duration;
      break;
    case ALIGN:
      next_time = next_time - (next_time % duration) + duration;
      break;
  }


  while (true) {
    const int32_t wait = next_time - current_time();  
    //TEMP!!! Serial.println(wait);
    if (wait <= 0) {
      break;
    }
  
    delay(100);
  
    if (Serial.available() and permit_usb) {
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
            }
            // if the file isn't open, pop up an error:
            else {
              Serial.println("error opening datalog.txt");
            }
          
          }
          break;
        case 'A':
          Serial.println(file_number, DEC);
          break;
  
        case 'R':
          // remove
          {
            const int file_num = get_int(0, 99);
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
            // switch to new file
            const int file_num = get_int(0, 99);
            log_file.close();
            log_file = set_log_file(file_num, FILE_WRITE);
            Serial.println();
          }
          break;
            
        case 's':
          {
            const uint32_t unix_time = Serial.parseInt();
            Serial.println(unix_time);
            rtc.adjust(DateTime(unix_time));
            Serial.println();
          }
          break;
  
        case 't':
          {
            const DateTime now = rtc.now();
            Serial.println(now.unixtime(), DEC);
          }
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

        case ' ':
          break;
          
        default:
          Serial.print("?");
          Serial.print(command);
          Serial.println("?");
          
        
      }
      
      Serial.print(">");
    }
  }
  new_log_row(DateTime(next_time));

  
  digitalWrite(led_pin, HIGH);
    

}  


void setup() {
  data_logger_setup(10);
  logger_setup();
  
}



void loop() {
  data_logger_wait(15, ALIGN, true); 
  logger_loop(NULL);
  data_logger_wait(3, OFFSET, false); 
  logger_loop(NULL);
  
}


