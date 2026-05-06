#include "BOLT-AMPERA-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include <Arduino.h>

/*
TODOs left for this implementation
- The battery has 3 CAN ports. One of the internal modules is responsible for the 7E4 polls, the battery for the 7E7 polls
- Current implementation only seems to get the 7E7 polls working.

- The values missing for a fully working implementation is:
- SOC% missing! (now estimated based on voltage)
- Capacity (kWh) (now estimated)
- Charge max power (now estimated)
- Discharge max power (now estimated)
- Current is updating extremely slow, consider switching to sensed_ value
- Balancing info seems to be available via OBD/ISO-TP service $22OBD/ISO-TP service $22 , 4340 and onwards
*/

/*TODO, messages we might need to send towards the battery to keep it happy and close contactors
0x214 Charger coolant temp info HV
0x20E Hybrid balancing request HV
0x30E High Voltage Charger Command HV
0x30C HVEM Provide Charging HV
0x316 OBHV Charge Process PEV HV
0x30F OBHV Charg Statn Current stat HV
0x312 OBHV Charg Statns Energy allocation HV
0x310 OBHV Charg Statn Vlt Energy Power HV
0x306 Off board HVCS Limit HV
0x309 Off board HVCS Min Limit HV
0x305 Vehicle Charging limit stat HV
0x314 Vehicle req energy transfer HV <<<<<<<<<< Sounds like contactor request resides here TODO
0x460 Energy Storage System Temp HV (Who sends this? Battery?)
*/

// Define the data points for %SOC depending on cell voltage
// NCM Discharge Curve Lookup Table (100 points, 4.2V-3.0V)
// SOC[100] = State of Charge (0.01% units, e.g., 10000 = 100.00%)
// voltage_lookup[100] = Pack voltage (mV)
const uint8_t numEntries = 100;
const uint16_t SOC[100] = {
    10000, 9985, 9970, 9955, 9940, 9925, 9910, 9895, 9880, 9865,  // 4.20V - 4.15V (High plateau)
    9850,  9820, 9790, 9760, 9730, 9700, 9660, 9620, 9580, 9540,  // 4.14V - 4.00V
    9500,  9450, 9400, 9350, 9300, 9250, 9200, 9150, 9100, 9050,  // 3.99V - 3.90V
    9000,  8900, 8800, 8700, 8600, 8500, 8400, 8300, 8200, 8100,  // 3.89V - 3.80V
    8000,  7850, 7700, 7550, 7400, 7250, 7100, 6950, 6800, 6650,  // 3.79V - 3.70V
    6500,  6300, 6100, 5900, 5700, 5500, 5300, 5100, 4900, 4700,  // 3.69V - 3.60V
    4500,  4300, 4100, 3900, 3700, 3500, 3300, 3100, 2900, 2700,  // 3.59V - 3.50V
    2500,  2250, 2000, 1750, 1500, 1250, 1000, 800,  600,  400,   // 3.49V - 3.40V (Steep drop)
    300,   200,  150,  100,  80,   60,   40,   30,   20,   10,    // 3.39V - 3.30V
    5,     2,    1,    0,    0,    0,    0,    0,    0,    0      // <3.30V (Cutoff)
};

const uint16_t voltage_lookup[100] = {
    4200, 4195, 4190, 4185, 4180, 4175, 4170, 4165, 4160, 4155,  // High plateau
    4150, 4140, 4130, 4120, 4110, 4100, 4090, 4080, 4070, 4060, 4050, 4040, 4030, 4020, 4010, 4000, 3990, 3980,
    3970, 3960, 3950, 3940, 3930, 3920, 3910, 3900, 3890, 3880, 3870, 3860, 3850, 3840, 3830, 3820, 3810, 3800,
    3790, 3780, 3770, 3760, 3750, 3740, 3730, 3720, 3710, 3700, 3690, 3680, 3670, 3660, 3650, 3640, 3630, 3620,
    3610, 3600, 3590, 3580, 3570, 3560, 3550, 3540, 3530, 3520, 3510, 3500, 3490, 3480, 3470, 3460, 3450, 3440,
    3430, 3420, 3410, 3400, 3390, 3380, 3370, 3360, 3350, 3340, 3330, 3320, 3310, 3300, 3290, 3280, 3270, 3260};

