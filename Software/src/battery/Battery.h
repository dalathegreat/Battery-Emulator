#ifndef BATTERY_H
#define BATTERY_H

// Abstract base class for next-generation battery implementations.
// Defines the interface to call battery specific functionality.
class Battery {
 public:
  virtual void setup(void) = 0;
  virtual void update_values() = 0;
};

#endif
