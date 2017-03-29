//
// Simple: a very basic example of using the Coweeta datalogger framework.
//
// We read and log a single analog sensor every minute, on the minute.
//
// The comments are aimed at someone new to both Arduino and the data_logger
// library.
//
// Coweeta Hydrology Laboratory, Southern Research Station, US Forest Service


// This file contains the definitions we need for the framework.
#include "data_logger.h"


// Spare ourselves from having to prepend "coweeta::" to any the entities we use
// from  data_logger.h  You will usually keep this line in your code unless your
// program is big and includes other libraries with clashing variable names.
using namespace coweeta;

// Declare our data logger object.  Inside this beastie is hidden all the fiddly
// functionality like timer and file management. We will set it up later.
static DataLogger logger;

// Our simple example involves us reading from a single analog input and logging
// the raw value found.  Here, we declare the pin we are reading from.
static const int sensor_pin = A0;

// Declare an array of structures that define what our schedule of events is.
// In this example, we have a single event that will occur at the start of each
// minute.  The HMS() function is just a convenience function that converts a
// duration given in hours, minutes and seconds to the equivalent number of
// seconds.
static const EventSchedule schedule[] = {
    event("main", HMS(0, 1, 0))
};


// The setup() function (every Arduino program has one) is run once at power-on/
// reset.  In it we prepare our logger object and perform any setup the rest of
// our program would need.  None is required in this example.
void setup() {

    // Always call this; it starts up communications links, timers and the like.
    logger.setup();

    // Specify the schedule of events to follow.  In this case we use the one
    // defined above, and we state that there is just the one event type.
    logger.set_schedule(schedule, 1);


}


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

    // Perform our reading, in this case we are simply reading a single voltage
    // on an analog pin on the Arduino board.
    const int value = analogRead(sensor_pin);

    // Start a new entry to record the details on.  This line will start with
    // just the timestamp included: for example "2017-02-25 22:53:00"
    logger.new_log_line();

    // Now log our reading.  For example if it is 123, then the log line will
    // now be: "2017-02-25 22:53:00,123"
    logger.log_int(value);

    // Having recorded everything we needed for this time, we terminate it.
    // If this is a normal, scheduled event the line is written to the currently
    // active log file.  If however, the event is triggered from the serial
    // interface - i.e. the management program being driven by a human operator
    // - then the line text is sent over that interface.
    logger.end_log_line();

    // And we are done.  We return out of loop(), only to immediately enter it
    // again.

}



