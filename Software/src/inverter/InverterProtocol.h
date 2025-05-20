#ifndef INVERTER_PROTOCOL_H
#define INVERTER_PROTOCOL_H

// The abstract base class for all inverter protocols
class InverterProtocol {
 public:
  virtual void setup() = 0;
  virtual const char* interface_name() = 0;

  // This function maps all the values fetched from battery to the correct battery emulator data structures
  virtual void update_values() = 0;
};

#endif
