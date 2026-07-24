#ifndef SUNGROW_CAN_H
#define SUNGROW_CAN_H

#include "CanInverterProtocol.h"

// Sungrow battery configuration
struct SungrowBatteryConfig {
  uint16_t nameplate_wh;
  uint8_t module_count;
};

class SungrowInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  // Constructor: request 250 kbps on the inverter CAN interface
  SungrowInverter() : CanInverterProtocol(CAN_Speed::CAN_SPEED_250KBPS) {}
  bool setup() override;
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Sungrow SBRXXX emulation over CAN bus";
  static constexpr uint8_t MODBUS_SLAVE_ADDR = 0x01;
  static constexpr uint16_t MODBUS_REGISTER_BASE_ADDR = 0x4DE2;
  static constexpr uint16_t MODBUS_REGISTER_QTY = 0x0006;

 private:
  unsigned long previousMillisBatch = 0;
  unsigned long previousMillis1s = 0;
  unsigned long previousMillis10s = 0;
  unsigned long previousMillis60s = 0;
  bool transmit_can_init = true;
  const uint8_t delay_between_batches_ms = INTERVAL_20_MS;

  uint8_t mux = 0;
  uint8_t version_char[14] = {0};
  uint8_t manufacturer_char[14] = {0};
  uint8_t model_char[14] = {0};
  uint32_t remaining_wh = 0;
  uint32_t capacity_wh = 0;
  uint8_t batch_send_index = 0;

  // Battery configuration (set via user_selected_inverter_sungrow_type = model 0-6)
  SungrowBatteryConfig battery_config = {9600, 3};  // Default: SBR096

  // Returns config based on battery model (0=SBR064 through 6=SBR256)
  // Model + 2 = module count, each module adds 3200Wh of capacity
  static constexpr SungrowBatteryConfig get_config_for_model(uint16_t model) {
    switch (model) {
      case 0:
        return {6400, 2};  // SBR064
      case 2:
        return {12800, 4};  // SBR128
      case 3:
        return {16000, 5};  // SBR160
      case 4:
        return {19200, 6};  // SBR192
      case 5:
        return {22400, 7};  // SBR224
      case 6:
        return {25600, 8};  // SBR256
      case 1:
      default:
        return {9600, 3};  // SBR096
    }
  }

  // Cached signed current in deci-amps (range: int16_t)
  int16_t current_dA = 0;

  // Clamp an int32 -> int16 safely
  static constexpr int16_t clamp_i32_to_i16(int32_t value) {
    if (value > 32767) {
      return 32767;
    }

    if (value < -32768) {
      return -32768;
    }

    return static_cast<int16_t>(value);
  }

  //Actual content messages
  CAN_frame SUNGROW_000 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x000,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_001 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x001,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_002 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x002,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_003 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x003,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_004 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x004,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_005 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x005,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_006 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x006,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_007 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x007,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_008_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x008,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_008_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x008,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_009 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x009,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00A_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x00A,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00A_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x00A,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x00B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x00D,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x00E,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_013 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x013,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_014 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x014,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_015 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x015,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_016 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x016,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_017 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x017,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_018 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x018,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_019 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x019,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01A,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01C = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01C,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01D,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01E,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_401 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x401,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x402 - static .30 head frame (SBRBCU-S_22011.01.30). On the real bus it REPLACES 0x400 (both 0x400
  //         and 0x1F0 stop when 0x402 starts). Payload is constant, byte 2 = 0x55. The emulator targets
  //         .30, so this is sent every cycle in the head group (SoC is carried by 0x500/0x702).
  CAN_frame SUNGROW_402 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x402,
                           .data = {0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x1F0 - firmware-update channel (fw .27-era; on the real bus 23 Jan-13 Jun 2026, dropped on .30).
  //         Irregular cadence, not steady telemetry - not emulated (no frame defined or transmitted).
  // 0x500 - BMS init message. b0-6 = static unknown values (kept visible below); b7 = SoC (set in update_values).
  CAN_frame SUNGROW_500 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x500,
                           .data = {0x01,                    // Unknown (500_Unknown_0)
                                    0x01, 0x03,              // Unknown (500_Unknown_1; b2 old guess: module count)
                                    0xFF, 0x00, 0x01, 0x00,  // Unknown (500_Unknown_2)
                                    0x00}};                  // Battery SoC integer % - set in update_values()
  CAN_frame SUNGROW_501 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x501,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_502 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x502,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_503 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x503,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_504 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x504,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_505 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x505,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_506 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x506,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_512 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x512,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x700 - All Zero
  CAN_frame SUNGROW_700 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x700,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x701 - Battery Operational Parameters
  CAN_frame SUNGROW_701 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x701,
                           .data = {0x00, 0x00,    // Battery MAX voltage
                                    0x00, 0x00,    // Battery MIN voltage
                                    0x00, 0x00,    // Battery MAX charge current
                                    0x00, 0x00}};  // Battery MIN charge current
  // 0x702 - Battery Charge Level Status
  CAN_frame SUNGROW_702 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x702,
                           .data = {0x00, 0x00,    // Battery SoC %
                                    0x00, 0x00,    // Battery SoH %
                                    0x00, 0x00,    // Battery remaining Wh
                                    0x00, 0x00}};  // Battery total Wh
  // 0x703 - Battery Energy Status
  CAN_frame SUNGROW_703 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x703,
                           .data = {0x00, 0x00, 0x00, 0x00,    // Energy charged
                                    0x00, 0x00, 0x00, 0x00}};  // Energy discharged
  // 0x704 - Battery Status
  CAN_frame SUNGROW_704 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x704,
                           .data = {0x00, 0x00,    // Battery voltage
                                    0x00, 0x00,    // Battery current
                                    0x00, 0x00,    // Another related battery voltage
                                    0x00, 0x00}};  // Battery temperature
  // 0x705 - Battery Status
  CAN_frame SUNGROW_705 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x705,
                           .data = {0x02,        // Battery status: 0=Unplugged, 1=Standby, 2=Run (always Run)
                                    0x00,        // Always 0 (705_Always_0)
                                    0x01,        // Always 1 (705_Always_1)
                                    0x00, 0x00,  // Battery model id - set in update_values() from module_count
                                    0x00, 0x00,  // Battery voltage - set in update_values()
                                    0x00}};      // Padding (705_Padding)
  // 0x706 - Battery Cell Status
  CAN_frame SUNGROW_706 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x706,
                           .data = {0x00, 0x00,    // Cell MAX temperature
                                    0x00, 0x00,    // Cell MIN temperature
                                    0x00, 0x00,    // Cell MAX voltage
                                    0x00, 0x00}};  // Cell MIN voltage
  // 0x707 - Battery Configuration
  CAN_frame SUNGROW_707 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x707,
                           .data = {0x30,        // BMS firmware patch (BCD .30)
                                    0x00, 0x00,  // Always 0
                                    0x01,        // BMS firmware minor (BCD .1)
                                    0x00, 0x00,  // Nameplate capacity Wh - filled in setup() from battery_config
                                    0x00,        // Module count - filled in setup() from battery_config
                                    0x00}};      // Always 0
  // 0x708 - Battery Serial Number (MUX)
  CAN_frame SUNGROW_708_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x708,
                              .data = {0x00,                                        // Fragment #
                                       0x53, 0x32, 0x33, 0x31, 0x30, 0x31, 0x32}};  // "S231012"
  CAN_frame SUNGROW_708_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x708,
                              .data = {0x01,                                        // Fragment #
                                       0x33, 0x34, 0x35, 0x36, 0x00, 0x00, 0x53}};  // "3456" S suffix
  // 0x709 - All Zero
  CAN_frame SUNGROW_709 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x709,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x70A - Battery Type (MUX)
  CAN_frame SUNGROW_70A_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70A,
                              .data = {0x00,                                        // Mux
                                       0x53, 0x55, 0x4E, 0x47, 0x52, 0x4F, 0x57}};  // "SUNGROW"
  CAN_frame SUNGROW_70A_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70A,
                              .data = {0x01,                                        // Mux
                                       0x00, 0x42, 0x52, 0x58, 0x58, 0x58, 0x00}};  // "\0BRXXX\0"
  // 0x70B - Battery Family
  CAN_frame SUNGROW_70B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x70B,
                           .data = {0x00, 0x53, 0x42, 0x52, 0x58, 0x58, 0x58, 0x00}};  // "\0SBRXXX\0"
  // 0x70D - TODO Modbus 3_10701
  CAN_frame SUNGROW_70D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x70D,
                           .data = {0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00}};
  // 0x70E - TODO Modbus 3_10705
  CAN_frame SUNGROW_70E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x70E,
                           .data = {0x07, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x70F (MUX). Byte 0 = mux selector, byte 1 = 0. Mux 00 b4-7 = max continuous power (static
  //             rating), filled in setup() from battery_config; mux 05/06/07 = per-module SoC, set
  //             in update_values(). The remaining unknown bytes below are frozen snapshots from a
  //             real-battery capture - kept visible so they aren't mistaken for decoded or live
  //             values. "Cold-drift cluster" = the 16-bit unknowns at b2-3 of muxes 02/03/04: on a
  //             real pack all three drift slowly *together* and read higher in the cold. Ruled out
  //             as power/current limits (those derate in cold), and as SoC and SoH.
  CAN_frame SUNGROW_70F_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x00,          // Mux selector
                                       0x00,          // Always 0
                                       0x88, 0x13,    // Unknown, fixed 5000 (static)
                                       0x00, 0x00,    // Max continuous power W, live - set in setup()
                                       0x00, 0x00}};  // Max continuous power W, rating - set in setup()
  CAN_frame SUNGROW_70F_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              // Mux selector + padding: whole payload is 0 on a real pack
                              .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_02 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x02,          // Mux selector
                                       0x00,          // Always 0
                                       0x70, 0x20,    // Unknown 16-bit (cold-drift cluster)
                                       0x00, 0x00,    // Padding (always 0)
                                       0x92, 0x09}};  // Unknown 16-bit, near-constant (~5760)
  CAN_frame SUNGROW_70F_03 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x03,          // Mux selector
                                       0x00,          // Always 0
                                       0xFD, 0x1D,    // Unknown 16-bit (cold-drift cluster)
                                       0x00, 0x00,    // Padding (always 0)
                                       0xCE, 0x26}};  // Unknown ~9980 (x0.01 = 99.8 %); not SoC, not SoH
  CAN_frame SUNGROW_70F_04 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x04,              // Mux selector
                                       0x00,              // Always 0
                                       0x0C, 0x06,        // Unknown 16-bit (cold-drift cluster), not charge
                                       0x00, 0x00, 0x00,  // Padding (always 0)
                                       0x00}};            // Charge state: 0 = Normal, 2 = Stop Charge
  CAN_frame SUNGROW_70F_05 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_06 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_07 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // 0x713 - Battery Cell Temperature Overview. Cell/module locations are arbitrary static placeholders
  //         (not the real min/max cell position); temperatures (b2-3, b6-7) set in update_values().
  CAN_frame SUNGROW_713 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x713,
                           .data = {0x02,          // Cell location, min temp (arbitrary)
                                    0x01,          // Module location, min temp (arbitrary)
                                    0x00, 0x00,    // Cell MIN temperature - set in update_values()
                                    0x01,          // Cell location, max temp (arbitrary)
                                    0x02,          // Module location, max temp (arbitrary)
                                    0x00, 0x00}};  // Cell MAX temperature - set in update_values()
  // 0x714 - Battery Cell Voltage Overview. Max-first: b0-3 = max cell, b4-7 = min cell (opposite of the
  //         min-first 0x713/0x715/0x716; confirmed on a real SBR - b2-3 always > b6-7). Cell/module
  //         locations are arbitrary static placeholders; voltages (b2-3, b6-7) set in update_values().
  CAN_frame SUNGROW_714 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x714,
                           .data = {0x05,          // Cell location, max voltage (arbitrary)
                                    0x01,          // Module location, max voltage (arbitrary)
                                    0x00, 0x00,    // Cell MAX voltage - set in update_values()
                                    0x11,          // Cell location, min voltage (arbitrary)
                                    0x02,          // Module location, min voltage (arbitrary)
                                    0x00, 0x00}};  // Cell MIN voltage - set in update_values()
  // 0x715 - Module 1+2 Cell Voltage Overview
  CAN_frame SUNGROW_715 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x715,
                           .data = {0x00, 0x00,    // Module 1 Cell MIN voltage
                                    0x00, 0x00,    // Module 1 Cell MAX voltage
                                    0x00, 0x00,    // Module 2 Cell MIN voltage
                                    0x00, 0x00}};  // Module 2 Cell MAX voltage
  // 0x716 - Module 2+3 Cell Voltage Overview
  CAN_frame SUNGROW_716 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x716,
                           .data = {0x00, 0x00,    // Module 3 Cell MIN voltage
                                    0x00, 0x00,    // Module 3 Cell MAX voltage
                                    0x00, 0x00,    // Module 4 Cell MIN voltage
                                    0x00, 0x00}};  // Module 4 Cell MAX voltage
  // 0x717 - Module 5+6 Cell Voltage Overview
  CAN_frame SUNGROW_717 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x717,
                           .data = {0x00, 0x00,    // Module 5 Cell MIN voltage
                                    0x00, 0x00,    // Module 5 Cell MAX voltage
                                    0x00, 0x00,    // Module 6 Cell MIN voltage
                                    0x00, 0x00}};  // Module 6 Cell MAX voltage
  // 0x718 - Module 7+8 Cell Voltage Overview
  CAN_frame SUNGROW_718 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x718,
                           .data = {0x00, 0x00,    // Module 7 Cell MIN voltage
                                    0x00, 0x00,    // Module 7 Cell MAX voltage
                                    0x00, 0x00,    // Module 8 Cell MIN voltage
                                    0x00, 0x00}};  // Module 8 Cell MAX voltage
  // 0x719 - POSSIBLY Status (3_10789) and Module fault (3_10790)
  CAN_frame SUNGROW_719 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x719,
                           .data = {0x02,                                        // Unknown (static, possibly status)
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  // (zero)
  // 0x71A - Module Cell Type (set in setup)
  CAN_frame SUNGROW_71A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71A,
                           .data = {0x00,    // Module 1 Cell type (set in setup)
                                    0x00,    // Module 2 Cell type (set in setup)
                                    0x00,    // Module 3 Cell type (set in setup)
                                    0x00,    // Module 4 Cell type (set in setup)
                                    0x00,    // Module 5 Cell type (set in setup)
                                    0x00,    // Module 6 Cell type (set in setup)
                                    0x00,    // Module 7 Cell type (set in setup)
                                    0x00}};  // Module 8 Cell type (set in setup)
  // 0x71B - Module 1+2 Production Date (set in setup)
  CAN_frame SUNGROW_71B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71B,
                           .data = {0x00, 0x00, 0x00, 0x00,    // Module 1 production date
                                    0x00, 0x00, 0x00, 0x00}};  // Module 2 production date
  // 0x71C - Module 3+4 Production Date (set in setup)
  CAN_frame SUNGROW_71C = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71C,
                           .data = {0x00, 0x00, 0x00, 0x00,    // Module 3 production date
                                    0x00, 0x00, 0x00, 0x00}};  // Module 4 production date
  // 0x71D - Module 5+6 Production Date (set in setup)
  CAN_frame SUNGROW_71D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71D,
                           .data = {0x00, 0x00, 0x00, 0x00,    // Module 5 production date
                                    0x00, 0x00, 0x00, 0x00}};  // Module 6 production date
  // 0x71E - Module 7+8 Production Date (set in setup)
  CAN_frame SUNGROW_71E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71E,
                           .data = {0x00, 0x00, 0x00, 0x00,    // Module 7 production date
                                    0x00, 0x00, 0x00, 0x00}};  // Module 8 production date
  // 0x71F - Module Serial Number (MUX) - set in setup()
  CAN_frame SUNGROW_71F_01_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_01_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_01_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_02_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_02_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_02_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_03_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_03_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_03_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_04_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_04_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_04_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_05_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_05_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_05_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_06_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_06_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_06_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_07_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_07_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x07, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_07_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_08_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_08_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_08_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
