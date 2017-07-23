
// Data logger sketch to test for oscillations in the Coweeta 
// heater controller project
//

#include "data_logger.h"

using namespace coweeta;

static DataLogger logger(49, 51, 53);

// Define pinouts.
static const int enable_line = 31;
static const int diff_input_line = 7;
static const int high_input_line = 14;
static const int low_input_line = 15;

// Define memory sizes.
static const int SAMPLES = 250;
static const int BIN_BITS = 3;
static const int BINS = (1 << BIN_BITS) + 1;


// We have three events ...
enum {
  volt_read = 1,
  heat_on = 2,
  heat_off = 4
};


static const EventSchedule schedule[] = {
  event("volt_read", HMS(0, 0, 1)),
  event("heat_on", HMS(0, 1, 0)),
  event("heat_off", HMS(0, 1, 0), 30)
};



// Returns the index of the highest bit set in val. 
// So log2(8) == 3 as does log2(9) and log2(15).
// In the same way log2(16) == 4
// log2(0) returns 0
static uint8_t log2(uint16_t val)
{
  for (uint8_t ln = 0; ln < 16; ln++) {
    if (val == 0) {
      return ln;
    }
    val >>= 1;
  }
}


// A structure that lets us sample an input over a duration.
// From it we can get min, max and mean values of the variable,
// as well as a histogram of values.
class Record {
public:
  uint16_t min;
  uint16_t max;
  uint32_t sum;
  uint8_t bins[BINS];
  uint16_t vals[SAMPLES];

  void clear(void)
  {
    min = 1024;
    max = 0;
    sum = 0;
    for (uint8_t i = 0; i < BINS; i++) {
      bins[i] = 0;
    }
    
  }
  void log_value(uint8_t i, uint16_t val) 
  {
      sum += val;
      vals[i] = val;
      if (val < min) {
        min = val;
      }
      if (val > max) {
        max = val;
      }
  }

  void calc_stuff(void)
  {

    const uint16_t diff = max - min;
    const int8_t l2diff = log2(diff);
    const uint8_t l2step = (l2diff - BIN_BITS > 0) ? l2diff - BIN_BITS : 0;
    const uint16_t base = min >> l2step;
    for (uint8_t i = 0; i < BINS; i++) {
      bins[i] = 0;
    }
    for (uint8_t i = 0; i < SAMPLES; i++) {
      const uint8_t offset = (vals[i] >> l2step) - base;
      bins[offset]++;
    }
  }
  
};


void setup() {
  logger.setup();
  logger.set_schedule(schedule, 3);
  pinMode(enable_line, OUTPUT);
}



void loop()
{
  // Have one recorder for each of the high and low analog inputs; and
  // one for the difference input.
  static Record rec[3];


  logger.wait_for_event();

  // Turn the heater on or off
  if (logger.is_event(heat_on)) {
    digitalWrite(enable_line, HIGH);
  }
  if (logger.is_event(heat_off)) {
    digitalWrite(enable_line, LOW);
  }

  
  if (logger.is_event(volt_read)) {
    for (uint8_t r = 0; r < 3; r++) {
      rec[r].clear();
    }
    for (uint8_t i = 0; i < SAMPLES; i++) {
      rec[0].log_value(i, analogRead(diff_input_line));
      rec[1].log_value(i, analogRead(high_input_line));
      rec[2].log_value(i, analogRead(low_input_line));
      delay(2);
    }
    
    logger.new_log_line();
    
    for (uint8_t r = 0; r < 3; r++) {
      rec[r].calc_stuff();
      logger.log_int(rec[r].min);
      logger.log_int(rec[r].max);
      logger.log_int(rec[r].sum / SAMPLES);
      for (uint8_t i = 0; i < BINS; i++) {
        logger.log_int(rec[r].bins[i]);
      }
    }
  }
  
  logger.end_log_line();
}

