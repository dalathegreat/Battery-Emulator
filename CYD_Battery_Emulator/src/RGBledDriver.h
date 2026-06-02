#ifndef _RGB_LED_DRIVER_H_
#define _RGB_LED_DRIVER_H_

void ChangeRGBColor(uint32_t color);  // uses 32-bit color code such as 0xFFd251
void setColor(uint8_t R, uint8_t G, uint8_t B); // uses individual 8-bit values for R, G, and B
void initRGBled() ;

#define RGB_COLOR_1 0xFF0000
#define RGB_COLOR_2 0x00FF00
#define RGB_COLOR_3 0x0000FF
#define RGB_COLOR_4 0xFF00ff
#define RGB_COLOR_5 0x00FFff

#endif // _RGB_LED_DRIVER_H_