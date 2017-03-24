#ifndef Data_logger_h
#define Data_logger_h

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
  #include "WConstants.h"
#endif

namespace datalogger {

inline int16_t interval(int8_t hours, int8_t minutes, int8_t seconds)
{
    return hours * 60 * 60 + minutes * 60 + seconds;
}

typedef struct {
    char category;
    int16_t interval;
    int16_t offset;
} EventSchedule;


class DataLogger {
  public:
    DataLogger();
    void setup(void);
    void set_schedule(const EventSchedule *schedule, uint8_t num_events);

    void wait_for_event(void);
    // void wait_for_division(int division);
    bool is_event(uint8_t event);

    void new_log_line(void);
    // void log_string(const char *string);
    void log_int(int value);
    // void log_float(double value);
    void add_delimiters(uint8_t num);
    // void output_line();

    void get_time(int *hour, int *minute, int *second);

};



}

#endif // Data_logger_h
