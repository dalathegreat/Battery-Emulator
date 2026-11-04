#ifndef GIVENERGY_LV_FRAMES_H
#define GIVENERGY_LV_FRAMES_H

#include <stddef.h>
#include <stdint.h>

// Wire format of the GivEnergy LV battery bus as a G3 inverter polls it.
// Modbus RTU, except that FC4 replies echo the start address where standard
// Modbus has a byte count. Sources: bms-analysis docs/01 to docs/03, checked
// against a G3 capture (September 2026).
namespace givenergy_lv {

constexpr size_t kRequestLen = 8;
constexpr size_t kMaxReplyLen = 61;  // FC3 reply to HR0..HR27
constexpr uint8_t kPackDevice = 1;   // the pack; 2 to 5 are empty slots
constexpr uint8_t kLastDevice = 5;
constexpr uint16_t kHoldingCount = 28;

// The three input-register blocks the inverter reads from each device.
constexpr uint16_t kBlock1Start = 0x00, kBlock1Count = 21;
constexpr uint16_t kBlock2Start = 0x15, kBlock2Count = 19;
constexpr uint16_t kBlock3Start = 0x28, kBlock3Count = 20;

uint16_t crc16(const uint8_t* data, size_t len);

// True for an 8-byte FC3, FC4 or FC6 request with a valid CRC.
bool is_request(const uint8_t* frame);

// Every value the battery puts on the wire, already in wire units. The
// inverter module fills it from the datalayer; the tests fill it from a
// capture.
struct Snapshot {
  char serial[11];    // 10 ASCII characters
  uint16_t firmware;  // HR13 and IR block 2. 3011 or higher makes the G3 use HR26/HR27
  // Holding registers
  uint16_t capacity_Ah;         // HR11
  uint32_t clock_hash;          // HR17 (low half) and HR18 (high half); ticks once a second
  uint16_t status;              // HR19
  uint16_t alarms;              // HR20
  uint16_t soc_pct;             // HR21
  uint16_t voltage_cV;          // HR22, 0.01 V
  int16_t current_cA;           // HR23, 0.01 A, positive = charging
  int16_t temperature_C;        // HR24
  uint16_t limit_cA;            // HR25, only used by the G3 below firmware 3011
  uint16_t charge_limit_cA;     // HR26
  uint16_t discharge_limit_cA;  // HR27
  // IR block 1
  int16_t sensor_temps_dC[5];
  // IR block 2
  uint8_t cell_count;
  uint16_t cycles;
  uint16_t pack_voltage_mV;
  uint16_t cell_sum_mV;
  int32_t current_mA;
  uint32_t full_capacity_cAh;
  uint32_t design_capacity_cAh;
  uint32_t remaining_cAh;
  uint8_t block2_soc_pct;  // the inverter stops discharging when this reaches its 4% floor
  uint16_t block2_word28;  // output status and current-direction latch bytes; 0x0E10 or 0x0610 on a real battery
  uint16_t block2_word32;  // capacity latch and voltage/current limit bytes; 0, or 4 at the pack over-voltage warning
  // IR block 3
  uint16_t cells_mV[16];
  int16_t temp_max_dC;
  int16_t temp_min_dC;
  uint16_t cell_max_mV;
  uint16_t cell_min_mV;
};

// Writes the reply to an 8-byte request into out (kMaxReplyLen bytes) and
// returns its length, or 0 when the battery would stay silent.
size_t build_reply(const uint8_t* request, const Snapshot& s, uint8_t* out);

}  // namespace givenergy_lv

#endif
