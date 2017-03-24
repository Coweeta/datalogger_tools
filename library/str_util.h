#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
  #include "WConstants.h"
#endif

void two_digit(int n);

void write_sint(signed x);

void write_uint(unsigned x);

const char *get_row(void);

void new_row(void);

void write_char(char ch);

bool empty(void);
