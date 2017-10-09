// Coweeta's Arduino Data Logger code

// Based on Adafruit's lighttemplogger.c

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
  #include "WConstants.h"
#endif

#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include <SdFat.h>
#include <RTCTimer.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>


extern SdFat sd_card_;

namespace junk {



void print_root_directory(const SdFat &sd_card) {
  File dir = sd_card.open("/");
  dir.rewindDirectory();
  while (true) {

    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    const int BUF_LEN = 13;
    char buf[BUF_LEN];
    const bool okay = entry.getName(buf, BUF_LEN);
    Serial.print("L ");
    Serial.print(buf);
    if (entry.isDirectory()) {
      Serial.print("/\n");
    } else {
      // files have sizes, directories do not
      Serial.print("\t");
      Serial.print(entry.size(), DEC);
      Serial.print("\n");
    }
    entry.close();
  }
  dir.close();
  Serial.print("L\n");
}


const int led_pin = 9;
const int log_interval = 10;


void die(const char* error_str) {
  Serial.print("ERROR: ");
  Serial.println(error_str);
  while(1) {
      digitalWrite(led_pin, HIGH);
      delay(500);
      digitalWrite(led_pin, LOW);
      delay(500);
  }

}


int get_next_file_number(void) {

  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (!sd_card_.exists(filename)) {
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
  File log_file = sd_card_.open(filename, mode);
  if (!log_file) {
    die("Couldn't create file.");
  }
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


uint32_t current_time()  {
  return rtc.now().getEpoch();
}


}

