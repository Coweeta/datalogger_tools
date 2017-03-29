
#include "RTClib.h"
#include <Wire.h>

#include <Adafruit_ADS1015.h>

#include "data_logger.h"

using namespace coweeta;

#define MCP9808_I2CADDR_DEFAULT        0x18
#define MCP9808_REG_CONFIG             0x01

#define MCP9808_REG_CONFIG_SHUTDOWN    0x0100
#define MCP9808_REG_CONFIG_CRITLOCKED  0x0080
#define MCP9808_REG_CONFIG_WINLOCKED   0x0040
#define MCP9808_REG_CONFIG_INTCLR      0x0020
#define MCP9808_REG_CONFIG_ALERTSTAT   0x0010
#define MCP9808_REG_CONFIG_ALERTCTRL   0x0008
#define MCP9808_REG_CONFIG_ALERTSEL    0x0004
#define MCP9808_REG_CONFIG_ALERTPOL    0x0002
#define MCP9808_REG_CONFIG_ALERTMODE   0x0001

#define MCP9808_REG_UPPER_TEMP         0x02
#define MCP9808_REG_LOWER_TEMP         0x03
#define MCP9808_REG_CRIT_TEMP          0x04
#define MCP9808_REG_AMBIENT_TEMP       0x05
#define MCP9808_REG_MANUF_ID           0x06
#define MCP9808_REG_DEVICE_ID          0x07


static void i2c_write16(uint8_t i2c_addr, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(i2c_addr);
    Wire.write(reg);
    Wire.write(value >> 8);
    Wire.write(value & 0xFF);
    Wire.endTransmission();
}

static uint16_t i2c_read16(uint8_t i2c_addr, uint8_t reg) {
  uint16_t val;

  Wire.beginTransmission(i2c_addr);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(i2c_addr, (uint8_t)2);
  val = Wire.read();
  val <<= 8;
  val |= Wire.read();
  return val;
}

static Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
static DataLogger logger;



const int am_rst = 7;
const int am_clk = 6;

enum {
  alfa_read = 1,
  bravo_read_1 = 2,
  bravo_read_2 = 4,
  bravo_energize = 8
};

// Declare an array of structures that define what our schedule of events is.
// The alfa_read event occurs every minute, on the minute.
// The bravo events occur every five minutes.  The energize event occurs
// 10 seconds before the start of the minute, then read 1 occurs on the minute,
// followed 10 seconds later by read 2.
static const EventSchedule schedule[] = {
  event("MCP9808", HMS(0, 1, 0)),  
  event("AM416", HMS(0, 0, 30))
};

void setup() {
  logger.setup();
  logger.set_schedule(schedule, 2);

  // set up multiplexer control
  pinMode(am_rst, OUTPUT);
  pinMode(am_clk, OUTPUT);

  ads.setGain(GAIN_ONE);
  ads.begin();

}



void loop() 
{

  logger.wait_for_event();
  logger.new_log_line();
  if (logger.is_event(alfa_read)) {
      const uint16_t t_reg = i2c_read16(MCP9808_I2CADDR_DEFAULT, MCP9808_REG_AMBIENT_TEMP);
      const int16_t temp = (t_reg & 0x0FFF) - (t_reg & 0x1000);
      logger.log_int(temp);
  } else {
      logger.skip_entries(1);
  }
  if (logger.is_event(bravo_read_1)) {
    digitalWrite(am_clk, LOW);
    digitalWrite(am_rst, HIGH);

    for (int i = 0; i < 16; i++) {
      delay(5);
      digitalWrite(am_clk, HIGH);
      delay(10);
      const int16_t diff = ads.readADC_Differential_0_1();
      logger.log_int(diff);
      digitalWrite(am_clk, LOW);
    }
    digitalWrite(am_rst, LOW);
  }
  logger.end_log_line();
}

