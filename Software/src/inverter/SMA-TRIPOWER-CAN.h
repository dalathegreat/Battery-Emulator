#ifndef SMA_CAN_TRIPOWER_H
#define SMA_CAN_TRIPOWER_H
#include <Arduino.h>
#include "../include.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define INVERTER_SELECTED

extern uint32_t system_capacity_Wh;                        //Wh,  0-500000Wh
extern uint32_t system_remaining_capacity_Wh;              //Wh,  0-500000Wh
extern int16_t system_temperature_min_dC;                  //C+1, -50.0 - 50.0
extern int16_t system_temperature_max_dC;                  //C+1, -50.0 - 50.0
extern int16_t system_battery_current_dA;                  //A+1, -1000 - 1000
extern uint16_t system_battery_voltage_dV;                 //V+1,  0-1000.0 (0-10000)
extern uint16_t system_max_design_voltage_dV;              //V+1,  0-1000.0 (0-10000)
extern uint16_t system_min_design_voltage_dV;              //V+1,  0-1000.0 (0-10000)
extern uint16_t system_scaled_SOC_pptt;                    //SOC%, 0-100.00 (0-10000)
extern uint16_t system_real_SOC_pptt;                      //SOC%, 0-100.00 (0-10000)
extern uint32_t system_max_discharge_power_W;              //W,    0-200000
extern uint32_t system_max_charge_power_W;                 //W,    0-200000
extern uint16_t system_cell_max_voltage_mV;                //mV, 0-5000, Stores the highest cell millivolt value
extern uint16_t system_cell_min_voltage_mV;                //mV, 0-5000, Stores the minimum cell millivolt value
extern uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages in mV
extern uint8_t system_number_of_cells;                     //Total number of cell voltages, set by each battery
extern uint8_t system_bms_status;                          //Enum 0-5
extern bool batteryAllowsContactorClosing;                 //Bool, true/false
extern bool inverterAllowsContactorClosing;                //Bool, true/false

void update_values_can_sma_tripower();
void send_can_sma_tripower();
void receive_can_sma_tripower(CAN_frame_t rx_frame);
void send_tripower_init();

#endif
