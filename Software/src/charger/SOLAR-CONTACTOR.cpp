#include "../include.h"
#ifdef SOLAR_CONTACTOR
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "NISSAN-LEAF-CHARGER.h"

/* This implements Nissan LEAF PDM charger support. 2013-2024 Gen2/3 PDMs are supported
 *
 * This code is intended to facilitate standalone battery charging, 
 * for instance for troubleshooting purposes or emergency generator usage
 *
 * Credits go to:
 *   Damien Maguire (evmbw.org, https://github.com/damienmaguire/Stm32-vcu)
 *
 * The code functions a bit differently incase a Nissan LEAF battery is used. Almost
 * all CAN messages are already sent from the battery in that case, so the charger
 * implementation can focus on only controlling the charger, spoofing battery messages
 * is not needed.
 *
 * Incase another battery type is used, the code ofcourse emulates a complete Nissan LEAF
 * battery onto the CAN bus. 
*/

static uint8_t charger_max_voltage = 111.0;
static uint8_t charger_max_soc = 95; 
static uint8_t charger_min_soc = 20; 
static uint8_t charger_current_voltage = 0;
static uint8_t charger_current_soc = 0;
static bool solar_contactor_enabled = 0;


void receive_can_charger(CAN_frame_t rx_frame) {
  
}

void send_can_charger() {

  //Update Charge Limits
  charger_max_voltage = datalayer.battery.info.max_design_voltage_dV;
  charger_max_soc = 90;
  charger_min_soc = 20;
  charger_current_voltage = datalayer.battery.status.voltage_dV;
  charger_current_soc = datalayer.battery.status.real_soc;

  // Serial.println(String(charger_max_voltage) + "   " + String(charger_max_soc) + "   " + String(charger_min_soc) + "   "  + String(charger_current_voltage) + "   "  + String(charger_current_soc) + "   " );

  // if ((charger_current_voltage <= charger_max_voltage) && (charger_current_soc <= charger_max_soc) && (charger_current_soc >= charger_min_soc ) ){
  if ((charger_current_soc <= charger_max_soc) && (charger_current_soc >= charger_min_soc ) ){
    Serial.println("Solar Contactor Enabled - SOC: " + String(charger_current_soc) + "  Voltage: "  + String(charger_current_soc));
    digitalWrite(POSITIVE_CONTACTOR_PIN, HIGH);
    datalayer.system.status.solar_allows_contactor_closing = true;
  }
  else{
    Serial.println("Solar Contactor Disabled - SoC:" + String(charger_current_soc)  );
    digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
    datalayer.system.status.solar_allows_contactor_closing = false;
  }

}
#endif
