#include <SdFat.h>
#include "file_transfer.h"
#include "junk.h"

SdFat sd_card;

using namespace coweeta;

void setup() {
  Serial.begin(250000);
  Serial.print("Hello\n");
  sd_card.begin(12);
  junk::print_root_directory(sd_card);

}

void loop() {
  if (Serial.available()) {
    char filename[20];
    int len = Serial.readBytesUntil('\n', filename, 20);
    filename[len] = '\0';
    Serial.print("\nfile: \"");
    Serial.print(filename);
    Serial.print("\"\n");
    FileTransfer ft = FileTransfer(sd_card, filename);
    if (ft.finished()) {
      Serial.print("\nERROR!!!!!!!\n");
    } else {
      while(!ft.finished()) {
        const uint32_t t = micros();
        ft.transfer_line();
        Serial.flush();
        Serial.println(micros() - t);
      }
      Serial.print("\nDONE\n");
    }
    
  }
  // put your main code here, to run repeatedly:

}
