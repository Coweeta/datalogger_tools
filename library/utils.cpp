#include "Arduino.h"
#include "utils.h"
namespace coweeta {

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


const char*  build_filename(uint16_t file_num) {
   static char filename[] = "LOG_000.CSV";
   filename[4] = file_num / 100 + '0';
   filename[5] = (file_num / 10) % 10 + '0';
   filename[6] = file_num % 10 + '0';
   return filename;
}


}
