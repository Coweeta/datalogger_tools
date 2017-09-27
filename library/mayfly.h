#ifndef MAYFLY_DATA_LOGGER_H_
#define MAYFLY_DATA_LOGGER_H_

#include "data_logger.h"

namespace coweeta
{

class MayflyDataLogger : public DataLogger
{
  public:
    MayflyDataLogger();
    void setup(void);
    float rtc_temperature(void);

  private:
    void wait_a_while(void);

    uint32_t get_unix_time(void);
    void set_unix_time(uint32_t);

    void write_timestamp(Print &stream);
};

} // namespace coweeta

#endif  // MAYFLY_DATA_LOGGER_H_