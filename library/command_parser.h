#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

enum {
  NONE,
  BAD_TERM = 't',  // The string is not terminated with a NL
  EMPTY = 'e',     // The string is empty (only spaces)
  MISSING = 'm',   // A field is missing.
  OVERFLOW = 'o',  // The integer has too macny digits
  NON_DIGIT = 'd', // What should be an integer has a wrong character
  RANGE = 'r',     // The integer is too bad or small
  EXTRA = 'x'      // Extra characters at end of string
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
    char error_;

    char peek();
    void skip_spaces();
    uint32_t get_int(void);
    bool prepare(bool skip_space);

  public:

    /// Instantiate the object with the string to process, and its size.
    CommandParser(char * buffer, uint8_t size);

    inline char error()
    {
      return error_;
    }

    bool check_complete();

    /// Read a single character.
    ///
    /// Doesn't skip spaces.
    char get_char();

    const char *get_word();

    const char *get_string();

    uint32_t get_uint32(uint32_t min=0, uint32_t max=0xFFFFFFFF);

    /// Read a signed integer from the string.
    ///
    /// If it is not in the specified range then return 0 and set the error
    /// flag.
    int32_t get_int32(int32_t min=-0x80000000, int32_t max=0x7FFFFFFF);

};

#endif        //  #ifndef COMMAND_PARSER_H