static uint16_t estimateSOC(uint16_t cellVoltage) {  // Linear interpolation function
  if (cellVoltage >= voltage_lookup[0]) {
    return SOC[0];
  }
  if (cellVoltage <= voltage_lookup[numEntries - 1]) {
    return SOC[numEntries - 1];
  }

  for (int i = 1; i < numEntries; ++i) {
    if (cellVoltage >= voltage_lookup[i]) {
      float t = (cellVoltage - voltage_lookup[i]) / (voltage_lookup[i - 1] - voltage_lookup[i]);
      return SOC[i] + t * (SOC[i - 1] - SOC[i]);
    }
  }
  return 0;  // Default return for safety, should never reach here
}

void BoltAmperaBattery::update_values() {
  // 1. CALCULATE PERFECT TOTAL AND MIN/MAX
  uint32_t total_mV = 0;
  uint16_t actual_max_mV = 0;
  uint16_t actual_min_mV = 5000;

  for (int i = 0; i < 96; i++) {
      uint16_t cv = cellblock_voltage[i];
      total_mV += cv;
      if (cv > actual_max_mV) actual_max_mV = cv;
      if (cv > 0 && cv < actual_min_mV) actual_min_mV = cv; // cv > 0 ignores empty cells
  }

  // 2. LINK TO MAIN DISPLAY
  if (total_mV > 0) {
      datalayer_battery->status.voltage_dV = (uint16_t)(total_mV / 100); // Total of 96 cells
      datalayer_battery->status.cell_max_voltage_mV = actual_max_mV;      
      datalayer_battery->status.cell_min_voltage_mV = actual_min_mV;      
  } else {
      // Fallback in case of missing cell data
      datalayer_battery->status.voltage_dV = battery_voltage_periodic_dV;
      datalayer_battery->status.cell_max_voltage_mV = battery_cell_voltage_max_mV;
      datalayer_battery->status.cell_min_voltage_mV = battery_cell_voltage_min_mV;
  }
  
  // 3. SOC, CURRENT AND CAPACITY
  datalayer_battery->status.real_soc = estimateSOC(actual_max_mV); // Uses highest cell to safely stop charging
  datalayer_battery->status.reported_soc = datalayer_battery->status.real_soc;
  
  // Current (Adapted to Dala's original ID 0x216 and factor 0.2)
  datalayer_battery->status.current_dA = (int16_t)(sensed_current_sensor_1 * 0.2);

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);
  datalayer_battery->status.soh_pptt = 9900;


  // ==============================================================================
  // --- ABSOLUTE DEADMAN'S SWITCH (Voltage & Temperature) ---
  // ==============================================================================
  
  // A. VOLTAGE: 100% in car is max 4.16V. Above 4.20V is critical! Below 3.20V is critical!
  // Debounce included: only triggers if voltage is measured too high multiple times.
  static uint8_t voltage_alarm_counter = 0;
  static bool critical_voltage_alarm = false;

  if (actual_max_mV >= 4200 || actual_min_mV <= 3200) {
      voltage_alarm_counter++;
      if (voltage_alarm_counter >= 5) { 
          critical_voltage_alarm = true;
          Serial.println("!!! DEADMAN'S SWITCH: Critical voltage reached! Current limited to 0A !!!");
      }
  } else {
      voltage_alarm_counter = 0;
      critical_voltage_alarm = false; // Value is safe again
  }

  // B. THERMAL RUNAWAY: Detect rapid temp rise PER SENSOR (3 degrees / 10 sec)
  static float previous_temps[6] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0}; 
  static unsigned long last_temp_check = millis();

  // Put the current 6 sensors in a convenient array
  float current_temps[6] = {
      battery_module_temp_1, battery_module_temp_2, battery_module_temp_3, 
      battery_module_temp_4, battery_module_temp_5, battery_module_temp_6
  };

  // 1. First time fill (on startup)
  if (previous_temps[0] == -100.0 && current_temps[0] > -40.0) {
      for (int i = 0; i < 6; i++) { previous_temps[i] = current_temps[i]; }
  } 
  // 2. Check all 6 sensors every 10 seconds
  else if (millis() - last_temp_check >= 10000) {
      for (int i = 0; i < 6; i++) {
          float temp_rise = current_temps[i] - previous_temps[i];
          
          // If ANY of the 6 sensors rises by 3 degrees: EMERGENCY STOP!
          if (temp_rise >= 3.0) {
              while(true) { delay(1000); } // System frozen!
          }
          
          // Update memory for next cycle
          previous_temps[i] = current_temps[i];
      }
      last_temp_check = millis();
  }
  // ==============================================================================
  // 4. RELEASE INVERTER POWER (WITH SAFETY LIMITS!)
  // Physical DC fuse is 40A. Software limit set to 35A to prevent melting.
  uint16_t max_discharge_A = 350;  // 35.0 Amps allowed
  uint16_t max_charge_A = 350;     // 35.0 Amps allowed
  
  // 35A * ~350V = ~12,250W. Safe limit set to 11 kW.
  uint16_t max_discharge_W = 11000;
  uint16_t max_charge_W = 11000;

  // --- BOTTOM PROTECTION (Prevent deep discharge) ---
  if (actual_min_mV < 3400) { 
      max_discharge_A = 100;  // Throttle down to 10.0 A
      max_discharge_W = 3000; 
  }
  if (actual_min_mV < 3200) { 
      max_discharge_A = 0;    // BATTERY EMPTY: 0.0 A discharge allowed
      max_discharge_W = 0;
  }

  // --- ACTIVATE DEADMAN'S SWITCH VOLTAGE ---
  // Placed at the bottom to OVERRIDE EVERYTHING in case of critical alarm.
  if (critical_voltage_alarm == true) {
      max_discharge_A = 0;
      max_charge_A = 0;
      max_discharge_W = 0;
      max_charge_W = 0;
  }

  /// --- TOP PROTECTION (Prevent overcharging highest cell) ---
  // Using 'static' so LilyGo remembers it hit the limit
  static bool charging_blocked = false;

  // 1. When do we enable or disable the block?
  if (actual_max_mV >= 4150) { 
      charging_blocked = true;   // Flag ON: That's enough, stop charger.
  } else if (actual_max_mV < 4100) { 
      charging_blocked = false;  // Flag OFF: Dropped far enough, charging allowed again.
  }

  // 2. What do we do with the current?
  if (charging_blocked == true) { 
      max_charge_A = 0;       // EMERGENCY STOP 0.0 A
      max_charge_W = 0;
  } else if (actual_max_mV > 4050) { 
      max_charge_A = 50;      // Throttle down to 5.0 A
      max_charge_W = 2000;
  }
  
  // --- TEMPERATURE PROTECTION (Prevent heat/cold damage) ---
  
  // 1. COLD (Look at coldest sensor)
  if (temperature_lowest_C < 5.0) { 
      // Below 5 degrees C: Charge very slowly to prevent damage
      // Note: using '>' so a limit already at 0 isn't accidentally set back to 50!
      if (max_charge_A > 50) { max_charge_A = 50; max_charge_W = 2000; }
  }
  if (temperature_lowest_C < 0.0) { 
      // Below freezing: NEVER CHARGE! (Careful discharging is allowed)
      max_charge_A = 0; 
      max_charge_W = 0;
      // Throttle discharging slightly just to be safe
      if (max_discharge_A > 200) { max_discharge_A = 200; max_discharge_W = 8000; }
  }

  // 2. HEAT (Look at warmest sensor)
  if (temperature_highest_C > 45.0) { 
      // Above 45 degrees C: Battery must cool down, throttle charge/discharge
      if (max_charge_A > 100) { max_charge_A = 100; max_charge_W = 4000; }
      if (max_discharge_A > 100) { max_discharge_A = 100; max_discharge_W = 4000; }
  }
  if (temperature_highest_C >= 55.0) { 
      // Above 55 degrees C: DANGER OF OVERHEATING! Full EMERGENCY STOP!
      max_charge_A = 0; max_charge_W = 0;
      max_discharge_A = 0; max_discharge_W = 0;
  }
  
  // Send safe values to inverter:
  datalayer_battery->status.max_discharge_power_W = max_discharge_W;
  datalayer_battery->status.max_charge_power_W = max_charge_W;
  datalayer_battery->status.max_discharge_current_dA = max_discharge_A;
  datalayer_battery->status.max_charge_current_dA = max_charge_A;
  datalayer_battery->status.bms_status = ACTIVE;

  // 5. TEMPERATURES
  datalayer_battery->status.temperature_min_dC = (int16_t)(temperature_lowest_C * 10);
  datalayer_battery->status.temperature_max_dC = (int16_t)(temperature_highest_C * 10);

  // 6. TRANSFER CELLS FOR WEB INTERFACE
  memcpy(datalayer_battery->status.cell_voltages_mV, cellblock_voltage, 96 * sizeof(uint16_t));

  if (datalayer_boltampera) {
    datalayer_boltampera->battery_module_temp_1 = battery_module_temp_1;
    datalayer_boltampera->battery_module_temp_2 = battery_module_temp_2;
    datalayer_boltampera->battery_module_temp_3 = battery_module_temp_3;
    datalayer_boltampera->battery_module_temp_4 = battery_module_temp_4;
    datalayer_boltampera->battery_module_temp_5 = battery_module_temp_5;
    datalayer_boltampera->battery_module_temp_6 = battery_module_temp_6;
    datalayer_boltampera->battery_max_temperature = (int16_t)temperature_highest_C;
    datalayer_boltampera->battery_min_temperature = (int16_t)temperature_lowest_C;
    datalayer_boltampera->battery_5V_ref = battery_5V_ref;
    datalayer_boltampera->battery_cell_average_voltage = battery_cell_average_voltage;
    datalayer_boltampera->battery_terminal_voltage = battery_terminal_voltage;
    datalayer_boltampera->battery_SOC_display = battery_SOC_display;
    datalayer_boltampera->battery_isolation_kohm = battery_isolation_kohm;
    datalayer_boltampera->battery_HV_locked = battery_HV_locked;
    datalayer_boltampera->battery_crash_event = battery_crash_event;
    datalayer_boltampera->battery_HVIL = battery_HVIL;
    datalayer_boltampera->battery_HVIL_status = battery_HVIL_status;
    datalayer_boltampera->battery_current_7E4 = battery_current_7E4;
  }
  
}

void BoltAmperaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // SNOOP MODE: Shows EVERY ID caught by LilyGo
  // If enabled, prints a lot, but confirms if 3E3 arrives
  // Serial.printf("Received ID: 0x%03X\n", rx_frame.ID); 

  uint8_t cellbank_mux = 0;
  uint8_t cellblock_index = 0;

  switch (rx_frame.ID) {
    case 0x200:
    case 0x202:
    case 0x204:
    case 0x206:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);
      if(rx_frame.ID == 0x200) cellblock_index = cellbank_mux * 3;
      if(rx_frame.ID == 0x202) cellblock_index = 24 + (cellbank_mux * 3);
      if(rx_frame.ID == 0x204) cellblock_index = 48 + (cellbank_mux * 3);
      if(rx_frame.ID == 0x206) cellblock_index = 72 + (cellbank_mux * 3);
      
      cellblock_voltage[cellblock_index] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 1] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 2] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25f;
      break;

    case 0x20C:
      battery_isolation_kohm = (rx_frame.data.u8[1] * 25);
      battery_cell_voltage_max_mV = (rx_frame.data.u8[4] * 20);
      battery_cell_voltage_min_mV = (rx_frame.data.u8[5] * 20);
      break;

    case 0x216: 
      // Dala's original channel for Ampera current meter
      sensed_current_sensor_1 = (int16_t)((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
      break;

    case 0x2C7:
      battery_voltage_periodic_dV = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4)) * 1.25;
      break;

    case 0x3E3:  // THIS MUST WORK
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperature_highest_C = (rx_frame.data.u8[2] / 2.0f) - 40.0f;
      temperature_lowest_C = (rx_frame.data.u8[4] / 2.0f) - 40.0f;
      // ALWAYS print this to Serial Monitor
      Serial.printf("!!! 3E3 FOUND !!! Max: %.1f, Min: %.1f\n", (float)temperature_highest_C, (float)temperature_lowest_C);
      break;

    case 0x7EC: // Reply from VICM
      //if (rx_frame.data.u8[0] == 0x10) transmit_can_frame(&BOLT_ACK_7E4);
      reply_poll_7E4 = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;

    case 0x7EF: // Reply from VITM
      //if (rx_frame.data.u8[0] == 0x10) transmit_can_frame(&BOLT_ACK_7E7);
      reply_poll_7E7 = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      if (reply_poll_7E7 == POLL_7E7_MODULE_TEMP_1) battery_module_temp_1 = (rx_frame.data.u8[4] / 2.0f) - 40;
      if (reply_poll_7E7 == POLL_7E7_MODULE_TEMP_2) battery_module_temp_2 = (rx_frame.data.u8[4] / 2.0f) - 40;
      if (reply_poll_7E7 == POLL_7E7_MODULE_TEMP_3) battery_module_temp_3 = (rx_frame.data.u8[4] / 2.0f) - 40;
      if (reply_poll_7E7 == POLL_7E7_MODULE_TEMP_4) battery_module_temp_4 = (rx_frame.data.u8[4] / 2.0f) - 40;
      if (reply_poll_7E7 == POLL_7E7_MODULE_TEMP_5) battery_module_temp_5 = (rx_frame.data.u8[4] / 2.0f) - 40;
      if (reply_poll_7E7 == POLL_7E7_MODULE_TEMP_6) battery_module_temp_6 = (rx_frame.data.u8[4] / 2.0f) - 40;
      break;

    default:
      break;
  }
  
  // TEMPERATURE FROM CAN B (Message 0x302)
    if (rx_frame.ID == 0x302) {
        // Convert the 6 modules: (value / 2) - 40
        float t1 = (rx_frame.data.u8[1] / 2.0f) - 40.0f;
        float t2 = (rx_frame.data.u8[2] / 2.0f) - 40.0f;
        float t3 = (rx_frame.data.u8[3] / 2.0f) - 40.0f;
        float t4 = (rx_frame.data.u8[4] / 2.0f) - 40.0f;
        float t5 = (rx_frame.data.u8[5] / 2.0f) - 40.0f;
        float t6 = (rx_frame.data.u8[6] / 2.0f) - 40.0f;

        // Determine highest (max) and lowest (min) temperature
        float max_t = t1; 
        float min_t = t1;
        
        if (t2 > max_t) { max_t = t2; } 
        if (t2 < min_t) { min_t = t2; }
        if (t3 > max_t) { max_t = t3; } 
        if (t3 < min_t) { min_t = t3; }
        if (t4 > max_t) { max_t = t4; } 
        if (t4 < min_t) { min_t = t4; }
        if (t5 > max_t) { max_t = t5; } 
        if (t5 < min_t) { min_t = t5; }
        if (t6 > max_t) { max_t = t6; } 
        if (t6 < min_t) { min_t = t6; }
        
        // Link to LilyGo datalayer
        this->temperature_highest_C = max_t;
        this->temperature_lowest_C = min_t;
    }
}

