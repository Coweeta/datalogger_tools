#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

enum {
  NONE,
  BAD_TERM,
  EMPTY,
  MISSING,
  BAD_NUM,
  OVERFLOW,
  NON_DIGIT,
  RANGE
};

/// Let's parse the command sent to the Logger from the management software.
/// Unlike the Arduino Serial::parseInt() method and friends, CommandParser
/// will communicate errors.
///
/// It is instantiated with the incoming string.
///
/// Note: that the string is modified by the object.
///
class CommandParser
{
  private:
    char *buffer_;
    uint8_t size_;
    uint8_t cursor_;
    uint8_t error_;


    char peek();
    void skip_spaces();
    uint32_t get_int(void);
    bool prepare(bool skip_space);

  public:

    /// Instantiate the object with the string to process, and its size.
    CommandParser(char * buffer, uint8_t size);

    inline uint8_t status()
    {
      return error_;
    }

    /// Read a single character.
    ///
    /// Doesn't skip spaces.
    char get_char();

    const char *get_blob();

    const char *get_string();


    uint32_t get_uint32(uint32_t min=0, uint32_t max=0xFFFFFFFF);


    /// Read a signed integer from the string.
    ///
    /// If it is not in the specified range then return 0 and set the error
    /// flag.
    int32_t get_int32(int32_t min=-0x80000000, int32_t max=0x7FFFFFFF);


};

#endif        //  #ifndef COMMAND_PARSER_H
