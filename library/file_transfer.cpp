#include "file_transfer.h"

namespace coweeta {

const int MAX_LINE_LEN = 200;

FileTransfer::FileTransfer(SdFat &sd_card, const char *filename)
{
  file_ = sd_card.open(filename, FILE_READ);
  if (!file_) {
    finished_ = true;
    return;
  }
  finished_ = false;

}

FileTransfer::~FileTransfer()
{
  if (file_) {
    file_.close();
  }
}

void FileTransfer::transfer_line()
{
  size_t sent = 0;
  // transfer type is ' ': file download.
  Serial.print(' ');
  while (true)
  {
    if (!file_.available()) {
      // file ends with non-NL
      Serial.print("\\XX\n");
      finished_ = true;
      return;
    }
    const char ch = file_.read();
    if (ch == '\n') {
      // end of line; send it.
      Serial.print('\n');
      if (!file_.available()) {
        finished_ = true;
        Serial.print(" \\XN\n");
      }
      return;
    } else if ((ch < ' ') || (ch > '~') || (ch == '\\')) {
      // non-printable char
      Serial.print('\\');
      Serial.print(int(ch / 16), HEX);
      Serial.print(int(ch % 16), HEX);
      sent += 3;
    } else {
      Serial.print(ch);
      sent++;
    }
    if (sent > MAX_LINE_LEN) {
      Serial.print("\\XT\n");
      return;
    }

  }
}

} // namespace coweeta
