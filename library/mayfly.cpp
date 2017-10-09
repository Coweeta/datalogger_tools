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

// Define assorted I/O pins for the Mayfly board.
enum {
  GREEN_LED_PIN = 8,
  RED_LED_PIN = 9,
  SD_CARD_SS_PIN = 12,
  RTC_PIN = A7,
  BATTERY_SENSE_PIN = A6
};


MayflyDataLogger::MayflyDataLogger()
{
  set_device_pins(GREEN_LED_PIN, RED_LED_PIN, SD_CARD_SS_PIN);
  set_button_pin(24);
  // Given the clock is 8MHz we can run the USB USART at 250kBaud with zero
  // rate error.  See ATmega1284 Datasheet Section 21.11. Examples of Baud Rate Setting
  // http://www.microchip.com/wwwproducts/en/ATMEGA1284
  set_usb_baud_rate(250000);
}


/// Triggered every second by the DS3231 real-time clock module.
static void rtc_isr(void)
{
  //Leave this blank
}


void MayflyDataLogger::setup(void)
{

  pinMode(RTC_PIN, INPUT_PULLUP);

  PcInt::attachInterrupt(RTC_PIN, rtc_isr);

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


static void print_two_digit(Print &stream, uint8_t value)
{
    stream.print(char((value / 10) + '0'));
    stream.print(char((value % 10) + '0'));
}

void MayflyDataLogger::write_timestamp(Print &stream)
{
  const DateTime timestamp = rtc.now();
  stream.print(timestamp.year());
  stream.print('-');
  print_two_digit(stream, timestamp.month());
  stream.print('-');
  print_two_digit(stream, timestamp.date());
  stream.print(' ');
  print_two_digit(stream, timestamp.hour());
  stream.print(':');
  print_two_digit(stream, timestamp.minute());
  stream.print(':');
  print_two_digit(stream, timestamp.second());
}



float MayflyDataLogger::rtc_temperature(void)
{
  return rtc.getTemperature();
}

} // namespace coweeta