void BoltAmperaBattery::calculateBalancing() {
    uint16_t minVoltage = 65000; 
    uint16_t threshold = 180; // Threshold of 180 mV is fine for now
    
    // Step 1: Find the lowest cell
    for (int i = 0; i < 96; i++) {
        if (cellblock_voltage[i] > 0 && cellblock_voltage[i] < minVoltage) { 
            minVoltage = cellblock_voltage[i];
        }
    }

    // Step 2: Clear mask
    memset(balanceMask, 0, 12);

    // --- NEW: 1-in-3 Rotation pattern (Safety against overheating) ---
    static uint8_t balance_phase = 0; // Can be 0, 1, or 2
    static unsigned long lastPhaseSwitch = 0;
    
    // Switch phase every 30 seconds (30000 ms)
    if (millis() - lastPhaseSwitch > 30000) {
        balance_phase++;
        if (balance_phase > 2) balance_phase = 0;
        lastPhaseSwitch = millis();
    }
    // ----------------------------------------------------------------------

    bool printNow = (millis() - previousMillisBalancingPrint >= 5000);
    if (printNow) {
        previousMillisBalancingPrint = millis();
        Serial.printf("\n=== BALANCING CHECK (PHASE %d) ===\n", balance_phase);
        Serial.printf("Lowest cell: %d mV  Threshold: +%d mV  → Target: > %d mV\n", 
                      minVoltage, threshold, minVoltage + threshold);
    }

    bool isBalancing = false;

    // Step 3: Select cells with 1-in-3 safety rule
    for (int i = 0; i < 96; i++) {
        // Check if cell voltage is high enough
        if (cellblock_voltage[i] > 0 && cellblock_voltage[i] >= (minVoltage + threshold)) {
            
            // Check if this cell's turn matches current phase!
            // Cell 0, 3, 6 etc in phase 0. Cell 1, 4, 7 in phase 1. Cell 2, 5, 8 in phase 2.
            if ((i % 3) == balance_phase) {
                int byteIndex = i / 8;
                int bitIndex = i % 8;
                balanceMask[byteIndex] |= (1 << bitIndex);
                isBalancing = true;
                
                if (printNow) {
                    Serial.printf("Cell %02d: %d mV (+%d mV) → BLEEDING (Active)\n",
                        i+1, cellblock_voltage[i], cellblock_voltage[i] - minVoltage);
                }
            } else {
                if (printNow) {
                    Serial.printf("Cell %02d: %d mV (+%d mV) → PAUSED (Cooling)\n",
                        i+1, cellblock_voltage[i], cellblock_voltage[i] - minVoltage);
                }
            }
        }
    }

    if (printNow) {
        if (!isBalancing) {
            Serial.println("No cells active in this phase (or all within threshold).");
        }
        Serial.print("Safe 1-in-3 Mask: ");
        for (int i = 0; i < 12; i++) Serial.printf("%02X ", balanceMask[i]);
        Serial.println("\n======================");
    }
}

