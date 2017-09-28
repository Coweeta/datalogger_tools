#include "Arduino.h"

#include "command_parser.h"



/// Check the next char without advancing past it.
char CommandParser::peek() {
  return buffer_[cursor_];
}


/// Advance the cursor to the next non-space, or the end of the string.
void CommandParser::skip_spaces()
{
  while ((cursor_ < size_) && (buffer_[cursor_] == ' ')) {
    cursor_++;
  }
}


/// Try to interpret the next lump of text as a decical integer.
/// If it is not then return 0 and set the error flag.
uint32_t CommandParser::get_int(void)
{
  uint32_t val = 0;
  char ch = buffer_[cursor_++];
  do {
    if ((ch < '0') || (ch > '9')) {
        error_ = NON_DIGIT;
        return 0;
    }

    const uint8_t digit = ch - '0';

    if (val > 0xFFFFFFFF / 10) {
      error_ = OVERFLOW;
      return 0;
    }
    val *= 10;
    uint32_t new_val = val + digit;
    if (new_val < val) {
      error_ = OVERFLOW;
      return 0;
    }
    val = new_val;
    ch = buffer_[cursor_++];
  } while (ch != ' ');

  return val;
}

bool CommandParser::prepare(bool skip_space)
{
  if (error_) {
    return true;
  }
  if (skip_space)
  {
    skip_spaces();
  }
  if (cursor_ >= size_) {
    error_ = MISSING;
    return true;
  }
  return false;  // No fault.
}


/// Instantiate the object with the string to process, and its size.
CommandParser::CommandParser(char * buffer, uint8_t size) :
  buffer_(buffer),
  size_(size),
  cursor_(0),
  error_(NONE)
{
  skip_spaces();

  const uint8_t last_char_pos = size_ - 1;

  if (buffer_[last_char_pos] != '\n') {
    error_ = BAD_TERM;
    return;
  }

  buffer_[last_char_pos] = ' ';
  size_--;

  if (cursor_ >= size_) {
    error_ = EMPTY;
    return;
  }

  error_ = NONE;
}


/// Read a single character.
///
/// Doesn't skip spaces.
char CommandParser::get_char()
{
  const bool fault = prepare(false);
  if (fault) {
    return '\0';
  }
  return buffer_[cursor_++];
}


const char *CommandParser::get_blob()
{
  const bool fault = prepare(true);
  if (fault) {
    return "";
  }
  const char* start = buffer_ + cursor_;
  while (buffer_[cursor_] != ' ') {
    cursor_++;
  }
  buffer_[cursor_++] = '\0';
  return start;
}


const char *CommandParser::get_string()
{
  const bool fault = prepare(false);
  if (fault) {
    return "";
  }
  const char* start = buffer_ + cursor_;
  buffer_[size_] = '\0';
  cursor_ = size_;
  return start;
}


uint32_t CommandParser::get_uint32(uint32_t min, uint32_t max)
{
  const bool fault = prepare(true);
  if (fault) {
    return 0;
  }
  const uint32_t val = get_int();
  if (error_) {
    return 0;
  }
  if ((val < min) || (val > max))
  {
    error_ = RANGE;
    return 0;
  }
  return val;
}


/// Read a signed integer from the string.
///
/// If it is not in the specified range then return 0 and set the error
/// flag.
int32_t CommandParser::get_int32(int32_t min, int32_t max)
{
  const bool fault = prepare(true);
  if (fault) {
    return 0;
  }
  const char ch = peek();
  if (ch == '-') {
    get_char();
    const uint32_t val = get_int();
    if ((min > 0) || (val > uint32_t(-(min + 10)) + 10) || ((max < 0) && (val < uint32_t(-max)))) {
      error_ = RANGE;
      return 0;
    }
    return -int32_t(val);
  }
  if (ch == '+') {
    get_char();
  }
  const uint32_t val = get_int();
  if ((max < 0) || (val > uint32_t(max)) || ((min > 0) && (val < uint32_t(min)))) {
    error_ = RANGE;
    return 0;
  }
  return int32_t(val);
}

