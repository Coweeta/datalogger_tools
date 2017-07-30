#include "data_logger.h"

#include <Adafruit_ADS1015.h>

// Spare ourselves from having to prepend "coweeta::" to any the entities we use
// from  data_logger.h
using namespace coweeta;

// Declare our data logger object.  We will set it up later.
static DataLogger logger;

static Adafruit_ADS1115 ads;  // Our 16 bit ADC

static const EventSchedule schedule[] = {
  event("toggle", HMS(1, 0, 0))
};

static const int power_pin = 22;

void setup() {
  logger.setup();

  // Specify the schedule of events to follow.  In this case we use the one
  // defined above, and we state that there is just the one event type.
  logger.set_schedule(schedule, 1);

  ads.begin();
  ads.setGain(GAIN_ONE);
  pinMode(power_pin, OUTPUT);
  digitalWrite(power_pin, LOW);

}

static const int BUF_SIZE = 32; 
static int16_t buf[BUF_SIZE];

static uint8_t power = HIGH;

void loop() {
  // put your main code here, to run repeatedly:
  logger.wait_for_event();
  for (uint8_t i = 0; i < BUF_SIZE; i++) {
    if (i == 4) {
      digitalWrite(power_pin, power);
    }
    buf[i] = ads.readADC_Differential_0_1();
    
  }
  // toggle
  power = (power == HIGH) ? LOW : HIGH;

  logger.new_log_line();
  for (uint8_t i = 0; i < BUF_SIZE; i++) {
      logger.log_int(buf[i]);
  }
  logger.end_log_line();    

}
