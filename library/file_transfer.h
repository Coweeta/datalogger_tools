#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <SdFat.h>

namespace coweeta {

class FileTransfer
{
private:
  File file_;
  bool finished_;

public:
  FileTransfer(SdFat &sd_card, const char *filename);
  ~FileTransfer();
  
  inline size_t file_size()
  {
    return file_.size();
  }

  inline bool finished()
  {
    return finished_;
  }
  void transfer_line();
};

} // namespace coweeta

#endif        //  #ifndef FILE_TRANSFER_H
