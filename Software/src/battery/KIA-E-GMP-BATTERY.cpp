#include "BATTERIES.h"
#ifdef KIA_E_GMP_BATTERY
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "KIA-E-GMP-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10ms = 0;  // will store last time a 10s CAN Message was send
static const int interval100 = 100;           // interval (ms) at which send CAN Messages
static const int interval10ms = 10;           // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;            //counter for checking if CAN is still alive

#define MAX_CELL_VOLTAGE 4250   //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950   //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION 150  //LED turns yellow on the board if mv delta exceeds this value



CAN_frame_t KIA_HYUNDAI_200 = {.FIR = {.B =
                                           {
                                               .DLC = 8,
                                               .FF = CAN_frame_std,
                                           }},
                               .MsgID = 0x200,
                               .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};  //Mid log value

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  system_real_SOC_pptt;

  system_SOH_pptt;

  system_battery_voltage_dV;

  system_battery_current_dA;

  system_capacity_Wh = BATTERY_WH_MAX;

  system_remaining_capacity_Wh = static_cast<int>((static_cast<double>(system_real_SOC_pptt) / 10000) * BATTERY_WH_MAX);

  system_max_charge_power_W;

  system_max_discharge_power_W;

  system_active_power_W = ((system_battery_voltage_dV * system_battery_current_dA) / 100);

  system_temperature_min_dC;

  system_temperature_max_dC;

  system_cell_max_voltage_mV;

  system_cell_min_voltage_mV;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }
  */

  if (system_bms_status == FAULT) {  //Incase we enter a critical fault state, zero out the allowed limits
    system_max_charge_power_W = 0;
    system_max_discharge_power_W = 0;
  }

  /* Safeties verified. Perform USB serial printout if configured to do so */

#ifdef DEBUG_VIA_USB
  Serial.println();
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0xABC:
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  //Send 100ms message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= interval10ms) {
    previousMillis10ms = currentMillis;

    //ESP32Can.CANWriteFrame(&KIA_HYUNDAI_200);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  Serial.println("Hyundai E-GMP (Electric Global Modular Platform) battery selected");

  system_max_design_voltage_dV = 8060;  // 806.0V, over this, charging is not possible (goes into forced discharge)
  system_min_design_voltage_dV = 6700;  // 670.0V under this, discharging further is disabled
}

#endif
