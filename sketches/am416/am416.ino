
// Example Coweeta data logger sketch 
//
// This uses:
// * an Adafruit 16 bit analog to digital converter, 
// * an Adafruit temperature sensor and
// * a Campbell Scientific relay multiplexer.
//
//
#include <Adafruit_ADS1015.h>
#include <cs_am416.h>

#include "data_logger.h"

static void read_channel(uint8_t channel);

static const int CHANNELS = 12;

using namespace coweeta;

static Adafruit_ADS1115 ads;  // Our 16 bit ADC

enum {
  HEATER1_PIN = 7,
  HEATER2_PIN = 5,
  GOOD_LED_PIN = 49,
  BAD_LED_PIN = 51,
  BEEPER_PIN = 53,
  AM_CLK_PIN = 22,
  AM_RESET_PIN = 23
};
 


enum {
  PREHEAT_TIME = 10,
  HEAT1_TIME = 1,
  HEAT2_TIME = 3,
  RUN_LENGTH = 100
};

enum {
  start = 1,
  adc_read = 2
};


static Am416 am(AM_CLK_PIN, AM_RESET_PIN, CHANNELS, read_channel);   
static DataLogger logger(GOOD_LED_PIN, BAD_LED_PIN, BEEPER_PIN);

static const EventSchedule schedule[] = {
  event("start", HMS(0, 5, 0), -10),
  event("adc_read", HMS(0, 0, 1), 0, Disabled)
};

void setup() {
  logger.setup();
  logger.set_schedule(schedule, 2);

  am.setup();
  
  ads.begin();
  ads.setGain(GAIN_TWO);

  pinMode(HEATER1_PIN, OUTPUT);
  pinMode(HEATER2_PIN, OUTPUT);
  digitalWrite(HEATER1_PIN, LOW);
  digitalWrite(HEATER2_PIN, LOW);
  

}

static int16_t int_diff[CHANNELS];

static void read_channel(uint8_t channel)
{
  int_diff[channel] = ads.readADC_Differential_0_1();
}


const double v_ref_bridge = 3300.0; //2560.0;
const double v_ref_adc = 2048.0; // 1024.0; // 512; // 256.0;
const double r_bridge = 10e3;
const double b_therm = 3380.0;
const double t_offset = 273.0;
const double t_zero = t_offset + 25;
const double r_zero = 10e3;
const double r_inf = r_zero * exp(-b_therm / t_zero);
      

void loop()
{

  static uint16_t step = 0;
  char *msg = NULL;
  
  logger.wait_for_event();
  
  logger.new_log_line();

  if (logger.is_event(start)) {
    logger.enable_events(adc_read);
    step = 0;
  }

  
  if (logger.is_event(adc_read)) {
    am.read_all();
    if (step == 0) {
      msg = "start";
    } else if (step == PREHEAT_TIME) {
      digitalWrite(HEATER1_PIN, HIGH);
      msg = "heat1";
    } else if (step == PREHEAT_TIME + HEAT1_TIME) {  
      digitalWrite(HEATER1_PIN, LOW);
      digitalWrite(HEATER2_PIN, HIGH);
      msg = "heat2";
    } else if (step == PREHEAT_TIME + HEAT1_TIME + HEAT2_TIME) {  
      digitalWrite(HEATER2_PIN, LOW);
      msg = "heat off";
    }
    if (step == RUN_LENGTH) {
      logger.disable_events(adc_read);
      msg = "end";
    }
  
    step++;

    for (uint8_t channel = 0; channel < CHANNELS; channel++) {
      logger.log_int(int_diff[channel]);
    }
    logger.skip_entries(1);
    for (uint8_t channel = 0; channel < CHANNELS; channel++) {
      //logger.skip_entries(1);
      const double v_diff_mv = int_diff[channel] * v_ref_adc / 0x8000;
      //logger.log_float(v_diff_mv, 3);
      const double v_therm_mv = v_diff_mv + v_ref_bridge * 5.1 / 15.1;
      //logger.log_float(v_therm_mv, 3);
      const double r_par = r_bridge * v_therm_mv / (v_ref_bridge - v_therm_mv);
      //logger.log_float(r_par, 3);
      const double r_therm = 1 / ((1 / r_par) - (1 / 10e3));
      //logger.log_float(r_therm, 3);
      const double temp = b_therm / log(r_therm / r_inf) - t_offset;
      
      logger.log_float(temp, 3);
    }

    if (msg) {
      logger.log_string(msg);
    }
  
  }
  logger.end_log_line();
}



