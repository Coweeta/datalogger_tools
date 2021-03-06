#ifndef Data_logger_h
#define Data_logger_h

#include "Arduino.h"

namespace coweeta {

// This is used exclusively by the EventSchedule structure.
typedef enum {
  Normal='n',
  Disabled='d'
} EventCategory;


// This structure is used to define at what frequency events are indended to
// occur.  A constant array of these structures is passed to the DataLogger
// object's set_schedule() method.  Rather than hand populate these structures
// the program would be best to employ helper functions such as event().
//
// Note that the maximum interval allowed is a little over 9 hours, so if you
// require something to happen at, say, a daily interval then you'd set an event
// with an 8 hour interval and check the time inside the handler code.  Or, if
// you have a higher frequency event, then just piggyback the low frequency
// event handling code on it.
typedef struct {
  EventCategory category;
  const char *name;
  int16_t interval;
  int16_t offset;
} EventSchedule;


// Used by DataLogger::set_date_and_time() to update the real-time clock (RTC)
// from the management software.
typedef struct {
  uint16_t year; // The current year (common era (CE)), e.g. 2017
  uint8_t month; // The ordinal month of the year (1 to 12)
  uint8_t day; // The day of the month (1 to 31)
} date_t;


// Each application will have a single DataLogger object.  This object handles
// all fiddly file handling, interfacing to the management software and so on.
// It is up to the program developer to ensure that only one instance exists and
// that it is correctly initialized - bad things are likely to happen otherwise.
class DataLogger {
public:
  // Constructor takes no arguments.
  DataLogger();

  // The setup() method is called from within the Arduino app's setup()
  // function.
  void setup(void);

  // The second phase of initialization, this method is called from the
  // Arduino app's setup() function too.
  void set_schedule(const EventSchedule *schedule, uint8_t num_events);

  // Where the microcontroller spends most of its time; waiting.
  // Returns when the next scheduled event is due to occur (say reading a
  // sensor) or when the human operator requests we trigger the event for the
  // purposes of system checking.
  void wait_for_event(void);

  // Once wait_for_event() returns we need to determine which event(s) are due
  // (unless there's only one declared).  This method is passed a mask which
  // is the logical ORing of all the events we want to test for.  If any of
  // those are due, the method returns true.
  //
  // Example:
  // if (logger.is_event(READ_TEMP | READ_WIND)) ...
  bool is_event(uint16_t event);

  // If we have determined that we are going to be recording something to our
  // CSV based log file then call this to create the new entry, starting with
  // a timestamp.
  void new_log_line(void);

  // Having called new_log_line() to start a new line, we would log data using
  // log_int() and friends.  This prepends the entry with a comma, so we get a
  // nice CSV file.
  void log_int(int value);

  // Text strings can be logged too.  It is the responsibility of the calling
  // program to ensure that that string doesn't contain commas (at least not
  // without appropriate escaping).  As with log_int(), the text is prepended
  // with a comma.  string is null terminated.
  void log_string(const char *string);

  // Add a floating point value to the log.  There will be dec_places digits
  // after the decimal place.  The value's magnitude  can't exceed that of a
  // 32 bit integer (i.e +/- 4e9).
  void log_float(double value, uint8_t dec_places);

  // In some situations we are not going to record a value for a particular
  // parameter, or set of parameters.  For example we may not read those
  // sensors on every event.  When this is the case call skip_entries() to
  // place commas in the log file so that the data columns are aligned.
  // This is only necessary when there will be further entries on this line.
  void skip_entries(uint8_t num);

  // Having recorded everything for this time, we call this method to
  // terminate the line and either write it to the log file, or pass it back
  // to the management application running on a laptop.
  void end_log_line(void);

  void get_time(int *hour, int *minute, int *second);

  void enable_events(uint16_t events);
  void disable_events(uint16_t events);

protected:
  void set_device_pins(uint8_t good_led, uint8_t bad_led, uint8_t sd_card);
  void set_beeper_pin(uint8_t pin);

  /// The button is used to signal that the SD card is being removed or replaced.
  void set_button_pin(uint8_t pin);
  void set_usb_baud_rate(uint32_t rate);

public:
  // Each of these methods is implemented in the board specific child class.

  // Called from within the main loop() function, will wait until the next event
  // is due, or there has been some input on the USB port (i.e. from the
  // management software).  This can put the micro into sleep mode to save
  // power.
  virtual void wait_a_while(void) = 0;

  // Returns the number of seconds since epoch.
  //TODO: deprecate.
  virtual uint32_t get_unix_time(void) = 0;

  // Sets the number of seconds since epoch.
  //TODO: deprecate.
  virtual void set_unix_time(uint32_t seconds) = 0;

  // Returns the time of day.  Used to determine if an event is due.
  // Let's use UTC time (Greenwich or Zulu time) to avoid daylight savings
  // grief.
   //TEMP!!! virtual uint32_t get_seconds_since_midnight(void) = 0;

  // Used to set the RTC from the management software.  The seconds field is the
  // number of seconds since midnight.
   //TEMP!!! virtual void set_date_and_time(uint32_t seconds, const date_t &date) = 0;

  // Called each time a line is written to the log file.  Must output a text
  // representation of the date and time to the stream.  An example of the
  // output is "2017-07-24 20:36:35".
  virtual void write_timestamp(Print &stream) = 0;

};


// A convenience function used to construct the EventSchedule array used by
// Datalogger::set_schedule() as part of set up.
inline EventSchedule event(const char* name, int16_t interval, int16_t offset=0, EventCategory category=Normal)
{
  return {.category=category, .name=name, .interval=interval, .offset=offset};
}


// Hour:Minute:Second - A convenience function used in EventSchedule
// construction.  Converts a interval given as a number of hours, minutes and
// seconds into the number of seconds.
inline int16_t HMS(int8_t hours, int8_t minutes, int8_t seconds)
{
  return hours * 60 * 60 + minutes * 60 + seconds;
}



} // namespace coweeta

#endif // Data_logger_h
