#ifndef TESLA_BATTERY_H
#define TESLA_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "TESLA-HTML.h"

// Little-endian (Intel) signal from an 8 byte CAN payload: start bit and length as in a DBC "start|len@1+"
inline uint32_t tesla_le_bits(const uint8_t* data, uint8_t start_bit, uint8_t length) {
  uint64_t raw = 0;
  for (int i = 7; i >= 0; i--) {
    raw = (raw << 8) | data[i];
  }
  return (uint32_t)((raw >> start_bit) & ((1ULL << length) - 1ULL));
}

// A pack CAC (0.1 Ah) is plausible when it is 20.0 to 400.0 Ah and min <= avg <= max (3/Y 0x3B2 and S/X 0x7E2)
inline bool tesla_cac_plausible(uint32_t min_x10, uint32_t avg_x10, uint32_t max_x10) {
  return min_x10 >= 200 && min_x10 <= avg_x10 && avg_x10 <= max_x10 && max_x10 <= 4000;
}

// 0x7FF gateway config, "Gen3" vehicles only, not applicable to Gen2 "classic" Model S and Model X
// These are user configurable from the Webserver UI
extern bool user_selected_tesla_digital_HVIL;
extern uint16_t user_selected_tesla_GTW_country;
extern bool user_selected_tesla_GTW_rightHandDrive;
extern uint16_t user_selected_tesla_GTW_mapRegion;
extern uint16_t user_selected_tesla_GTW_chassisType;
extern uint16_t user_selected_tesla_GTW_packEnergy;

