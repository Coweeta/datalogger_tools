//
// Multi_Event: an example of using the Coweeta datalogger framework.
//
// This is one step up from the simple.ino example.
// We read and log from multiple sensors at a variety of rates.  We also drive
// some control signals.  And we record data not just as integers but also
// strings and floating point values.
//
// This example assumes some application where we have a heater circuit
// controlled by a pin and are measured some response to that.
//
// Coweeta Hydrology Laboratory, Southern Research Station, US Forest Service

#include "data_logger.h"


// Spare ourselves from having to prepend "coweeta::" to any the entities we use
// from  data_logger.h
using namespace coweeta;

// Declare our data logger object.  We will set it up later.
static DataLogger logger;

// We are going to read from three sensors.  The alfa sensors are read together,
// the bravo sensor is read at a different rate, following several seconds after
// a control signal (heater_enable) being sent.
static const int sensor_alfa_1 = A1;
static const int sensor_alfa_2 = A2;
static const int sensor_bravo = A3;

// The microcontroller pin that we set high to turn on a heater element.
static const int heater_enable = 11;

// Declare some handles for the events we will generate.  These must map
// one-to-one with the events in our event schedule below.  The values of these
// constants must be sequential powers of two.  The rationale behind this is
// that multiple events can be coincident - and we can test for combinations of
// event by calling logger.is_event(alfa_read | bravo_read_1) for example.
// Read the "|" character as "or" here.
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
  event(HMS(0, 1, 0)),  // alfa_read
  event(HMS(0, 5, 0)),  // bravo_read_1
  event(HMS(0, 5, 0), 10),  // bravo_read_2
  event(HMS(0, 5, 0), -10) // bravo_energize
};


// Standard Arduino function called on start up.
void setup() {

  logger.setup();

  // Specify the schedule of events to follow.  In this case we use the one
  // defined above, and we state that there is just the one event type.
  logger.set_schedule(schedule, 4);


}


// Just to demonstrate how one might log text values get_change() returns a
// string that indicates how the voltage at sensor_alfa_2 has changed since
// it was last checked.  It will return "up", "down" or "steady".
static const char* get_change(void) {
  static uint16_t previous = 0;
  const char *change;
  const uint16_t current = analogRead(sensor_alfa_2);
  if (current < previous) {
    change = "down";
  } else if (current > previous) {
    change = "up";
  } else {
    change = "steady";
  }
  previous = current;
  return change;
}


//
// The loop() function is run repeatedly and ceaselessly once setup() has
// completed.
//
// It is in this function that all of the logging occurs.
//
void loop() {
  // First we wait until it is time to do something or until a human operator
  // has requested something happens.  In this case this would be at the start
  // of each minute.
  logger.wait_for_event();

  // We only want to log data for the XXX_read events, not for the
  // bravo_energize event.  So here we check if it is time for one - both - of
  // them.
  if (logger.is_event(alfa_read | bravo_read_1 | bravo_read_2)) {
    // So, we are logging something; start a new line.
    logger.new_log_line();

    // Check if this is an "alfa read" event.  If so log the two measured
    // values.  One is recorded as a float (with some arbitrary offset and
    // scale), the other as a string.  If we are not reading the alfa sensors
    // then add in some blank entries in there place.
    if (logger.is_event(alfa_read)) {
      const float value1 = (analogRead(sensor_alfa_1) - 123) / 17.0;
      logger.log_float(value1, 3);

      logger.log_string(get_change());
    } else {
      logger.skip_entries(2);
    }

    // Check if this one of the "bravo read" event and, if so, record the bravo
    // sensor value.
    if (logger.is_event(bravo_read_1 | bravo_read_2)) {
      const int value = analogRead(sensor_bravo);
      logger.log_int(value);
    }

    // Having logged anything that needed logging, we can terminate the line.
    logger.end_log_line();
  }

  // The "bravo energize" event doesn't involve recording anything, just
  // driving some lines.  So we don't need to involve the logger.  Turn the
  // heater on, wait half a sec and turn it off.
  if (logger.is_event(bravo_energize)) {
    digitalWrite(heater_enable, HIGH);
    delay(500);
    digitalWrite(heater_enable, LOW);
  }

}



