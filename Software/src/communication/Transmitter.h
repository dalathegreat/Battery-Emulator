#ifndef _TRANSMITTER_H
#define _TRANSMITTER_H

class Transmitter {
 public:
  virtual ~Transmitter() = default;

  virtual void transmit(unsigned long currentMillis) = 0;
};

void register_transmitter(Transmitter* transmitter);

#endif
