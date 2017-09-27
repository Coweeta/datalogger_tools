#include "mayfly.h"
#include <Adafruit_ADS1015.h>

using namespace coweeta;

enum {
  READ_TEMP = 1,
  READ_ADC = 2
};

const int NUM_EVENT_TYPES = 2;

static const EventSchedule schedule[NUM_EVENT_TYPES] = {
  event("read_temp", HMS(0, 1, 0)),
  event("read_adc", HMS(0, 0, 40))
};

static MayflyDataLogger logger;
static Adafruit_ADS1115 adc;  // Our 16 bit ADC


void setup() {
  logger.setup();
  logger.set_schedule(schedule, NUM_EVENT_TYPES);
  adc.begin();
}


void loop() {
  
  logger.wait_for_event();
  
  logger.new_log_line();
  
  if (logger.is_event(READ_TEMP)) {
    const float temperature = logger.rtc_temperature();
    logger.log_float(temperature, 2);
  } else {
    logger.skip_entries(1);
  }
  
  if (logger.is_event(READ_ADC)) {
    const int16_t voltage = adc.readADC_Differential_0_1();
    logger.log_int(voltage);
  }

  logger.end_log_line();

}

