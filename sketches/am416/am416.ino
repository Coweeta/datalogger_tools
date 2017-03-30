
// Example Coweeta data logger sketch 
//
// This uses:
// * an Adafruit 16 bit analog to digital converter, 
// * an Adafruit temperature sensor and
// * a Campbell Scientific relay multiplexer.
//
//
#include <Adafruit_ADS1015.h>
#include <Adafruit_MCP9808.h>

#include "data_logger.h"

using namespace coweeta;


class Am416 {
public:
  Am416();
  void setup(uint8_t channels, uint8_t clk_pin, uint8_t res_pin);
  void read_all(void (*callback)(uint8_t channel));
private:
  uint8_t _channels;
  uint8_t _clk_pin;
  uint8_t _res_pin;
  void (*_callback)(uint8_t channel);
};

Am416::Am416()
{
}

void Am416::setup(uint8_t channels, uint8_t clk_pin, uint8_t res_pin)
{
  _channels = channels;
  _clk_pin = clk_pin;
  _res_pin = res_pin;
  pinMode(_clk_pin, OUTPUT);
  pinMode(_res_pin, OUTPUT);
}


void Am416::read_all(void (*callback)(uint8_t channel))
{
  digitalWrite(_clk_pin, LOW);
  digitalWrite(_res_pin, HIGH);

  for (uint8_t i = 0; i < _channels; i++) {
      delay(5);
      digitalWrite(_clk_pin, HIGH);
      delay(10);
      callback(i);
      digitalWrite(_clk_pin, LOW);
    }
    digitalWrite(_res_pin, LOW);
}

static Adafruit_ADS1115 ads;  // Our 16 bit ADC
static Adafruit_MCP9808 mcp;  // Our temperature probe
static Am416 am;   

static DataLogger logger;



enum {
  temp_read = 1,
  adc_read = 2
};


static const EventSchedule schedule[] = {
  event("temp_read", HMS(0, 1, 0)),
  event("adc_read", HMS(0, 0, 30))
};

void setup() {
  logger.setup();
  logger.set_schedule(schedule, 2);

  am.setup(16, 6, 7);
  
  ads.begin();
  ads.setGain(GAIN_SIXTEEN);

  mcp.begin();
}


static void read_channel(uint8_t channel)
{
  const int16_t diff = ads.readADC_Differential_0_1();
  logger.log_int(diff);
}



void loop()
{

  logger.wait_for_event();
  
  logger.new_log_line();
  if (logger.is_event(temp_read)) {
      float temp = mcp.readTempC();
      logger.log_float(temp, 1);
  } else {
      logger.skip_entries(1);
  }
  
  if (logger.is_event(adc_read)) {
    am.read_all(read_channel);
  }
  
  logger.end_log_line();
}

