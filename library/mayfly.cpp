#include "mayfly.h"

#include <avr/sleep.h>
#include <RTCTimer.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>
/// USB comms needs the D21 (PCINT21) button to be pressed to wake up the micro.
/// Following this being pressed - or any USB traffic - the controller avoids
/// power-down for a minute.
///

namespace coweeta
{

enum {
  GREEN_LED_PIN = 8,
  RED_LED_PIN = 9,
  SD_CARD_SS_PIN = 12,
  RTC_PIN = A7,
  BATTERY_SENSE_PIN = A6
};


MayflyDataLogger::MayflyDataLogger()
{
  good_led_pin_ = GREEN_LED_PIN;
  bad_led_pin_ = RED_LED_PIN;
  logger_cs_pin_ = SD_CARD_SS_PIN;
}


static void wakeISR(void)
{
  //Leave this blank
}


void MayflyDataLogger::setup(void)
{
  pinMode(RTC_PIN, INPUT_PULLUP);

  PcInt::attachInterrupt(RTC_PIN, wakeISR);

  //Setup the RTC in interrupt mode
  rtc.enableInterrupts(EverySecond);

  set_sleep_mode(SLEEP_MODE_IDLE);

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

  // Re-enable ADC
  ADCSRA |= _BV(ADEN);

}


char *MayflyDataLogger::write_timestamp(char *buffer, size_t len)
{
  *buffer = 'Z';
  return buffer + 1;
}



float MayflyDataLogger::rtc_temperature(void)
{
  return rtc.getTemperature();
}

} // namespace coweeta
