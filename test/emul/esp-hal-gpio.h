#ifndef ESP_HAL_GPIO_H
#define ESP_HAL_GPIO_H

#define LOW 0x0
#define HIGH 0x1

#define INPUT 0x01
// arduino-esp32 values; the emulated pinMode treats every input mode alike.
#define INPUT_PULLUP 0x05
#define INPUT_PULLDOWN 0x09
// Changed OUTPUT from 0x02 to behave the same as Arduino pinMode(pin,OUTPUT)
// where you can read the state of pin even when it is set as OUTPUT
#define OUTPUT 0x03

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

#endif
