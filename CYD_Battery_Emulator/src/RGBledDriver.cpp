#include <Arduino.h>

// For RGB LED
#define RED_LED_PIN 4
#define GREEN_LED_PIN 16
#define BLUE_LED_PIN 17



uint8_t getRedValueFromColor(uint32_t c) {
    return (c >> 16);
}

uint8_t getGreenValueFromColor(uint32_t c) {
    return (c >> 8);
}

uint8_t getBlueValueFromColor(uint32_t c) {
    return (c);
}

void setColor(uint8_t R, uint8_t G, uint8_t B)
{
    // Note: LEDs are "active low", meaning HIGH == off, LOW == on
    analogWrite(RED_LED_PIN, 255 -R) ;
    analogWrite(GREEN_LED_PIN, 255 - G);
    analogWrite(BLUE_LED_PIN, 255 - B);
}

void ChangeRGBColor(uint32_t color)
{
    setColor(getRedValueFromColor(color), getGreenValueFromColor(color), getBlueValueFromColor(color));
}


void initRGBled()
{
    // set all three pins to output mode
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
}