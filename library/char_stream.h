#include "Print.h"

class CharStream: public Print
{
private:
  char *buffer_;
  size_t size_;
  size_t pos_;

public:
  CharStream(char *buffer, uint8_t size);
  size_t write(const uint8_t *buffer, size_t size);
  size_t write(uint8_t ch);

  inline void reset(void)
  {
    pos_ = 0;
  }

  inline void dump(Print &dest)
  {
    dest.write(buffer_, pos_);
  }

  inline size_t bytes_written(void)
  {
    return pos_;
  }
};