#ifndef PYLON_CAN_H
#define PYLON_CAN_H

#include "CanInverterProtocol.h"

/* Pylontech-HV-over-CAN as spoken by a Deye HV battery (HVCU firmware HVCUE1625C16N01, V2.05.1).
 *
 * Frame layout, encodings and timing were taken from the original battery firmware running in an
 * emulator (see deye-emu). Differences from the generic Pylon HV implementation:
 *  - all multi-byte values are little-endian; currents carry a +30000 (0.1 A) offset
 *  - frames are broadcast on the battery's schedule (10 ms tick, at most one frame per tick):
 *      every 1 s    : 0x4210..0x4290, 0x5300
 *      every ~5.5 s : 0x42E0, 0x42F0 ("Deye_HV"), 0x7310, 0x7320, 0x5210..0x52F0, 0x5310
 *    the content of the inverter's 0x4200 request is ignored (it only marks the inverter alive)
 *  - 0x4201 challenge is answered on 0x4202 with the first 8 bytes of SHA-256(challenge)
 *  - 0x7320 is [towers u16][modules u8][cells/module u8][voltage level V u16][Ah u16]
 *  - 0x7330/0x7340 are not sent; the manufacturer string is 0x42F0
 */
class PylonInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  bool setup() override;
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Pylontech HV over CAN (Deye)";

 private:
  void send_challenge_response(const CAN_frame& challenge);

  /* System description reported in 0x7320. Settings in the web UI override these:
     modules -> user_selected_inverter_modules, cells per module -> user_selected_inverter_cells_per_module,
     voltage level -> user_selected_inverter_voltage_level, Ah -> user_selected_inverter_ah_capacity.
     If no module count is set it is derived from the max design voltage (16 cells x 3.6 V per module). */
  static const int TOWERS = 1;             // battery towers in parallel (one emulated battery = one tower)
  static const int CELLS_PER_MODULE = 16;  // Deye HV modules are 16S LFP
  static const int VOLTS_PER_MODULE = 51;  // Deye reports 51 V per module as the voltage level
  static const int AH_CAPACITY = 40;       // per tower; the original battery reports 40 Ah

  static const int VOLTAGE_OFFSET_DV = 20;  // Small offset voltage to avoid generating voltage events
  static const int TICK_MS = 10;

  // Real-battery transmit schedule: frame, period in 10 ms ticks, ticks elapsed
  struct Slot {
    CAN_frame* frame;
    uint16_t period;
    uint16_t elapsed;
  };

  static CAN_frame ext(uint32_t id, uint8_t b0 = 0, uint8_t b1 = 0, uint8_t b2 = 0, uint8_t b3 = 0, uint8_t b4 = 0,
                       uint8_t b5 = 0, uint8_t b6 = 0, uint8_t b7 = 0) {
    return CAN_frame{.FD = false, .ext_ID = true, .DLC = 8, .ID = id, .data = {b0, b1, b2, b3, b4, b5, b6, b7}};
  }

  // Values not listed as "set in update_values()" are what the original battery sends.
  CAN_frame PYLON_4210 = ext(0x4210);  // set in update_values()
  CAN_frame PYLON_4220 = ext(0x4220);  // set in update_values()
  CAN_frame PYLON_4230 = ext(0x4230);  // set in update_values()
  CAN_frame PYLON_4240 = ext(0x4240);  // set in update_values()
  CAN_frame PYLON_4250 = ext(0x4250);  // set in update_values()
  CAN_frame PYLON_4260 = ext(0x4260);  // set in update_values()
  CAN_frame PYLON_4270 = ext(0x4270);  // set in update_values()
  CAN_frame PYLON_4280 = ext(0x4280);  // set in update_values()
  CAN_frame PYLON_4290 = ext(0x4290);  // fault extension: no faults
  CAN_frame PYLON_42E0 = ext(0x42E0);
  CAN_frame PYLON_42F0 = ext(0x42F0, 'D', 'e', 'y', 'e', '_', 'H', 'V', 0x00);
  CAN_frame PYLON_7310 = ext(0x7310, 0x01, 0x00, 0x50, 0x16, 0x50, 0x03, 0x00, 0x01);  // HW/SW versions
  CAN_frame PYLON_7320 = ext(0x7320);                                                  // set in update_values()
  CAN_frame PYLON_5210 = ext(0x5210);  // tower 1 summary, set in update_values()
  CAN_frame PYLON_5220 = ext(0x5220);  // tower cell voltage extremes, set in update_values()
  CAN_frame PYLON_5230 = ext(0x5230);  // tower cell temperature extremes, set in update_values()
  CAN_frame PYLON_5240 = ext(0x5240);  // tower current limits, set in update_values()
  CAN_frame PYLON_5250 = ext(0x5250, 0xF9);
  CAN_frame PYLON_5260 = ext(0x5260);
  CAN_frame PYLON_5270 = ext(0x5270);
  CAN_frame PYLON_5280 = ext(0x5280);  // voltage measurement points, set in update_values()
  CAN_frame PYLON_5290 = ext(0x5290);
  CAN_frame PYLON_52A0 = ext(0x52A0, 0x00, 0x00, 0x88, 0x13);  // byte 1 set in update_values()
  CAN_frame PYLON_52B0 = ext(0x52B0);
  CAN_frame PYLON_52C0 = ext(0x52C0);
  CAN_frame PYLON_52D0 = ext(0x52D0, 0x02, 0x50, 0x03, 0x00, 0x00, 0x06);
  CAN_frame PYLON_52E0 = ext(0x52E0, 0x02, 0x05, 0x01);
  CAN_frame PYLON_52F0 = ext(0x52F0);  // relay state bits, set in update_values()
  CAN_frame PYLON_5300 = ext(0x5300);  // 4 x 16-bit protection/alarm bitmaps: none active
  CAN_frame PYLON_5310 = ext(0x5310, 0x02, 0x00, 0x01);

  // Same order and periods as the battery firmware's dispatcher table.
  Slot schedule[30] = {
      {&PYLON_4210, 100, 0}, {&PYLON_4220, 100, 0}, {&PYLON_4230, 100, 0}, {&PYLON_4240, 100, 0}, {&PYLON_4250, 100, 0},
      {&PYLON_4260, 100, 0}, {&PYLON_4270, 100, 0}, {&PYLON_4280, 100, 0}, {&PYLON_4290, 100, 0}, {&PYLON_42E0, 500, 0},
      {&PYLON_42F0, 500, 0}, {&PYLON_7310, 500, 0}, {&PYLON_7320, 500, 0}, {&PYLON_5210, 500, 0}, {&PYLON_5220, 500, 0},
      {&PYLON_5230, 500, 0}, {&PYLON_5240, 500, 0}, {&PYLON_5250, 500, 0}, {&PYLON_5260, 500, 0}, {&PYLON_5270, 500, 0},
      {&PYLON_5280, 500, 0}, {&PYLON_5290, 500, 0}, {&PYLON_52A0, 500, 0}, {&PYLON_52B0, 500, 0}, {&PYLON_52C0, 500, 0},
      {&PYLON_52D0, 500, 0}, {&PYLON_52E0, 500, 0}, {&PYLON_52F0, 500, 0}, {&PYLON_5300, 100, 0}, {&PYLON_5310, 500, 0},
  };
  unsigned long previousTickMillis = 0;
  bool tickStarted = false;

  // Deye inverters send an 8-byte challenge on 0x4201; a Deye battery answers on 0x4202 with the
  // first 8 bytes of SHA-256(challenge). Not part of the public Pylontech HV protocol.
  CAN_frame PYLON_4202 = ext(0x4202);

  uint16_t discharge_cutoff_voltage_dV = 0;
  uint16_t charge_cutoff_voltage_dV = 0;
};

#endif
