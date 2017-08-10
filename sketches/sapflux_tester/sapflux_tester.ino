#include <Adafruit_ADS1015.h>
#include <Adafruit_MCP9808.h>

#include "data_logger.h"

// Spare ourselves from having to prepend "coweeta::" to any the entities we use
// from  data_logger.h
using namespace coweeta;

// Declare our data logger object.  We will set it up later.
static DataLogger logger(49, 51, 53);

static Adafruit_ADS1115 adc;  // Our 16 bit ADC
static Adafruit_MCP9808 temp_sense;   // Our non-thermocouple temperature sensor

enum {
  log_temp = 1,
  start_seq = 2,
  stop_seq = 4,
  heat_pulse = 8
};

static const EventSchedule schedule[] = {
  // single
  event("log_temp", HMS(0, 0, 1), 0),
  event("start_seq", HMS(0, 30, 0), -10),
  event("stop_seq", HMS(0, 30, 0), 90),
  event("heat_pulse", HMS(0, 30, 0), 0)
};

static const int heater_power_pin = 31;
static const int high_input_line = 14;

void setup() {
  logger.setup();

  // Specify the schedule of events to follow.  In this case we use the one
  // defined above, and we state that there is just the one event type.
  logger.set_schedule(schedule, 4);

  adc.begin();
  adc.setGain(GAIN_SIXTEEN);

  temp_sense.begin();
  
  pinMode(heater_power_pin, OUTPUT);
  digitalWrite(heater_power_pin, LOW);
  

}


static bool now_logging = false;

void loop() {

  const char *state = 0;
  // put your main code here, to run repeatedly:
  logger.wait_for_event();

  if (logger.is_event(start_seq)) {
    now_logging = true;
    state = "begin";
  }
  if (logger.is_event(stop_seq)) {
    state = "end";
  }

  if (logger.is_event(heat_pulse)) {
    state = "heat";
  }
  if (logger.is_event(log_temp) and now_logging) {
    logger.new_log_line();
    const int16_t high_diff = adc.readADC_Differential_0_1();
    const int16_t low_diff = adc.readADC_Differential_2_3();
    const float base_temp = temp_sense.readTempC();
    const float high_temp = base_temp - high_diff * 0.1817;
    const float low_temp = base_temp - low_diff * 0.1817;
    const float batt_volt = analogRead(high_input_line) * 15.0 / 1024;
    logger.log_int(high_diff);
    logger.log_int(low_diff);
    logger.log_float(base_temp, 2);
    logger.log_float(high_temp, 2);
    logger.log_float(low_temp, 2);
    logger.log_float(high_temp - low_temp, 2);
    logger.log_float(batt_volt, 2);
    if (state) {
      logger.log_string(state);
    }
    logger.end_log_line();
  }

  if (logger.is_event(heat_pulse) and now_logging) {
    digitalWrite(heater_power_pin, HIGH);
    delay(500);
    digitalWrite(heater_power_pin, LOW);
  }

  if (logger.is_event(stop_seq)) {
    now_logging = false;
  }
    
}
