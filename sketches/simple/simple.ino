#include "data_logger.h"

static datalogger::DataLogger dl;


void setup() {

    const datalogger::EventSchedule schedule[] = {
        {0, datalogger::interval(0, 0, 30), datalogger::interval(0, 0, 0)},
        {0, datalogger::interval(0, 0, 20), datalogger::interval(0, 0, 0)}
    };
    
    dl.setup();

    dl.set_schedule(schedule, 2);


}

/*
Each event lasts an instant.


*/
void loop() {
    dl.wait_for_event();

    dl.new_log_line();

    if (dl.is_event(0)) {
        dl.log_int(123);
    } else {
        dl.add_delimiters(1);
    }

    if (dl.is_event(1)) {
        dl.log_int(456);
    }

   
}


