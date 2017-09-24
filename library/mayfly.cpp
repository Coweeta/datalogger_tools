#include "mayfly.h"

#include <RTCTimer.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>

enum {
  GREEN_LED_PIN = 8,
  RED_LED_PIN = 9,
  SD_CARD_SS_PIN = 12,
  RTC_PIN = A7,
  BATTERY_SENSE_PIN = A6
};



MayflyDataLogger::MayflyDataLogger():
  good_led_pin_(GREEN_LED_PIN),
  bad_led_pin_(RED_LED_PIN),
  logger_cs_pin_(SD_CARD_SS_PIN)
{
  DataLogger::DataLogger();
}


MayflyDataLogger::setup()
{
  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wakeISR);

  //Setup the RTC in interrupt mode
  rtc.enableInterrupts(RTC_INT_PERIOD);

  //Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  DataLogger::setup();

}


uint32_t MayflyDataLogger::get_unix_time(void)
{
  return rtc.now().getEpoch();
}


void MayflyDataLogger::set_unix_time(uint32_t seconds)
{
  rtc.setEpoch(seconds);
}


void MayflyDataLogger::wait_a_while(void)
{
  Serial.flush();

  // The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();

  // Disable ADC
  ADCSRA &= ~_BV(ADEN);

  // Sleep time
  noInterrupts();
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();

  // Enable ADC
  ADCSRA |= _BV(ADEN);

  // This method handles any sensor specific wake setup
  sensorsWake();

}


char *MayflyDataLogger::write_timestamp(char *buffer, size_t len)
{
  *buffer = 'Z';
  return buffer + 1;
}


#if 0

//
// Bits copied from https://github.com/EnviroDIY/EnviroDIY_Mayfly_Logger/blob/master/examples/logging_to_EnviroDIY/logging_to_EnviroDIY.ino
//

#include <Arduino.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <SdFat.h>
#include <RTCTimer.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>


int RTC_PIN = A7;  // RTC Interrupt pin

static void wake_isr()
{
  //Leave this blank
}


void set_up_rtc_interrupt(void)
{
  // Sets up the sleep mode (used on device wake-up)
  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wake_isr);

  //Setup the RTC in interrupt mode
  rtc.enableInterrupts(RTC_INT_PERIOD);

  //Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}


}


void sleep_until_external_event(void)
{
}



// Puts the system to sleep to conserve battery life.
void systemSleep()
{

  // Wait until the serial ports have finished transmitting
  Serial.flush();

  // The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();

  // Disable ADC
  ADCSRA &= ~_BV(ADEN);

  // Sleep time
  noInterrupts();
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();

  // Enable ADC
  ADCSRA |= _BV(ADEN);

  // This method handles any sensor specific wake setup
  sensorsWake();
}

#endif