class TeslaBattery : public CanBattery {
 public:
  bool mandatory_charge_taper() { return true; }
  // Use the default constructor to create the first or single battery.
  TeslaBattery() : renderer(&datalayer_extended.tesla, &datalayer.battery) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    previous_max_percentage = datalayer.battery_settings.max_percentage;
    datalayer_tesla = &datalayer_extended.tesla;
  }
  // Use this constructor for the second battery.
  TeslaBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_TESLA* extended, CAN_Interface targetCan)
      : CanBattery(targetCan), renderer(extended, datalayer_ptr) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    previous_max_percentage = datalayer.battery_settings.max_percentage;
    datalayer_tesla = extended;
  }
  virtual void setup();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  bool supports_clear_isolation() { return true; }
  bool supports_insulation_resistance() { return true; }
  void clear_isolation() { user_requests_isolation_clear = true; }

  bool supports_reset_BMS() { return true; }
  void reset_BMS() { user_requests_bms_reset = true; }

  // Slots in BMS_alertMatrixActive that the SOC and reset logic reads.
  static constexpr uint8_t kBmsAlertSocChange = 74;        // 0x320 mux 2, byte 3, bit 4
  static constexpr uint8_t kBmsAlertEcuResetBlocked = 99;  // 0x320 mux 2, byte 7, bit 7

  bool supports_reset_SOC() { return true; }
  void reset_SOC() { user_requests_soc_reset = true; }

  bool supports_charged_energy() { return true; }

  bool supports_manual_balancing() { return true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

  // Beginning of life energy (SOH estimate reference): 0x392 mux 1 (new fw) wins over 0x292 (legacy fw, reads 0 or 0.1
  // kWh on some firmware). The source is sticky: it depends only on the pack firmware, so once 0x392 mux 1 has given a
  // plausible value a later 0x292 value is never used. Source 0 = unavailable.
  // A raw 1023 in a 10 bit energy field (0x292, 0x392, legacy 0x382) is "not available".
  static constexpr uint8_t SohReferenceLegacyFw = 1;
  static constexpr uint8_t SohReferenceNewFw = 2;
  // Plausible nominal pack energy (full, when new): 38.0 to 150.0 kWh, in 0.1 kWh. Even a garbage pair at the two limits
  // gives 38.0 / 150.0 = 25.3 %, above the 25 % EVENT_SOH_LOW (ERROR) limit. Real packs are 54 to 100 kWh.
  static constexpr uint16_t EnergyMin_x10 = 380;
  static constexpr uint16_t EnergyMax_x10 = 1500;
  static constexpr bool energy_x10_plausible(uint16_t x10) { return x10 >= EnergyMin_x10 && x10 <= EnergyMax_x10; }
  static constexpr bool energy_x10_plausible_10bit(uint16_t x10) { return energy_x10_plausible(x10) && x10 != 1023; }
  // The nominal full pack energy is not available for short bursts (the S/X pack sends 1023 for about 5 s): the last
  // plausible value is held for EnergyHold_s seconds, then it is unknown (0): SOH 99 % and remaining Wh from the
  // configured capacity. Called once a second (update_values).
  static constexpr uint16_t EnergyHold_s = 30;
  // The hold counters count update_values() calls (1 Hz, not millis()).
  struct EnergyHold {
    uint32_t value = 0;  // held value (0.1 kWh for the nominal full energy, Wh for the remaining), 0 = unknown
    uint16_t seconds_not_plausible = 0;
  };
  static void hold_energy(EnergyHold& hold, uint32_t received, bool plausible);
  // The UI SOC the inverter gets, shared by the 3/Y and the S/X. The pack sends 1023 (not available) now and then: the
  // last valid UI SOC is held, for SocHoldLimit_s (30 minutes). After that the SOC is cleared (0, remaining capacity 0),
  // there is no fallback: the brick SOC and the energies are not the UI SOC. EVENT_SOC_UNAVAILABLE (a WARNING) is raised
  // after SocUnavailableDelay_s without a valid UI SOC. The counters count update() calls and only while the pack is on
  // the bus (CAN_battery_still_alive > 0): if the CAN bus dies they freeze and the last SOC stays, EVENT_CAN_RX_FAILURE
  // covers that case.
  struct SocHold {
    static constexpr uint16_t SocHoldLimit_s = 30 * 60;
    static constexpr uint16_t SocUnavailableDelay_s = 30;
    uint16_t soc_ui = 0;   // the held UI SOC, 0.1 %: the last valid one (or the initial SOC min)
    bool expired = false;  // no SOC for SocHoldLimit_s
    // On every SOC frame, the 10 bit UI SOC and SOC min in 0.1 %, 1023 = not available. SOC min is only the initial SOC
    // until a valid UI SOC was seen: 0 would zero the discharge limit (battery empty) at startup.
    void on_frame(uint16_t soc_ui_x10, uint16_t soc_min_x10);
    // Once a second (update_values)
    void update(bool pack_on_bus, uint8_t battery_index);
    uint16_t real_soc() const { return expired ? 0 : soc_ui * 10; }  // 0.01 %

   private:
    bool ui_seen = false;              // a valid UI SOC was received at least once
    bool ui_fresh = false;             // a valid UI SOC was received since the last update()
    bool any_fresh = false;            // a SOC (the UI SOC, or the initial SOC min) was received since then
    uint16_t seconds_without_soc = 0;  // no valid UI SOC (EVENT_SOC_UNAVAILABLE)
    uint16_t seconds_held = 0;         // no SOC at all, the held SOC is cleared after SocHoldLimit_s
  };
  // An SOH estimate above 101.00 % (0.01 %) is not credible: unknown. 100.00-101.00 % is sent as 100.00 %.
  static constexpr uint32_t SohEstimateMax_pptt = 10100;
  // SOH sent to the inverter (soh_pptt, 0.01 %) = nominal_full_x10 * 10000 / beginning_of_life_x10, both in 0.1 kWh:
  // 72.1 kWh over 85.0 kWh is 8482 = 84.82 %. When it cannot be calculated the inverter gets 99 % as on master
  // (soh_available stays true, only the web card says unknown). Returns whether the estimate could be calculated.
  static bool apply_soh_estimate(DATALAYER_BATTERY_STATUS_TYPE& status, uint32_t nominal_full_x10,
                                 uint32_t beginning_of_life_x10);
  // The nominal remaining energy (Wh, 0 = unknown) is plausible when above 0 and not above the nominal full energy (0.1
  // kWh, 0 = unknown) plus one 0.1 kWh LSB (the full energy is rounded to 0.1 kWh from 0.02 kWh counts). 0 is not
  // accepted: a pack that was not received yet reads 0, and with a real 0 kWh the SOC x full fallback is 0 too.
  static constexpr bool nominal_remaining_plausible(uint32_t remaining_Wh, uint32_t nominal_full_x10) {
    return remaining_Wh > 0 && nominal_full_x10 > 0 && remaining_Wh <= nominal_full_x10 * 100 + 100;
  }
  // Remaining Wh = the pack's nominal remaining energy (already in Wh) when plausible (the nominal full energy must be
  // known). Else real_soc / 10000 (0.01 % -> fraction) x nominal_full_x10 * 100 (0.1 kWh -> Wh), 27.10 % of 72.1 kWh
  // is 19539 Wh. OBSOLETE fallback: SOC x total_capacity_Wh when the full energy is not known.
  static uint32_t remaining_Wh(uint16_t real_soc, uint32_t nominal_full_x10, uint32_t nominal_remaining_Wh,
                               uint32_t total_capacity_Wh);

  static constexpr const char* NameSX = "Tesla Model S/X";
  static constexpr const char* Name3Y = "Tesla Model 3/Y";

 private:
  TeslaHtmlRenderer renderer;

  // OBSOLETE fallback: total capacity from the user selected GTW pack energy (50/74/62/100 kWh), set once in setup().
  // Used only when the pack reports no plausible capacity when new (0x392/0x292, legacy S/X packs report none).
  uint32_t fallback_total_capacity_Wh = 0;

  // Per-instance extended data for the "More Battery Info" webserver page.
  // Points at datalayer_extended.tesla for the main battery, or datalayer_extended.tesla_2
  // for the second battery, so double-battery setups no longer share one struct.
  DATALAYER_INFO_TESLA* datalayer_tesla;

 protected:
  /* Do not change anything below this line! */
  static const int MAX_PACK_VOLTAGE_SX_NCMA = 4600;   // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_SX_NCMA = 2850;   // V+1, if pack voltage goes over this, charge stops
  static const int MAX_PACK_VOLTAGE_3Y_NCMA = 4030;   // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_3Y_NCMA = 2850;   // V+1, if pack voltage goes below this, discharge stops
  static const int MAX_PACK_VOLTAGE_3Y_LFP = 3880;    // V+1, if pack voltage goes over this, charge stops
  static const int MIN_PACK_VOLTAGE_3Y_LFP = 2968;    // V+1, if pack voltage goes below this, discharge stops
  static const int MAX_CELL_DEVIATION_NCA_NCM = 500;  //LED turns yellow on the board if mv delta exceeds this value
  static const int MAX_CELL_DEVIATION_LFP = 400;      //LED turns yellow on the board if mv delta exceeds this value
  static const int MAX_CELL_VOLTAGE_NCA_NCM =
      4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_NCA_NCM =
      2950;                                      //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_VOLTAGE_LFP = 3650;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_LFP = 2800;  //Battery is put into emergency stop if one cell goes below this value

  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // Per-instance state for handle_incoming_can_frame.
  // These were previously static locals — static locals are shared across all
  // class instances, which breaks double/triple battery support.
  uint8_t mux = 0;
  uint16_t temp = 0;
  bool mux0_read = false;
  bool mux1_read = false;
  uint16_t brick_volts = 0;      // per-brick voltage scratch variable (0x401)
  uint8_t mux_zero_counter = 0;  // counts mux==0 frames to detect full cell scan
  uint8_t mux_max = 0;           // highest mux index seen so far

  // Per-instance state for transmit_can.
  int transmitPhase = -1;

  bool operate_contactors = false;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  void printFaultCodesIfActive();
  void printFaultCodesPcsCp();  // PCS 0x3A4 + CP 0x31E alert matrices (tesla-m3-pack-findings)

  unsigned long previousMillis10 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis500 = 0;   // will store last time a 500ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent

  //UDS session tracker
  //static bool uds_SessionInProgress = false; // Future use
  //0x221 VCFRONT_LVPowerState
  uint8_t alternateMux = 0;
  uint8_t frameCounter_TESLA_221 = 15;  // Start at 15 for Mux 0
  uint8_t vehicleState = 1;             // "OFF": 0, "DRIVE": 1, "ACCESSORY": 2, "GOING_DOWN": 3
  static const uint8_t CAR_OFF = 0;
  static const uint8_t CAR_DRIVE = 1;
  static const uint8_t ACCESSORY = 2;
  static const uint8_t GOING_DOWN = 3;
  uint8_t powerDownSeconds = 9;  // Car power down (i.e. contactor open) tracking timer, 3 seconds per sendingState
  //0x2E1 VCFRONT_status, 6 mux tracker
  uint8_t muxNumber_TESLA_2E1 = 0;
  //0x334 UI
  bool TESLA_334_INITIAL_SENT = false;
  //0x3A1 VCFRONT_vehicleStatus, 15 frame counter (temporary)
  uint8_t frameCounter_TESLA_3A1 = 0;
  bool timeToMux3A1 = true;
  //0x504 TWC_status
  bool TESLA_504_INITIAL_SENT = false;
  //0x7FF GTW_carConfig, 5 mux tracker
  uint8_t muxNumber_TESLA_7FF = 0;
  //Max percentage charge tracker
  uint16_t previous_max_percentage = 0;

  //0x082 UI_tripPlanning: "cycle_time" 1000ms
  static constexpr CAN_frame TESLA_082 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x082,
                                          .data = {0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80}};

  //0x102 VCLEFT_doorStatus: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_102 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x102,
                                          .data = {0x22, 0x33, 0x00, 0x00, 0xC0, 0x38, 0x21, 0x08}};

  //0x103 VCRIGHT_doorStatus: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_103 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x103,
                                          .data = {0x22, 0x33, 0x00, 0x00, 0x30, 0xF2, 0x20, 0x02}};

  //0x118 DI_systemStatus: "cycle_time" 50ms, DI_systemStatusChecksum/DI_systemStatusCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_118 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x118,
                         .data = {0xAB, 0x60, 0x2A, 0x00, 0x00, 0x08, 0x00, 0x00}};
  CAN_frame TESLA_118_digital_hvil = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x118,
                                      .data = {0x61, 0x80, 0x30, 0x10, 0x00, 0x08, 0x00, 0x80}};
  uint16_t content_118_digital_hvil[16] = {0x6180, 0x6281, 0x6382, 0x6483, 0x6584, 0x6685, 0x6786, 0x6887,
                                           0x6988, 0x6A89, 0x6B8A, 0x6C8B, 0x6D8C, 0x6E8D, 0x6F8E, 0x708F};

  //0x2A8 CMPD_state: "cycle_time" 100ms, different depending on firmware, semi-manual increment for now
  CAN_frame TESLA_2A8 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2A8,
                         .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x2C}};

  //0x213 UI_cruiseControl: "cycle_time" 500ms, UI_speedLimitTick/UI_cruiseControlCounter - different depending on firmware, semi-manual increment for now
  //Ref tesla-m3-pack-findings (fw 2019.20.4.2): 0x213 UI_cruiseControl DLC 3 (this frame uses DLC 2; likely firmware drift)
  CAN_frame TESLA_213 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x213, .data = {0x00, 0x15}};

  //0x221 These frames will/should eventually be migrated to 2 base frames (1 per mux), and then just the relevant bits changed

  //0x221 VCFRONT_LVPowerState "Drive"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_DRIVE (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_DRIVE_Mux0 = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x221,
                                    .data = {0x60, 0x55, 0x55, 0x15, 0x54, 0x51, 0xF1, 0xD8}};

  //0x221 VCFRONT_LVPowerState "Drive"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_DRIVE (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_DRIVE_Mux1 = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x221,
                                    .data = {0x61, 0x05, 0x55, 0x05, 0x00, 0x00, 0x00, 0xE3}};

  //0x221 VCFRONT_LVPowerState "Accessory"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_ACCESSORY (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_ACCESSORY_Mux0 = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 8,
                                        .ID = 0x221,
                                        .data = {0x40, 0x55, 0x55, 0x05, 0x54, 0x51, 0xF5, 0xAC}};

  //0x221 VCFRONT_LVPowerState "Accessory"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_ACCESSORY (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_ACCESSORY_Mux1 = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 8,
                                        .ID = 0x221,
                                        .data = {0x41, 0x05, 0x55, 0x55, 0x01, 0x00, 0x04, 0x18}};

  //0x221 VCFRONT_LVPowerState "Going Down"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts GOING_DOWN (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_GOING_DOWN_Mux0 = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 8,
                                         .ID = 0x221,
                                         .data = {0x00, 0x89, 0x55, 0x06, 0xA4, 0x51, 0xF1, 0xED}};

  //0x221 VCFRONT_LVPowerState "Going Down"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts GOING_DOWN (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_GOING_DOWN_Mux1 = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 8,
                                         .ID = 0x221,
                                         .data = {0x01, 0x09, 0x55, 0x59, 0x00, 0x00, 0x00, 0xDB}};

  //0x221 VCFRONT_LVPowerState "Off"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts OFF (Mux0, Counter 15): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_OFF_Mux0 = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x221,
                                  .data = {0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0xF1, 0x65}};

  //0x221 VCFRONT_LVPowerState "Off"
  //VCFRONT_vehiclePowerState VEHICLE_POWER_STATE_OFF, key parts OFF (Mux1, Counter 0): "cycle_time" 50ms each mux/LVPowerStateIndex, VCFRONT_LVPowerStateChecksum/VCFRONT_LVPowerStateCounter generated via generateMuxFrameCounterChecksum
  CAN_frame TESLA_221_OFF_Mux1 = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x221,
                                  .data = {0x01, 0x01, 0x01, 0x50, 0x00, 0x00, 0x00, 0x76}};

  //0x229 SCCM_rightStalk: "cycle_time" 100ms, SCCM_rightStalkChecksum/SCCM_rightStalkCounter generated via dedicated generateTESLA_229 function for now
  //CRC seemingly related to AUTOSAR ID array... "autosarDataIds": [124,182,240,47,105,163,221,28,86,144,202,9,67,125,183,241] found in Model 3 firmware
  CAN_frame TESLA_229 = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x229, .data = {0x46, 0x00, 0x00}};

  //0x241 VCFRONT_coolant: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_241 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 7,
                                          .ID = 0x241,
                                          .data = {0x35, 0x34, 0x0C, 0x0F, 0x8F, 0x55, 0x00}};

  //0x2D1 VCFRONT_okToUseHighPower: "cycle_time" 100ms
  static constexpr CAN_frame TESLA_2D1 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x2D1, .data = {0xFF, 0x01}};

  //0x2E1, 6 muxes
  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_VEHICLE_AND_RAILS = {.FD = false,
                                                            .ext_ID = false,
                                                            .DLC = 8,
                                                            .ID = 0x2E1,
                                                            .data = {0x29, 0x0A, 0x00, 0xFF, 0x0F, 0x00, 0x00, 0x00}};

  //{0x29, 0x0A, 0x00, 0xFF, 0x0F, 0x00, 0x00, 0x00} INIT
  //{0x29, 0x0A, 0x0D, 0xFF, 0x0F, 0x00, 0x00, 0x00} DRIVE
  //{0x29, 0x0A, 0x09, 0xFF, 0x0F, 0x00, 0x00, 0x00} HV_UP_STANDBY
  //{0x29, 0x0A, 0x0A, 0xFF, 0x0F, 0x00, 0x00, 0x00} ACCESSORY
  //{0x29, 0x0A, 0x06, 0xFF, 0x0F, 0x00, 0x00, 0x00} SLEEP_STANDBY

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_HOMELINK = {.FD = false,
                                                   .ext_ID = false,
                                                   .DLC = 8,
                                                   .ID = 0x2E1,
                                                   .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_REFRIGERANT_SYSTEM = {.FD = false,
                                                             .ext_ID = false,
                                                             .DLC = 8,
                                                             .ID = 0x2E1,
                                                             .data = {0x03, 0x6D, 0x99, 0x02, 0x1B, 0x57, 0x00, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_LV_BATTERY_DEBUG = {.FD = false,
                                                           .ext_ID = false,
                                                           .DLC = 8,
                                                           .ID = 0x2E1,
                                                           .data = {0xFC, 0x1B, 0xD1, 0x99, 0x9A, 0xD8, 0x09, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_MUX_5 = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 8,
                                                .ID = 0x2E1,
                                                .data = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x2E1 VCFRONT_status: "cycle_time" 10ms each mux/statusIndex
  static constexpr CAN_frame TESLA_2E1_BODY_CONTROLS = {.FD = false,
                                                        .ext_ID = false,
                                                        .DLC = 8,
                                                        .ID = 0x2E1,
                                                        .data = {0x08, 0x21, 0x04, 0x6E, 0xA0, 0x88, 0x06, 0x04}};

  //0x2E8 EPBR_status: "cycle_time" 100ms
  CAN_frame TESLA_2E8 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2E8,
                         .data = {0x02, 0x00, 0x10, 0x00, 0x00, 0x80, 0x00, 0x6C}};

  //0x284 UI_vehicleModes: "cycle_time" 500ms
  static constexpr CAN_frame TESLA_284 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 5,
                                          .ID = 0x284,
                                          .data = {0x10, 0x00, 0x00, 0x00, 0x00}};

  //0x293 UI_chassisControl: "cycle_time" 500ms, UI_chassisControlChecksum/UI_chassisControlCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_293 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x293,
                         .data = {0x01, 0x0C, 0x55, 0x91, 0x55, 0x15, 0x01, 0xF3}};

  //0x3A1 VCFRONT_vehicleStatus: "cycle_time" 50ms, VCFRONT_vehicleStatusChecksum/VCFRONT_vehicleStatusCounter eventually need to be generated via generateMuxFrameCounterChecksum
  //Looks like 2 muxes, counter at bit 52 width 4 and checksum at bit 56 width 8? Need later software Model3_ETH.compact.json signal file or DBC.
  CAN_frame TESLA_3A1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x3A1,
                         .data = {0xC3, 0xFF, 0xFF, 0xFF, 0x3D, 0x00, 0xD0, 0x01}};
  uint8_t frame6_3A1[16] = {0xD0, 0xE2, 0xF0, 0x02, 0x10, 0x22, 0x30, 0x42,
                            0x50, 0x62, 0x70, 0x82, 0x90, 0xA2, 0xB0, 0xC2};
  uint8_t frame7_3A1[16] = {0x01, 0xCB, 0x21, 0xEB, 0x41, 0x0B, 0x61, 0x2B,
                            0x81, 0x4B, 0xA1, 0x6B, 0xC1, 0x8B, 0xE1, 0xAB};
  //0x313 UI_powertrainControl: "cycle_time" 500ms, UI_powertrainControlChecksum/UI_powertrainControlCounter generated via generateFrameCounterChecksum
  //Ref tesla-m3-pack-findings (fw 2019.20.4.2): CAN 0x313 = UI_trackModeSettings on that firmware; UI_powertrainControl is 0x334 there (name drift)
  CAN_frame TESLA_313 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x313,
                         .data = {0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x1B}};

  //0x321 VCFRONT_sensors: "cycle_time" 1000ms
  CAN_frame TESLA_321 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x321,
      .data = {0xEC, 0x71, 0xA7, 0x6E, 0x02, 0x6C, 0x00, 0x04}};  // Last 2 bytes are counter and checksum

  //0x333 UI_chargeRequest: "cycle_time" 500ms, UI_chargeTerminationPct value = 900 [bit 16, width 10, scale 0.1, min 25, max 100]
  //Ref tesla-m3-pack-findings (fw 2019.20.4.2): 0x333 UI_chargeRequest DLC 4 (this frame uses DLC 5; likely firmware drift)
  CAN_frame TESLA_333 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x333, .data = {0x84, 0x30, 0x84, 0x07, 0x02}};

  //0x334 UI request: "cycle_time" 500ms, initial frame car sends
  //Ref tesla-m3-pack-findings (fw 2019.20.4.2): CAN 0x334 = UI_powertrainControl on that firmware
  static constexpr CAN_frame TESLA_334_INITIAL = {.FD = false,
                                                  .ext_ID = false,
                                                  .DLC = 8,
                                                  .ID = 0x334,
                                                  .data = {0x3F, 0x3F, 0xC8, 0x00, 0xE2, 0x3F, 0x80, 0x1E}};

  //0x334 UI request: "cycle_time" 500ms, generated via generateFrameCounterChecksum
  CAN_frame TESLA_334 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x334,
                         .data = {0x3F, 0x3F, 0x00, 0x0F, 0xE2, 0x3F, 0x90, 0x75}};

  //0x3B3 UI_vehicleControl2: "cycle_time" 500ms
  //Ref tesla-m3-pack-findings (fw 2019.20.4.2): 0x3B3 UI_vehicleControl2 DLC 2 (this frame uses DLC 8; likely firmware drift)
  static constexpr CAN_frame TESLA_3B3 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x3B3,
                                          .data = {0x90, 0x80, 0x05, 0x08, 0x00, 0x00, 0x00, 0x01}};

  //0x39D IBST_status: "cycle_time" 50ms, IBST_statusChecksum/IBST_statusCounter generated via generateFrameCounterChecksum
  CAN_frame TESLA_39D = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x39D, .data = {0xE1, 0x59, 0xC1, 0x27, 0x00}};

  //0x3C2 VCLEFT_switchStatus (Mux0, initial frame car sends): "cycle_time" 50ms, sent once
  static constexpr CAN_frame TESLA_3C2_INITIAL = {.FD = false,
                                                  .ext_ID = false,
                                                  .DLC = 8,
                                                  .ID = 0x3C2,
                                                  .data = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x5A, 0x05}};

  //0x3C2 VCLEFT_switchStatus (Mux0): "cycle_time" 50ms each mux/SwitchStatusIndex
  static constexpr CAN_frame TESLA_3C2_Mux0 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x3C2,
                                               .data = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x5A, 0x45}};

  //0x3C2 VCLEFT_switchStatus (Mux1): "cycle_time" 50ms each mux/SwitchStatusIndex
  static constexpr CAN_frame TESLA_3C2_Mux1 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x3C2,
                                               .data = {0x29, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x504 Initially sent
  static constexpr CAN_frame TESLA_504_INITIAL = {.FD = false,
                                                  .ext_ID = false,
                                                  .DLC = 8,
                                                  .ID = 0x504,
                                                  .data = {0x00, 0x1B, 0x06, 0x03, 0x00, 0x01, 0x00, 0x01}};

  //0x55A Unknown but always sent: "cycle_time" 500ms
  static constexpr CAN_frame TESLA_55A = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x55A,
                                          .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer (UK/RHD)
  CAN_frame TESLA_7FF_Mux1 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7FF,
                              .data = {0x01, 0x49, 0x42, 0x47, 0x00, 0x03, 0x15, 0x01}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer
  static constexpr CAN_frame TESLA_7FF_Mux2 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x02, 0x66, 0x32, 0x24, 0x04, 0x49, 0x95, 0x82}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer (EU/Long Range)
  CAN_frame TESLA_7FF_Mux3 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7FF,
                              .data = {0x03, 0x01, 0x08, 0x48, 0x01, 0x00, 0x00, 0x12}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer
  static constexpr CAN_frame TESLA_7FF_Mux4 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x04, 0x73, 0x03, 0x67, 0x5C, 0x00, 0x00, 0x00}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer
  static constexpr CAN_frame TESLA_7FF_Mux5 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x05, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer - later firmware has muxes 6 & 7, needed?
  static constexpr CAN_frame TESLA_7FF_Mux6 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x06, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0xD0}};

  //0x7FF GTW_carConfig: "cycle_time" 100ms each mux/carConfigMultiplexer - later firmware has muxes 6 & 7, needed?
  static constexpr CAN_frame TESLA_7FF_Mux7 = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 8,
                                               .ID = 0x7FF,
                                               .data = {0x07, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00}};

  //0x722 BMS_bmbKeepAlive: "cycle_time" 100ms, should only be sent when testing packs or diagnosing problems
  static constexpr CAN_frame TESLA_722 = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x722,
                                          .data = {0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}};

  //0x25D CP_status: "cycle_time" 100ms, stops some cpMia errors, but not necessary for standalone pack operation so not used/necessary. Note CP_type for different regions, the below has "IEC_CCS"
  static constexpr CAN_frame TESLA_25D = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x25D,
                                          .data = {0x37, 0x41, 0x01, 0x16, 0x08, 0x00, 0x00, 0x00}};

  //0x602 BMS UDS diagnostic request: on demand
  CAN_frame TESLA_602 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x602,
                         .data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};  // Define initial UDS request

  //0x610 BMS Query UDS request: on demand
  //Ref tesla-m3-pack-findings (fw 2019.20.4.2): CAN 0x610 = UDS_hvpRequest (HVP). On that firmware the BMS/HVBMS UDS request is 0x602 (UDS_bmsRequest) -> response 0x612. Verify addressing against target firmware.
  static constexpr CAN_frame TESLA_610 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x610,
      .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};  // Define initial UDS request
  //1CF needed for Digital HVIL option
  CAN_frame TESLA_1CF_digital_hvil = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x1CF,
                                      .data = {0x01, 0x00, 0x00, 0x1A, 0x1C, 0x02, 0x60, 0x69}};
  uint16_t content_1CF_digital_hvil[8] = {0x6069, 0x8089, 0xA0A9, 0xC0C9, 0xE0E9, 0x0009, 0x2029, 0x4049};
  uint8_t index_1CF = 0;
  uint8_t index_118 = 0;
  uint8_t contactor_counter = 0;
  uint8_t stateMachineClearIsolationFault = 0xFF;
  uint8_t stateMachineBMSReset = 0xFF;
  uint8_t stateMachineSOCReset = 0xFF;
  // Per-pack, not datalayer.battery_settings: that struct is shared by every pack, so with two
  // or three Tesla packs a command meant for one pack could be picked up by whichever instance's
  // update_values() happens to run next, not necessarily the one the user selected.
  bool user_requests_isolation_clear = false;
  bool user_requests_bms_reset = false;
  bool user_requests_soc_reset = false;
  uint8_t stateMachineBMSQuery = 0xFF;
  uint16_t battery_cell_max_v = 3300;
  uint16_t battery_cell_min_v = 3300;
  bool cellvoltagesRead = false;
  //0x3d2: 978 BMS_kwhCounter
  uint32_t battery_total_discharge = 0;
  uint32_t battery_total_charge = 0;
  //0x352: 850 BMS_energyStatus
  bool BMS352_mux = false;                            // variable to store when 0x352 mux is present
  uint16_t battery_energy_buffer = 0;                 // kWh
  uint16_t battery_energy_buffer_m1 = 0;              // kWh
  uint16_t battery_energy_to_charge_complete = 0;     // kWh
  uint16_t battery_energy_to_charge_complete_m1 = 0;  // kWh
  uint16_t battery_expected_energy_remaining = 0;     // kWh
  uint16_t battery_expected_energy_remaining_m1 = 0;  // kWh
  bool battery_full_charge_complete = false;          // Changed to bool
  bool battery_fully_charged = false;                 // Changed to bool
  uint16_t battery_ideal_energy_remaining = 0;        // kWh
  uint16_t battery_ideal_energy_remaining_m0 = 0;     // kWh
  uint16_t battery_nominal_energy_remaining = 0;      // kWh
  uint16_t battery_nominal_energy_remaining_m0 = 0;   // kWh
  bool nominal_remaining_seen = false;                // 0x352 without mux received
  bool nominal_remaining_m0_seen = false;             // 0x352 mux 0 received
  bool bol_0x292_seen = false;
  bool bol_0x392_seen = false;
  uint16_t battery_nominal_full_pack_energy = 0;     // Kwh
  uint16_t battery_nominal_full_pack_energy_m0 = 0;  // Kwh
  EnergyHold nominal_full_hold;                      // the value used for SOH and remaining Wh
  EnergyHold nominal_remaining_hold;                 // Wh, the value used for remaining Wh
  //0x132 306 HVBattAmpVolt
  uint16_t battery_volts = 0;                  // V
  int16_t battery_amps = 0;                    // A
  int16_t battery_raw_amps = 0;                // A
  uint16_t battery_charge_time_remaining = 0;  // Minutes
  //0x252 594 BMS_powerAvailable
  uint16_t BMS_maxRegenPower = 0;           //rename from battery_regenerative_limit
  uint16_t BMS_maxDischargePower = 0;       // rename from battery_discharge_limit
  uint16_t BMS_maxStationaryHeatPower = 0;  //rename from battery_max_heat_park
  uint16_t BMS_hvacPowerBudget = 0;         //rename from battery_hvac_max_power
  uint8_t BMS_notEnoughPowerForHeatPump = 0;
  uint8_t BMS_powerLimitState = 0;
  uint8_t BMS_inverterTQF = 0;
  //0x2d2: 722 BMSVAlimits
  uint16_t battery_max_discharge_current = 0;
  uint16_t battery_max_charge_current = 0;
  uint16_t BMS_max_voltage = 0;
  uint16_t BMS_min_voltage = 0;
  //0x2b4: 692 PCS_dcdcRailStatus
  bool older_firmware = false;              // DLC 5 frame seen: firmware up to 2023.2.12
  bool newer_firmware = false;              // 0x332 carries thermistorTAvg: firmware from 2023.12.1
  uint16_t battery_dcdcHvBusVolt = 0;       // 0.1 V
  uint16_t battery_dcdcLvBusVolt = 0;       // 10 mV
  int16_t battery_dcdcLvOutputCurrent = 0;  // 0.1 A
  //0x292: 658 BMS_socStatus
  bool battery_temp_seen = false;                // 0x332 mux 0 (thermistor min/max) received
  uint16_t battery_beginning_of_life = 0;        // 0.1 kWh, from 0x392 mux 1 else 0x292 (0 = unavailable)
  uint8_t battery_beginning_of_life_source = 0;  // 0 = unavailable, SohReferenceLegacyFw or SohReferenceNewFw
  uint16_t bol_0x292_x10 = 0;  // raw capacity when new of 0x292 and 0x392 mux 1, 0.1 kWh (detail page)
  uint16_t bol_0x392_x10 = 0;
  // Raw 0x292 SOC fields, 0.1 %, 1023 = not available / not received yet. The detail page shows them as received
  uint16_t battery_soc_min = 1023;
  uint16_t battery_soc_max = 1023;
  uint16_t battery_soc_ui_raw = 1023;  // live UI SOC
  uint16_t battery_soc_ave = 1023;
  SocHold soc_hold;  // the held UI SOC for the inverter
  uint8_t battery_battTempPct = 0;
  //0x392: BMS_packConfig
  uint32_t battery_packMass = 0;
  uint32_t battery_platformMaxBusVoltage = 0;  // raw, scaled in update_values
  uint32_t battery_packConfigMultiplexer = 0;
  uint32_t battery_moduleType = 0;
  uint32_t battery_reservedConfig = 0;
  //0x332: 818 BattBrickMinMax:BMS_bmbMinMax
  int16_t battery_max_temp = 0;  // C*
  int16_t battery_min_temp = 0;  // C*
  uint16_t battery_BrickVoltageMax = 0;
  uint16_t battery_BrickVoltageMin = 0;
  uint8_t battery_BrickTempMaxNum = 0;
  uint8_t battery_BrickTempMinNum = 0;
  uint8_t battery_BrickModelTMax = 0;
  uint8_t battery_BrickModelTMin = 0;
  uint8_t battery_BrickVoltageMaxNum = 0;  //rename from battery_max_vno
  uint8_t battery_BrickVoltageMinNum = 0;  //rename from battery_min_vno
  //0x20A: 522 HVP_contactorState
  uint8_t battery_contactor = 0;  //State of contactor
  uint8_t battery_hvil_status = 0;
  uint8_t battery_packContNegativeState = 0;
  uint8_t battery_packContPositiveState = 0;
  uint8_t battery_packContactorSetState = 0;
  bool battery_packCtrsClosingBlocked = false;    // Change to bool
  bool battery_pyroTestInProgress = false;        // Change to bool
  bool battery_packCtrsOpenNowRequested = false;  // Change to bool
  bool battery_packCtrsOpenRequested = false;     // Change to bool
  uint8_t battery_packCtrsRequestStatus = 0;
  bool battery_packCtrsResetRequestRequired = false;  // Change to bool
  bool battery_dcLinkAllowedToEnergize = false;       // Change to bool
  bool battery_fcContNegativeAuxOpen = false;         // Change to bool
  uint8_t battery_fcContNegativeState = 0;
  bool battery_fcContPositiveAuxOpen = false;  // Change to bool
  uint8_t battery_fcContPositiveState = 0;
  uint8_t battery_fcContactorSetState = 0;
  bool battery_fcCtrsClosingAllowed = false;    // Change to bool
  bool battery_fcCtrsOpenNowRequested = false;  // Change to bool
  bool battery_fcCtrsOpenRequested = false;     // Change to bool
  uint8_t battery_fcCtrsRequestStatus = 0;
  bool battery_fcCtrsResetRequestRequired = false;  // Change to bool
  bool battery_fcLinkAllowedToEnergize = false;     // Change to bool
                                                    //0x72A: BMS_serialNumber
  uint8_t battery_serialNumber[14] = {0};           // Stores raw HEX values for ASCII chars
  bool parsed_battery_serialNumber = false;
  char battery_manufactureDate[11] = {0};  // YYYY-MM-DD\0, owned per instance - see dayOfYearToDate()
                                           //Via UDS
  uint8_t battery_partNumber[12] = {0};    //stores raw HEX values for ASCII chars
  bool parsed_battery_partNumber = false;
  //Via UDS
  //static uint8_t BMS_partNumber[12] = {0};  //stores raw HEX values for ASCII chars
  //static bool parsed_BMS_partNumber = false;
  //0x300: BMS_info
  uint16_t BMS_info_buildConfigId = 0;
  uint16_t BMS_info_hardwareId = 0;
  uint16_t BMS_info_componentId = 0;
  uint8_t BMS_info_pcbaId = 0;
  uint8_t BMS_info_assemblyId = 0;
  uint16_t BMS_info_usageId = 0;
  uint16_t BMS_info_subUsageId = 0;
  uint8_t BMS_info_platformType = 0;
  uint32_t BMS_info_appCrc = 0;
  uint64_t BMS_info_bootGitHash = 0;
  uint8_t BMS_info_bootUdsProtoVersion = 0;
  uint32_t BMS_info_bootCrc = 0;
  //0x212: 530 BMS_status
  bool BMS_hvacPowerRequest = false;          //Change to bool
  bool BMS_notEnoughPowerForDrive = false;    //Change to bool
  bool BMS_notEnoughPowerForSupport = false;  //Change to bool
  bool BMS_preconditionAllowed = false;       //Change to bool
  bool BMS_updateAllowed = false;             //Change to bool
  bool BMS_activeHeatingWorthwhile = false;   //Change to bool
  bool BMS_cpMiaOnHvs = false;                //Change to bool
  uint8_t BMS_contactorState = 0;
  uint8_t BMS_state = 0;
  uint8_t BMS_hvState = 0;
  uint16_t BMS_isolationResistance = 0;
  bool BMS_chargeRequest = false;    //Change to bool
  bool BMS_keepWarmRequest = false;  //Change to bool
  uint8_t BMS_uiChargeStatus = 0;
  bool BMS_diLimpRequest = false;   //Change to bool
  bool BMS_okToShipByAir = false;   //Change to bool
  bool BMS_okToShipByLand = false;  //Change to bool
  uint32_t BMS_chgPowerAvailable = 0;
  uint8_t BMS_chargeRetryCount = 0;
  bool BMS_pcsPwmEnabled = false;        //Change to bool
  bool BMS_ecuLogUploadRequest = false;  //Change to bool
  uint8_t BMS_minPackTemperature = 0;
  // 0x224:548 PCS_dcdcStatus
  uint8_t PCS_dcdcPrechargeStatus = 0;
  uint8_t PCS_dcdc12VSupportStatus = 0;
  uint8_t PCS_dcdcHvBusDischargeStatus = 0;
  uint16_t PCS_dcdcMainState = 0;
  uint8_t PCS_dcdcSubState = 0;
  bool PCS_dcdcFaulted = false;          //Change to bool
  bool PCS_dcdcOutputIsLimited = false;  //Change to bool
  uint32_t PCS_dcdcMaxOutputCurrentAllowed = 0;
  uint8_t PCS_dcdcPrechargeRtyCnt = 0;
  uint8_t PCS_dcdc12VSupportRtyCnt = 0;
  uint8_t PCS_dcdcDischargeRtyCnt = 0;
  uint8_t PCS_dcdcPwmEnableLine = 0;
  uint8_t PCS_dcdcSupportingFixedLvTarget = 0;
  uint8_t PCS_ecuLogUploadRequest = 0;
  uint8_t PCS_dcdcPrechargeRestartCnt = 0;
  uint8_t PCS_dcdcInitialPrechargeSubState = 0;
  //0x312: 786 BMS_thermalStatus
  uint16_t BMS_powerDissipation = 0;
  uint16_t BMS_flowRequest = 0;
  uint16_t BMS_inletActiveCoolTargetT = 0;
  uint16_t BMS_inletPassiveTargetT = 0;
  uint16_t BMS_inletActiveHeatTargetT = 0;
  uint16_t BMS_packTMin = 0;
  uint16_t BMS_packTMax = 0;
  bool BMS_pcsNoFlowRequest = false;
  bool BMS_noFlowRequest = false;
  //0x3C4: PCS_info
  uint8_t PCS_partNumber[13] = {0};  //stores raw HEX values for ASCII chars
  bool parsed_PCS_partNumber = false;
  uint16_t PCS_info_buildConfigId = 0;
  uint16_t PCS_info_hardwareId = 0;
  uint16_t PCS_info_componentId = 0;
  uint8_t PCS_info_pcbaId = 0;
  uint8_t PCS_info_assemblyId = 0;
  uint16_t PCS_info_usageId = 0;
  uint16_t PCS_info_subUsageId = 0;
  uint8_t PCS_info_platformType = 0;
  uint32_t PCS_info_appCrc = 0;
  uint32_t PCS_info_cpu2AppCrc = 0;
  uint64_t PCS_info_bootGitHash = 0;
  uint8_t PCS_info_bootUdsProtoVersion = 0;
  uint32_t PCS_info_bootCrc = 0;
  //0x2A4; 676 PCS_thermalStatus
  int16_t PCS_chgPhATemp = 0;
  int16_t PCS_chgPhBTemp = 0;
  int16_t PCS_chgPhCTemp = 0;
  int16_t PCS_dcdcTemp = 0;
  int16_t PCS_ambientTemp = 0;
  //0x2C4; 708 PCS_logging
  uint16_t PCS_logMessageSelect = 0;
  uint16_t PCS_dcdcMaxLvOutputCurrent = 0;
  uint16_t PCS_dcdcCurrentLimit = 0;
  uint16_t PCS_dcdcLvOutputCurrentTempLimit = 0;
  uint16_t PCS_dcdcUnifiedCommand = 0;
  uint16_t PCS_dcdcCLAControllerOutput = 0;
  int16_t PCS_dcdcTankVoltage = 0;
  uint16_t PCS_dcdcTankVoltageTarget = 0;
  uint16_t PCS_dcdcClaCurrentFreq = 0;
  int16_t PCS_dcdcTCommMeasured = 0;
  uint16_t PCS_dcdcShortTimeUs = 0;
  uint16_t PCS_dcdcHalfPeriodUs = 0;
  uint16_t PCS_dcdcIntervalMaxFrequency = 0;
  uint16_t PCS_dcdcIntervalMaxHvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMaxLvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMaxLvOutputCurr = 0;
  uint16_t PCS_dcdcIntervalMinFrequency = 0;
  uint16_t PCS_dcdcIntervalMinHvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMinLvBusVolt = 0;
  uint16_t PCS_dcdcIntervalMinLvOutputCurr = 0;
  uint32_t PCS_dcdc12vSupportLifetimekWh = 0;
  //0x7AA: //1962 HVP_debugMessage:
  uint8_t HVP_debugMessageMultiplexer = 0;
  bool HVP_gpioPassivePyroDepl = false;  //Change to bool
  bool HVP_gpioPyroIsoEn = false;        //Change to bool
  bool HVP_gpioCpFaultIn = false;        //Change to bool
  bool HVP_gpioPackContPowerEn = false;  //Change to bool
  bool HVP_gpioHvCablesOk = false;       //Change to bool
  bool HVP_gpioHvpSelfEnable = false;    //Change to bool
  bool HVP_gpioLed = false;              //Change to bool
  bool HVP_gpioCrashSignal = false;      //Change to bool
  bool HVP_gpioShuntDataReady = false;   //Change to bool
  bool HVP_gpioFcContPosAux = false;     //Change to bool
  bool HVP_gpioFcContNegAux = false;     //Change to bool
  bool HVP_gpioBmsEout = false;          //Change to bool
  bool HVP_gpioCpFaultOut = false;       //Change to bool
  bool HVP_gpioPyroPor = false;          //Change to bool
  bool HVP_gpioShuntEn = false;          //Change to bool
  bool HVP_gpioHvpVerEn = false;         //Change to bool
  bool HVP_gpioFcContFlywheelEnable = false;
  bool HVP_gpioCpLatchEnable = false;
  bool HVP_gpioFcContPowerEnable = false;
  bool HVP_gpioHvilEnable = false;
  bool HVP_gpioPortSelSpiRdy = false;
  bool HVP_gpioPyroUnlock = false;
  uint16_t HVP_hvp1v5Ref = 0;
  int16_t HVP_shuntCurrentDebug = 0;
  bool HVP_packCurrentMia = false;
  bool HVP_auxCurrentMia = false;
  bool HVP_currentSenseMia = false;
  uint16_t HVP_info_buildConfigId = 0;
  uint16_t HVP_info_hardwareId = 0;
  uint16_t HVP_info_componentId = 0;
  uint8_t HVP_info_pcbaId = 0;
  uint8_t HVP_info_assemblyId = 0;
  uint16_t HVP_info_usageId = 0;
  uint16_t HVP_info_subUsageId = 0;
  uint8_t HVP_info_platformType = 0;
  uint32_t HVP_info_appCrc = 0;
  uint64_t HVP_info_bootGitHash = 0;
  uint8_t HVP_info_bootUdsProtoVersion = 0;
  uint32_t HVP_info_bootCrc = 0;
  int16_t HVP_dcLinkVoltage = 0;
  int16_t HVP_packVoltage = 0;
  int16_t HVP_fcLinkVoltage = 0;
  uint16_t HVP_packContVoltage = 0;
  int16_t HVP_packNegativeV = 0;
  int16_t HVP_packPositiveV = 0;
  uint16_t HVP_pyroAnalog = 0;
  int16_t HVP_dcLinkNegativeV = 0;
  int16_t HVP_dcLinkPositiveV = 0;
  int16_t HVP_fcLinkNegativeV = 0;
  uint16_t HVP_fcContCoilCurrent = 0;
  uint16_t HVP_fcContVoltage = 0;
  uint16_t HVP_hvilInVoltage = 0;
  uint16_t HVP_hvilOutVoltage = 0;
  int16_t HVP_fcLinkPositiveV = 0;
  uint16_t HVP_packContCoilCurrent = 0;
  uint16_t HVP_battery12V = 0;
  int16_t HVP_shuntRefVoltageDbg = 0;
  int16_t HVP_shuntAuxCurrentDbg = 0;
  int16_t HVP_shuntBarTempDbg = 0;
  int16_t HVP_shuntAsicTempDbg = 0;
  uint8_t HVP_shuntAuxCurrentStatus = 0;
  uint8_t HVP_shuntBarTempStatus = 0;
  uint8_t HVP_shuntAsicTempStatus = 0;
  //0x3aa: HVP_alertMatrix1 Fault codes   // Change to bool
  //We ignore 0x3AA for now, as on later software/firmware this is a muxed frame so values aren't correct.
  /*
  bool battery_WatchdogReset = false;           //Warns if the processor has experienced a reset due to watchdog reset.
  bool battery_PowerLossReset = false;          //Warns if the processor has experienced a reset due to power loss.
  bool battery_SwAssertion = false;             //An internal software assertion has failed.
  bool battery_CrashEvent = false;              //Warns if the crash signal is detected by HVP
  bool battery_OverDchgCurrentFault = false;    //Warns if the pack discharge is above max discharge current limit
  bool battery_OverChargeCurrentFault = false;  //Warns if the pack discharge current is above max charge current limit
  bool battery_OverCurrentFault = false;  //Warns if the pack current (discharge or charge) is above max current limit.
  bool battery_OverTemperatureFault = false;  //A pack module temperature is above maximum temperature limit
  bool battery_OverVoltageFault = false;      //A brick voltage is above maximum voltage limit
  bool battery_UnderVoltageFault = false;     //A brick voltage is below minimum voltage limit
  bool battery_PrimaryBmbMiaFault =
      false;  //Warns if the voltage and temperature readings from primary BMB chain are mia
  bool battery_SecondaryBmbMiaFault =
      false;  //Warns if the voltage and temperature readings from secondary BMB chain are mia
  bool battery_BmbMismatchFault =
      false;  //Warns if the primary and secondary BMB chain readings don't match with each other
  bool battery_BmsHviMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
  bool battery_CpMiaFault = false;       //Warns if the CP node is mia on HVS CAN
  bool battery_PcsMiaFault = false;      //The PCS node is mia on HVS CAN
  bool battery_BmsFault = false;         //Warns if the BMS ECU has faulted
  bool battery_PcsFault = false;         //Warns if the PCS ECU has faulted
  bool battery_CpFault = false;          //Warns if the CP ECU has faulted
  bool battery_ShuntHwMiaFault = false;  //Warns if the shunt current reading is not available
  bool battery_PyroMiaFault = false;     //Warns if the pyro squib is not connected
  bool battery_hvsMiaFault = false;      //Warns if the pack contactor hw fault
  bool battery_hviMiaFault = false;      //Warns if the FC contactor hw fault
  bool battery_Supply12vFault = false;   //Warns if the low voltage (12V) battery is below minimum voltage threshold
  bool battery_VerSupplyFault = false;   //Warns if the Energy reserve voltage supply is below minimum voltage threshold
  bool battery_HvilFault = false;        //Warn if a High Voltage Inter Lock fault is detected
  bool battery_BmsHvsMiaFault = false;   //Warns if the BMS node is mia on HVS or HVI CAN
  bool battery_PackVoltMismatchFault =
      false;                         //Warns if the pack voltage doesn't match approximately with sum of brick voltages
  bool battery_EnsMiaFault = false;  //Warns if the ENS line is not connected to HVC
  bool battery_PackPosCtrArcFault = false;  //Warns if the HVP detectes series arc at pack contactor
  bool battery_packNegCtrArcFault = false;  //Warns if the HVP detectes series arc at FC contactor
  bool battery_ShuntHwAndBmsMiaFault = false;
  bool battery_fcContHwFault = false;
  bool battery_robinOverVoltageFault = false;
  bool battery_packContHwFault = false;
  bool battery_pyroFuseBlown = false;
  bool battery_pyroFuseFailedToBlow = false;
  bool battery_CpilFault = false;
  bool battery_PackContactorFellOpen = false;
  bool battery_FcContactorFellOpen = false;
  bool battery_packCtrCloseBlocked = false;
  bool battery_fcCtrCloseBlocked = false;
  bool battery_packContactorForceOpen = false;
  bool battery_fcContactorForceOpen = false;
  bool battery_dcLinkOverVoltage = false;
  bool battery_shuntOverTemperature = false;
  bool battery_passivePyroDeploy = false;
  bool battery_logUploadRequest = false;
  bool battery_packCtrCloseFailed = false;
  bool battery_fcCtrCloseFailed = false;
  bool battery_shuntThermistorMia = false;
  */
  // Bits stored by the 0x320 handler.
  bool bmsSocChangeAlert() const { return teslaAlertGet(datalayer_tesla->BMS_alertMatrixActive, kBmsAlertSocChange); }
  bool bmsEcuResetBlocked() const {
    return teslaAlertGet(datalayer_tesla->BMS_alertMatrixActive, kBmsAlertEcuResetBlocked);
  }
};

#endif
