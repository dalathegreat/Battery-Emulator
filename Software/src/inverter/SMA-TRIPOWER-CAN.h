#ifndef SMA_CAN_TRIPOWER_H
#define SMA_CAN_TRIPOWER_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

extern uint16_t system_scaled_SOC_pptt;      //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;        //SOC%, 0-100.00 (0-10000)
extern uint8_t system_number_of_cells;       //Total number of cell voltages, set by each battery
extern bool batteryAllowsContactorClosing;   //Bool, true/false
extern bool inverterAllowsContactorClosing;  //Bool, true/false

void update_values_can_sma_tripower();
void send_can_sma_tripower();
void receive_can_sma_tripower(CAN_frame_t rx_frame);
void send_tripower_init();

#endif
