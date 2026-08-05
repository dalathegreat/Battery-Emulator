#ifndef _TRANSMITTER_H
#define _TRANSMITTER_H

class Transmitter {
 protected:
  // Never deleted through the base - see Battery.h for the pattern rationale
  ~Transmitter() = default;

 public:
  virtual void transmit(unsigned long currentMillis) = 0;
};

void register_transmitter(Transmitter* transmitter);

#endif
