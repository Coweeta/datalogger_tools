#include "Arduino.h"
#include <iostream>
#include <string.h>
#include <stdio.h>


#include "command_parser.h"

void test_u_numbers(uint32_t val, uint32_t min=0, uint32_t max=0xFFFFFFFF, uint8_t status=NONE)
{
  char buf[100];
  snprintf(buf, 100, "%u\n", val);
  CommandParser cp = CommandParser(buf, strlen(buf));
  uint32_t got = cp.get_uint32(min, max);
  uint8_t err = cp.status();
  if (err != status) {
    std::cout << "Bad status: want " << int(status) << ", got " << int(err) << " \"" << buf << "\"\n";
  } else if ((status == NONE) && (got != val)) {
    std::cout << "Bad val: want " << val << ", got " << got << "\n";
  } else {
    std::cout << val << " okay\n";
  }
}

void test_s_numbers(int32_t val, int32_t min=-0x80000000, int32_t max=0x7FFFFFFF, uint8_t status=NONE)
{
  char buf[100];
  snprintf(buf, 100, "%d\n", val);
  CommandParser cp = CommandParser(buf, strlen(buf));
  int32_t got = cp.get_int32(min, max);
  uint8_t err = cp.status();
  if (err != status) {
    std::cout << "Bad status: want " << int(status) << ", got " << int(err) << " \"" << buf << "\"\n";
  } else if ((status == NONE) && (got != val)) {
    std::cout << "Bad val: want " << val << ", got " << got << "\n";
  } else {
    std::cout << val << " okay\n";
  }
}

int main() {

  char buf[100];
  strcpy(buf, "  H there 123456789 -456 678 +0 0 bob  this is the end \n");


  std::cout << buf << "\n";
  CommandParser cp = CommandParser(buf, strlen(buf));

  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "CMD = '" << char(cp.get_char()) << "'\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "WORD = \"" << cp.get_word() << "\"\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "NUM = " << cp.get_uint32(10000, 0xFFFFFFFF) << "\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "NUM = " << cp.get_int32(-1000, 1000) << "\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "NUM = " << cp.get_int32(-1000, 1000) << "\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "NUM = " << cp.get_int32(-1000, 1000) << "\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "NUM = " << cp.get_int32(-1000, 1000) << "\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "WORD = \"" << cp.get_word() << "\"\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  std::cout << "WORD = \"" << cp.get_string() << "\"\n";
  std::cout << "STAT = (" << int(cp.status()) << ")\n\n";

  test_u_numbers(100);
  test_u_numbers(0xFFFFFFFF);
  test_u_numbers(0);
  test_u_numbers(0, 0, 10);
  test_u_numbers(10, 0, 10);
  test_u_numbers(0, 1, 10, RANGE);
  test_u_numbers(11, 0, 10, RANGE);

  test_s_numbers(0);
  test_s_numbers(-100);
  test_s_numbers(100);
  test_s_numbers(-0x80000000);
  test_s_numbers(0x7FFFFFFF);
  test_s_numbers(0, -7, 9);
  test_s_numbers(-7, -7, 9);
  test_s_numbers(9, -7, 9);
  test_s_numbers(-8, -7, 9, RANGE);
  test_s_numbers(10, -7, 9, RANGE);

  test_s_numbers(-6, -10, -6);
  test_s_numbers(-11, -10, -6, RANGE);
  test_s_numbers(-5, -10, -6, RANGE);


  return cp.status();
}
