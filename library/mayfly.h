#ifndef MAYFLY_DATA_LOGGER_H_
#define MAYFLY_DATA_LOGGER_H_

#include "data_logger.h"

class MayflyDataLogger: public DataLogger {
  public:
    MayflyDataLogger();
    void setup(void);

  private:
    void wait_a_while(void);

    uint32_t get_unix_time(void);
    void set_unix_time(uint32_t);

    char *write_timestamp(char *buffer, size_t len);
}

#endif  // MAYFLY_DATA_LOGGER_H_