void BoltAmperaBattery::transmit_can(unsigned long currentMillis) {
    
    // --- 0. CLEAR DTC CODES ONCE (AFTER 3 SECONDS) ---
    static bool dtc_cleared = false;
    if (!dtc_cleared && currentMillis > 3000) {
        dtc_cleared = true; // Toggle flag: never repeat!
        
        CAN_frame dtc_clear_vicm;
        memset(&dtc_clear_vicm, 0, sizeof(CAN_frame));
        dtc_clear_vicm.ID = 0x7E4;
        dtc_clear_vicm.DLC = 8;
        dtc_clear_vicm.data.u8[0] = 0x04;
        dtc_clear_vicm.data.u8[1] = 0x14;
        dtc_clear_vicm.data.u8[2] = 0xFF;
        dtc_clear_vicm.data.u8[3] = 0xFF;
        dtc_clear_vicm.data.u8[4] = 0xFF;
        transmit_can_frame_to_interface(&dtc_clear_vicm, CAN_NATIVE);

        CAN_frame dtc_clear_bms;
        memset(&dtc_clear_bms, 0, sizeof(CAN_frame));
        dtc_clear_bms.ID = 0x7E7;
        dtc_clear_bms.DLC = 8;
        dtc_clear_bms.data.u8[0] = 0x04;
        dtc_clear_bms.data.u8[1] = 0x14;
        dtc_clear_bms.data.u8[2] = 0xFF;
        dtc_clear_bms.data.u8[3] = 0xFF;
        dtc_clear_bms.data.u8[4] = 0xFF;
        transmit_can_frame_to_interface(&dtc_clear_bms, CAN_NATIVE);
        
        Serial.println("!!! DTC CODES CLEARED VIA CAN NATIVE !!!");
    }

    // --- 1. HEARTBEAT (0x778) - Keeps internal contactors closed/active ---
    static unsigned long previousMillis778 = 0;
    if (currentMillis - previousMillis778 >= 20) {
        previousMillis778 = currentMillis;
        CAN_frame heartbeat;
        memset(&heartbeat, 0, sizeof(CAN_frame)); // Makes message safe and clean
        heartbeat.ID = 0x778;
        heartbeat.DLC = 7;
        heartbeat.data.u8[0] = 0x00; heartbeat.data.u8[1] = 0x31; 
        heartbeat.data.u8[2] = 0x00; heartbeat.data.u8[3] = 0x00; 
        heartbeat.data.u8[4] = 0x00; heartbeat.data.u8[5] = 0x00; 
        heartbeat.data.u8[6] = 0x00;
        transmit_can_frame_to_interface(&heartbeat, CAN_NATIVE); 
    }

   // --- 2. STAY AWAKE & BALANCING REQUEST (0x20E) ---
    static unsigned long previousMillis20E = 0;
    static uint8_t counter_20E = 0x10;
    
    if (currentMillis - previousMillis20E >= 20) { 
        previousMillis20E = currentMillis;

        // CALCULATE MASK FIRST (Stored in balanceMask)
        calculateBalancing();

        CAN_frame frame_20E;
        memset(&frame_20E, 0, sizeof(CAN_frame));
        frame_20E.ID = 0x20E;
        frame_20E.DLC = 6; // 1 byte counter + 4 bytes data
        frame_20E.data.u8[0] = counter_20E;

        // FILL ACTUAL CALCULATED DATA IN CAN MESSAGE
        if (counter_20E == 0x10) {
            // Section 1 (Cell 1-32)
            frame_20E.data.u8[1] = balanceMask[0]; 
            frame_20E.data.u8[2] = balanceMask[1]; 
            frame_20E.data.u8[3] = balanceMask[2]; 
            frame_20E.data.u8[4] = balanceMask[3]; 
        } 
        else if (counter_20E == 0x20) {
            // Section 2 (Cell 33-64)
            frame_20E.data.u8[1] = balanceMask[4]; 
            frame_20E.data.u8[2] = balanceMask[5]; 
            frame_20E.data.u8[3] = balanceMask[6]; 
            frame_20E.data.u8[4] = balanceMask[7]; 
        } 
        else if (counter_20E == 0x30) {
            // Section 3 (Cell 65-96)
            frame_20E.data.u8[1] = balanceMask[8]; 
            frame_20E.data.u8[2] = balanceMask[9]; 
            frame_20E.data.u8[3] = balanceMask[10]; 
            frame_20E.data.u8[4] = balanceMask[11]; 
        }

        transmit_can_frame_to_interface(&frame_20E, CAN_NATIVE);
        
        counter_20E += 0x10;
        if (counter_20E > 0x30) counter_20E = 0x10;
    }

    // --- 3. SOC-DEPENDENT 020A ---
    static unsigned long previousMillis20A = 0;
    if (currentMillis - previousMillis20A >= 20) {
        previousMillis20A = currentMillis;
        uint16_t soc_pct = datalayer_battery->status.real_soc / 100;
        uint8_t soc_byte = (uint8_t)(0xD4 + (soc_pct * 43 / 100));
        if (soc_byte == 0xD5) soc_byte = 0xD6;
        if (soc_byte > 0xFF) soc_byte = 0xFF;
        CAN_frame frame_20A;
        memset(&frame_20A, 0, sizeof(CAN_frame));
        frame_20A.ID = 0x20A;
        frame_20A.DLC = 8;
        frame_20A.data.u8[0] = 0x02;
        frame_20A.data.u8[4] = 0xB1;
        frame_20A.data.u8[6] = 0xA7;
        frame_20A.data.u8[7] = soc_byte;
        transmit_can_frame_to_interface(&frame_20A, CAN_NATIVE);
    }

    // --- 4. CHARGER STATUS & LIMITS (0x305) - Every 100ms ---
    static unsigned long previousMillis305 = 0;
    if (currentMillis - previousMillis305 >= 100) {
        previousMillis305 = currentMillis;
        CAN_frame frame_305;
        memset(&frame_305, 0, sizeof(CAN_frame));
        frame_305.ID = 0x305;
        frame_305.DLC = 8;
        frame_305.data.u8[0] = 0x60; frame_305.data.u8[1] = 0x00; frame_305.data.u8[2] = 0x02; frame_305.data.u8[3] = 0x58;
        frame_305.data.u8[4] = 0x11; frame_305.data.u8[5] = 0x94; frame_305.data.u8[6] = 0x05; frame_305.data.u8[7] = 0xDC;
        transmit_can_frame_to_interface(&frame_305, CAN_NATIVE);
    }

    // --- 5. CHARGER COMMAND (0x307) - Every 100ms ---
    static unsigned long previousMillis307 = 0;
    if (currentMillis - previousMillis307 >= 100) {
        previousMillis307 = currentMillis;
        CAN_frame frame_307;
        memset(&frame_307, 0, sizeof(CAN_frame));
        frame_307.ID = 0x307;
        frame_307.DLC = 8;
        frame_307.data.u8[0] = 0xCC; frame_307.data.u8[1] = 0x20; frame_307.data.u8[2] = 0x00; frame_307.data.u8[3] = 0x00;
        frame_307.data.u8[4] = 0xFF; frame_307.data.u8[5] = 0x04; frame_307.data.u8[6] = 0x00; frame_307.data.u8[7] = 0x00;
        transmit_can_frame_to_interface(&frame_307, CAN_NATIVE);
    }

    // --- 6. THERMAL MANAGEMENT / COOLING (0x460) - Every 500ms ---
    static unsigned long previousMillis460 = 0;
    if (currentMillis - previousMillis460 >= 500) {
        previousMillis460 = currentMillis;
        CAN_frame frame_460;
        memset(&frame_460, 0, sizeof(CAN_frame));
        frame_460.ID = 0x460;
        frame_460.DLC = 4;
        frame_460.data.u8[0] = 0x0A; frame_460.data.u8[1] = 0x1C; frame_460.data.u8[2] = 0x00; frame_460.data.u8[3] = 0x00;
        transmit_can_frame_to_interface(&frame_460, CAN_NATIVE);
    }

    // --- 7. OBC MESSAGES - every 100ms ---
    static unsigned long previousMillis_obc = 0;
    if (currentMillis - previousMillis_obc >= 100) {
        previousMillis_obc = currentMillis;

        CAN_frame f304; memset(&f304, 0, sizeof(CAN_frame));
        f304.ID = 0x304; f304.DLC = 4;
        transmit_can_frame_to_interface(&f304, CAN_NATIVE);

        CAN_frame f306; memset(&f306, 0, sizeof(CAN_frame));
        f306.ID = 0x306; f306.DLC = 8;
        transmit_can_frame_to_interface(&f306, CAN_NATIVE);

        // 0x0309: first 5 seconds 0x10 (EVSE coupled), then always 0x00
        CAN_frame f309; memset(&f309, 0, sizeof(CAN_frame));
        f309.ID = 0x309; f309.DLC = 8;
        if (currentMillis < 5000) f309.data.u8[0] = 0x10;
        transmit_can_frame_to_interface(&f309, CAN_NATIVE);

        CAN_frame f30C; memset(&f30C, 0, sizeof(CAN_frame));
        f30C.ID = 0x30C; f30C.DLC = 8;
        transmit_can_frame_to_interface(&f30C, CAN_NATIVE);

        CAN_frame f30F; memset(&f30F, 0, sizeof(CAN_frame));
        f30F.ID = 0x30F; f30F.DLC = 8;
        transmit_can_frame_to_interface(&f30F, CAN_NATIVE);

        CAN_frame f310; memset(&f310, 0, sizeof(CAN_frame));
        f310.ID = 0x310; f310.DLC = 8;
        transmit_can_frame_to_interface(&f310, CAN_NATIVE);

        CAN_frame f312; memset(&f312, 0, sizeof(CAN_frame));
        f312.ID = 0x312; f312.DLC = 7;
        transmit_can_frame_to_interface(&f312, CAN_NATIVE);

        // 0x0314 + 0x0316 always together, 0.1ms apart
        CAN_frame f314; memset(&f314, 0, sizeof(CAN_frame));
        f314.ID = 0x314; f314.DLC = 8;
        f314.data.u8[0] = 0x0F;
        f314.data.u8[6] = 0xB0;
        transmit_can_frame_to_interface(&f314, CAN_NATIVE);

        CAN_frame f316; memset(&f316, 0, sizeof(CAN_frame));
        f316.ID = 0x316; f316.DLC = 3;
        transmit_can_frame_to_interface(&f316, CAN_NATIVE);
    }
}

void BoltAmperaBattery::setup(void) {
    // 1. Existing configuration (Your original, important settings)
    strncpy(datalayer.system.info.battery_protocol, Name, 63);
    datalayer.system.info.battery_protocol[63] = '\0';
    datalayer_battery->info.number_of_cells = 96;
    datalayer_battery->info.total_capacity_Wh = 64000;
    datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
    datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
    datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
    datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
    datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
    if (allows_contactor_closing) {
        *allows_contactor_closing = true;
    }
}
