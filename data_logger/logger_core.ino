
#include "RTClib.h"
#include <Wire.h>

#include <Adafruit_ADS1015.h>

#include "str_util.h"
#include "logger_core.h"

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

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


void i2c_write16(uint8_t i2c_addr, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(i2c_addr);
    Wire.write(reg);
    Wire.write(value >> 8);
    Wire.write(value & 0xFF);
    Wire.endTransmission();
}

uint16_t i2c_read16(uint8_t i2c_addr, uint8_t reg) {
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

/*
static uint8_t i2cread(void) {
  return Wire.read();
}

static void i2cwrite(uint8_t x) {
  Wire.write((uint8_t)x);
}

static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(i2cAddress);
  i2cwrite((uint8_t)reg);
  i2cwrite((uint8_t)(value>>8));
  i2cwrite((uint8_t)(value & 0xFF));
  Wire.endTransmission();
}

static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg) {
  Wire.beginTransmission(i2cAddress);
  i2cwrite(ADS1015_REG_POINTER_CONVERT);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddress, (uint8_t)2);
  return ((i2cread() << 8) | i2cread());  
}

static int16_t readADC_Differential_0_1() {
  // Start with default values
  const uint16_t configg = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_PGA_4_096V  | ADS1015_REG_CONFIG_MUX_DIFF_0_1 | ADS1015_REG_CONFIG_OS_SINGLE |
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Write config register to the ADC
  writeRegister(ADS1015_ADDRESS, ADS1015_REG_POINTER_CONFIG, configg);

  // Wait for the conversion to complete
  delay(ADS1115_CONVERSIONDELAY);

  // Read the conversion results
  const uint16_t res = readRegister(ADS1015_ADDRESS, ADS1015_REG_POINTER_CONVERT);
  return (int16_t)res;
  
}
*/

const int am_rst = 7;
const int am_clk = 6;

void logger_setup() {
  Wire.begin();

  // set up multiplexer control
  pinMode(am_rst, OUTPUT);
  pinMode(am_clk, OUTPUT);

  ads.setGain(GAIN_ONE); 
  ads.begin();

  // Serial.println(i2c_read16(MCP9808_I2CADDR_DEFAULT, MCP9808_REG_MANUF_ID),HEX);
  // Serial.println(i2c_read16(MCP9808_I2CADDR_DEFAULT, MCP9808_REG_DEVICE_ID),HEX);

}



void logger_loop(const DateTime &date_time) {
  const int value = analogRead(1);
  write_char(',');
  write_sint(value);
  const uint16_t t_reg = i2c_read16(MCP9808_I2CADDR_DEFAULT, MCP9808_REG_AMBIENT_TEMP);
  const int16_t temp = (t_reg & 0x0FFF) - (t_reg & 0x1000);
  write_char(',');
  write_uint(temp);

  digitalWrite(am_clk, LOW);
  digitalWrite(am_rst, HIGH);
  
  for (int i = 0; i < 16; i++) {
    delay(5);
    digitalWrite(am_clk, HIGH);
    delay(10);
    const int16_t diff = ads.readADC_Differential_0_1();
    write_char(',');
    write_sint(diff);
    digitalWrite(am_clk, LOW);
  }
  digitalWrite(am_rst, LOW);
}


