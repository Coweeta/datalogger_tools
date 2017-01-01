
#include "RTClib.h"
#include "str_util.h"
#include "logger_core.h"

void logger_setup() {

}


void logger_loop(const DateTime &date_time) {
  const int value = analogRead(1);
  write_char(',');
  write_sint(value);
}


