#include "str_util.h"


static char line[80];
static char *cursor;

void two_digit(int n) {
  *(cursor++) = (n / 10) + '0';
  *(cursor++) = (n % 10) + '0';
}




void write_sint(signed x) {
  if (x < 0) {
    *(cursor++) = '-';
    write_uint(-x);
  } else {
    write_uint(x);
  }
}


void write_uint(unsigned x) {
  char d[10];
  int i = 0;

  if (x == 0) {
    *(cursor++) = '0';
    return;
  }
  while (x > 0) {
    d[i] = '0' + (x % 10);
    x /= 10;
    i++;
  }
  while(i--) {
    *(cursor++) = d[i];
  }
}



const char *get_row(void) {
  *(cursor) = '\0';
  return line;
}


void new_row(void) {
    cursor = line;
}


void write_char(char ch) {
    *(cursor++) = ch;
}



