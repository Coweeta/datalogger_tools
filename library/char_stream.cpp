#include "char_stream.h"

CharStream::CharStream(char *buffer, uint8_t size):
    buffer_(buffer),
    size_(size),
    pos_(0)
{
}


size_t CharStream::write(const uint8_t *buffer, size_t size)
{
  for (size_t i = 0; i < size; i++, pos_++) {
    if (pos_ == size_) {
      setWriteError(1);
      return i;
    }
    buffer_[pos_] = buffer[i];
  }
  return size;
}


size_t CharStream::write(uint8_t ch)
{
  if (pos_ == size_) {
    setWriteError(1);
    return 0;
  }
  buffer_[pos_++] = ch;
  return 1;
}