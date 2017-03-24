#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
  #include "WConstants.h"
#endif

#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

namespace junk {


void print_root_directory();
void die(const char* error_str);

void new_log_row(const DateTime &now);

int get_next_file_number(void);
File set_log_file(int file_num, int mode);

int get_int(int min, int max) ;
uint32_t current_time() ;

}