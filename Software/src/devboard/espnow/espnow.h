#ifndef _ESPNOW_H_
#define _ESPNOW_H_

#include <stddef.h>
#include <stdint.h>

/* =====================================================================================
 * Battery Emulator ESP-NOW telemetry protocol, version 2
 * =====================================================================================
 *
 * Replaces the v1 protocol, which broadcast raw C structs copied byte-for-byte out of the
 * datalayer. That approach broke every receiver whenever a field was inserted, reordered
 * or resized, and it quantized the cell voltages to 20 mV to fit the 250 byte ESP-NOW v1
 * frame limit.
 *
 * v2 is a self-describing key/length/value (TLV) stream:
 *
 *   - Adding a new field never breaks an existing receiver. Unknown keys are skipped
 *     using the length that is always present in the record.
 *   - Adding a new *data type* never breaks an existing receiver either: the length is
 *     encoded in the tag independently of the type, so a parser that has never heard of
 *     a type can still skip past it. This is the property that lets the protocol grow
 *     without another compatibility break.
 *   - Fields that a given battery integration does not provide are simply not emitted,
 *     so receivers can distinguish "not supported" from "zero".
 *   - Cell voltages are transmitted as raw millivolts. No quantization, for all three
 *     batteries.
 *
 * Wire format
 * -----------
 * Every ESP-NOW packet is one frame: a 12 byte header followed by TLV records.
 *
 *   Header (12 bytes, little-endian):
 *     0   uint8   magic[0]           'B' (0x42)
 *     1   uint8   magic[1]           'E' (0x45)
 *     2   uint8   protocol_version   ESPNOW_PROTOCOL_VERSION (2)
 *     3   uint8   frame_type         see espnow_frame_type_t
 *     4   uint16  emulator_id        low 16 bits of the emulator's factory MAC
 *     6   uint8   battery_id         0 = emulator-wide, 1..3 = battery number
 *     7   uint8   flags              see ESPNOW_FLAG_*
 *     8   uint32  uptime_s           emulator uptime, seconds (detects reboots)
 *
 *   TLV record:
 *     uint8   key                    see espnow_key_t
 *     uint8   tag                    (type_class << 5) | length_code
 *     ...     length                 length_code < 30 : none, value length = length_code
 *                                    length_code == 30: uint8  length follows
 *                                    length_code == 31: uint16 length follows (LE)
 *     ...     value                  "length" bytes
 *
 * Receiver skip loop (this is all that is needed to stay forward compatible):
 *
 *     while (i + 2 <= len) {
 *       uint8_t key = buf[i++];
 *       uint8_t tag = buf[i++];
 *       uint8_t lc  = tag & 0x1F;
 *       uint16_t n;
 *       if      (lc < 30)  n = lc;
 *       else if (lc == 30) n = buf[i++];
 *       else             { n = buf[i] | (buf[i + 1] << 8); i += 2; }
 *       handle_or_ignore(key, tag >> 5, &buf[i], n);
 *       i += n;
 *     }
 *
 * Key 0xFF is reserved as an escape for a future 16 bit key space. The extended key is
 * carried INSIDE the value - its first two bytes, little-endian - and NOT between the key
 * and the tag. That keeps the tag as the second byte of every record, so the skip loop
 * above stays correct with no knowledge of extended keys at all: an old receiver simply
 * skips the record by its length, exactly as it would for any other unknown key. Putting
 * the extended key before the tag would desynchronise every receiver written before the
 * escape was used, which is the one failure mode this format exists to avoid.
 *
 * Compatibility rules for future changes to this file:
 *   - Never reuse or change the meaning of an allocated key. Retire it instead.
 *   - Never change the unit or scaling of an allocated key. Allocate a new key.
 *   - New keys, new type classes and new frame types may be added freely.
 *   - Bump ESPNOW_PROTOCOL_VERSION only for a change that violates the above.
 *
 * Frame size
 * ----------
 * ESP-NOW v2 (ESP-IDF 5.4+) raises the maximum payload from 250 to 1470 bytes, which is
 * what makes unquantized 16 bit cell voltages practical. v2 is assumed: every SoC the
 * emulator runs on supports it, so there is no runtime version negotiation.
 *
 * Note that the limit that matters is the RECEIVER's buffer, not its silicon. A receiver
 * that has not raised its own receive buffer above the 250 byte default will silently
 * drop larger frames - ESPHome's espnow component is one such case (max_payload_size).
 * The cell voltage array is therefore always split into index-tagged chunks sized by
 * ESPNOW_MAX_PAYLOAD below, so lowering that one constant is enough to talk to a 250 byte
 * receiver. Receivers must always honour ESPNOW_KEY_CELL_INDEX rather than assuming a
 * chunk starts at cell 0.
 * =====================================================================================
 */

#define ESPNOW_PROTOCOL_VERSION 2

#define ESPNOW_MAGIC_0 0x42 /* 'B' */
#define ESPNOW_MAGIC_1 0x45 /* 'E' */

#define ESPNOW_HEADER_SIZE 12

/* Header flags */
#define ESPNOW_FLAG_MORE_CHUNKS 0x01 /* another chunk of this frame_type follows */

/* Events are not streamed as they happen: the most recent ESPNOW_EVENT_REPLAY entries are
 * re-sent as a batch, so a receiver that starts after the emulator still gets the history.
 * Every frame of a batch carries ESPNOW_KEY_EVENT_INDEX and ESPNOW_KEY_EVENT_TOTAL, and all
 * but the last set ESPNOW_FLAG_MORE_CHUNKS. A receiver should therefore REPLACE its list on
 * each batch rather than append, and should not de-duplicate: repetition is intended. */
#define ESPNOW_EVENT_REPLAY 10

/* Maximum number of explicitly configured unicast receivers. Empty configuration falls
 * back to a single broadcast peer. */
#define ESPNOW_MAX_PEERS 8

/* TLV type classes (tag >> 5). Advisory only: a receiver never needs the type class to
 * parse the stream, only to interpret a key it already knows. */
enum espnow_type_t {
  ESPNOW_TYPE_UINT = 0,  /* unsigned integer, little-endian, 1/2/4/8 bytes */
  ESPNOW_TYPE_INT = 1,   /* signed two's complement, little-endian, 1/2/4/8 bytes */
  ESPNOW_TYPE_FLOAT = 2, /* IEEE-754 binary32, little-endian, 4 bytes */
  ESPNOW_TYPE_BOOL = 3,  /* 1 byte, 0 or 1 */
  ESPNOW_TYPE_STR = 4,   /* UTF-8 text, NOT NUL terminated */
  ESPNOW_TYPE_BYTES = 5, /* opaque octets */
  ESPNOW_TYPE_ARR16 = 6, /* array of little-endian uint16 */
  ESPNOW_TYPE_BITS = 7   /* bit array, LSB first within each byte */
};

/* Length codes 30 and 31 escape to an explicit 8 / 16 bit length. */
#define ESPNOW_LEN_CODE_U8 30
#define ESPNOW_LEN_CODE_U16 31
#define ESPNOW_LEN_CODE_MAX_INLINE 29

enum espnow_frame_type_t {
  ESPNOW_FRAME_SYSTEM = 0x01,  /* emulator-wide state, battery_id = 0 */
  ESPNOW_FRAME_BATTERY = 0x02, /* per-battery scalars, battery_id = 1..3 */
  ESPNOW_FRAME_CELLS = 0x03,   /* per-battery cell voltages + balancing bits */
  ESPNOW_FRAME_EVENT = 0x04    /* one emulator event */
};

/* -------------------------------------------------------------------------------------
 * Key registry. Keys are globally unique across all frame types so a receiver can use a
 * single dispatch table.
 *
 *   0x01..0x2F  emulator-wide
 *   0x30..0x4F  battery configuration / nameplate
 *   0x50..0x8F  battery live measurements
 *   0x90..0x9F  cell arrays
 *   0xA0..0xAF  events
 *   0xB0..0xEF  free for future upstream use
 *   0xF0..0xFE  reserved for private forks; upstream will never allocate here
 *   0xFF        escape for a future 16 bit key space - the real key is the first
 *               two bytes of the value (LE), so the record still skips correctly
 * ---------------------------------------------------------------------------------- */
enum espnow_key_t {
  /* ---- emulator-wide (ESPNOW_FRAME_SYSTEM) ---- */
  ESPNOW_KEY_FW_VERSION = 0x01,      /* STR   firmware version string */
  ESPNOW_KEY_HOSTNAME = 0x02,        /* STR   device hostname */
  ESPNOW_KEY_SOURCE_MAC = 0x03,      /* BYTES 6 bytes, factory MAC of the emulator */
  ESPNOW_KEY_SYSTEM_STATUS = 0x04,   /* UINT8 system_status_enum (types.h) */
  ESPNOW_KEY_PAUSE_STATUS = 0x05,    /* UINT8 battery_pause_status (safety.h) */
  ESPNOW_KEY_EVENT_LEVEL = 0x06,     /* UINT8 EVENTS_LEVEL_TYPE (events.h) */
  ESPNOW_KEY_EMULATOR_STATUS = 0x07, /* UINT8 EMULATOR_STATUS (events.h) */
  ESPNOW_KEY_CPU_TEMP_C = 0x08,      /* FLOAT degrees C, omitted if measurement disabled */
  ESPNOW_KEY_CPU_FREE_HEAP = 0x09,   /* UINT32 bytes */
  ESPNOW_KEY_BATTERY_COUNT = 0x0A,   /* UINT8  number of configured batteries, 1..3 */
  ESPNOW_KEY_WIFI_RSSI_DBM = 0x0B,   /* INT8   station RSSI, omitted when not associated */
  ESPNOW_KEY_INVERTER_ALIVE = 0x0C,  /* UINT8  inverter keepalive countdown */
  ESPNOW_KEY_CONTACTORS = 0x0D,      /* UINT8  0 = starting up, 1 = engaged, 2 = opened */
  ESPNOW_KEY_DC_BUS_LIVE = 0x0E,     /* BOOL   DC bus energized towards the inverter */
  ESPNOW_KEY_EQUIPMENT_STOP = 0x0F,  /* BOOL   equipment stop latched */
  ESPNOW_KEY_IP_ADDRESS = 0x10,      /* BYTES  4 octets, IPv4 in display order (octets[0]
                                        is the leading octet); omitted when the station
                                        is not associated */
  ESPNOW_KEY_SSID = 0x11,            /* STR    SSID the station is joined to; omitted when
                                        not associated */
  ESPNOW_KEY_AP_ACTIVE = 0x12,       /* BOOL   the emulator's own access point is up right
                                        now - reflects the live Wi-Fi mode, so it goes
                                        false once the AP is torn down on provisioning
                                        timeout even though the setting stays enabled */

  /* ---- battery nameplate (ESPNOW_FRAME_BATTERY) ---- */
  ESPNOW_KEY_NUMBER_OF_CELLS = 0x30,       /* UINT8  cells in the pack */
  ESPNOW_KEY_CHEMISTRY = 0x31,             /* UINT8  battery_chemistry_enum (types.h) */
  ESPNOW_KEY_TOTAL_CAPACITY_WH = 0x32,     /* UINT32 Wh */
  ESPNOW_KEY_REPORTED_CAPACITY_WH = 0x33,  /* UINT32 Wh, as presented to the inverter */
  ESPNOW_KEY_MAX_DESIGN_VOLTAGE_DV = 0x34, /* UINT16 deciVolt */
  ESPNOW_KEY_MIN_DESIGN_VOLTAGE_DV = 0x35, /* UINT16 deciVolt */
  ESPNOW_KEY_MAX_CELL_DESIGN_MV = 0x36,    /* UINT16 mV */
  ESPNOW_KEY_MIN_CELL_DESIGN_MV = 0x37,    /* UINT16 mV */
  ESPNOW_KEY_MAX_CELL_DEVIATION_MV = 0x38, /* UINT16 mV */

  /* ---- battery live values (ESPNOW_FRAME_BATTERY) ---- */
  ESPNOW_KEY_SOC_PPTT = 0x50,                 /* UINT16 0.01 %, scaled/reported SOC */
  ESPNOW_KEY_SOC_REAL_PPTT = 0x51,            /* UINT16 0.01 %, real SOC from the BMS */
  ESPNOW_KEY_SOH_PPTT = 0x52,                 /* UINT16 0.01 % */
  ESPNOW_KEY_VOLTAGE_DV = 0x53,               /* UINT16 deciVolt */
  ESPNOW_KEY_CURRENT_DA = 0x54,               /* INT16  deciAmpere, + = charging */
  ESPNOW_KEY_REPORTED_CURRENT_DA = 0x55,      /* INT16  deciAmpere, all batteries summed */
  ESPNOW_KEY_ACTIVE_POWER_W = 0x56,           /* INT32  W, + = charging */
  ESPNOW_KEY_REMAINING_CAPACITY_WH = 0x57,    /* UINT32 Wh, real */
  ESPNOW_KEY_REPORTED_REMAIN_WH = 0x58,       /* UINT32 Wh, as presented to the inverter */
  ESPNOW_KEY_MAX_CHARGE_POWER_W = 0x59,       /* UINT32 W */
  ESPNOW_KEY_MAX_DISCHARGE_POWER_W = 0x5A,    /* UINT32 W */
  ESPNOW_KEY_MAX_CHARGE_CURRENT_DA = 0x5B,    /* UINT16 deciAmpere */
  ESPNOW_KEY_MAX_DISCHARGE_CURRENT_DA = 0x5C, /* UINT16 deciAmpere */
  ESPNOW_KEY_OVERRIDE_CHARGE_W = 0x5D,        /* UINT32 W, user override */
  ESPNOW_KEY_OVERRIDE_DISCHARGE_W = 0x5E,     /* UINT32 W, user override */
  ESPNOW_KEY_CELL_MAX_MV = 0x5F,              /* UINT16 mV */
  ESPNOW_KEY_CELL_MIN_MV = 0x60,              /* UINT16 mV */
  ESPNOW_KEY_TEMPERATURE_MAX_DC = 0x61,       /* INT16  0.1 degrees C */
  ESPNOW_KEY_TEMPERATURE_MIN_DC = 0x62,       /* INT16  0.1 degrees C */
  ESPNOW_KEY_TOTAL_CHARGED_WH = 0x63,         /* INT32  Wh lifetime */
  ESPNOW_KEY_TOTAL_DISCHARGED_WH = 0x64,      /* INT32  Wh lifetime */
  ESPNOW_KEY_INSULATION_KOHM = 0x65,          /* UINT16 kOhm, omitted until a valid sample */
  ESPNOW_KEY_BALANCING_STATUS = 0x66,         /* UINT8  balancing_status_enum (types.h) */
  ESPNOW_KEY_BALANCING_ACTIVE_CELLS = 0x67,   /* UINT16 count of shunts currently on */
  ESPNOW_KEY_CHARGING_STATE = 0x68,           /* UINT8  ChargingState (types.h) */
  ESPNOW_KEY_LIMITING_FACTOR = 0x69,          /* UINT8  LimitingFactor (types.h) */
  ESPNOW_KEY_REAL_BMS_STATUS = 0x6A,          /* UINT8  real_bms_status_enum (types.h) */
  ESPNOW_KEY_CAN_ALIVE = 0x6B,                /* UINT8  battery keepalive countdown */
  ESPNOW_KEY_CAN_ERROR_COUNTER = 0x6C,        /* UINT16 CAN CRC error count */
  ESPNOW_KEY_LED_MODE = 0x6D,                 /* UINT8  led_mode_enum (types.h) */
  ESPNOW_KEY_BATTERY_DETECTED = 0x6E,         /* BOOL   at least one frame ever received */
  ESPNOW_KEY_DCDC_CURRENT_DA = 0x6F,          /* INT16  deciAmpere, Tesla only */
  ESPNOW_KEY_DCDC_VOLTAGE_MV = 0x70,          /* UINT16 mV, Tesla only */
  ESPNOW_KEY_AUTOCAL_TAPER = 0x71,            /* BOOL   BYD Atto 3 only */
  ESPNOW_KEY_AUTOCAL_DWELL_S = 0x72,          /* UINT32 s, BYD Atto 3 only */
  ESPNOW_KEY_AUTOCAL_COOLDOWN_READY = 0x73,   /* BOOL BYD Atto 3 only */
  ESPNOW_KEY_AUTOCAL_SOC_DRIFT = 0x74,        /* FLOAT %, BYD Atto 3 only */

  /* ---- cell arrays (ESPNOW_FRAME_CELLS) ---- */
  ESPNOW_KEY_CELL_COUNT = 0x90,       /* UINT16 total cells in this battery */
  ESPNOW_KEY_CELL_INDEX = 0x91,       /* UINT16 zero-based index of the first cell below */
  ESPNOW_KEY_CELL_VOLTAGES_MV = 0x92, /* ARR16  raw mV, one entry per cell, unquantized */
  ESPNOW_KEY_CELL_BALANCING = 0x93,   /* BITS   one bit per cell, set = shunt on */

  /* ---- events (ESPNOW_FRAME_EVENT) ---- */
  ESPNOW_KEY_EVENT_ID = 0xA0,       /* UINT16 EVENTS_ENUM_TYPE ordinal */
  ESPNOW_KEY_EVENT_NAME = 0xA1,     /* STR    symbolic name, e.g. "EVENT_CAN_RX_FAILURE" */
  ESPNOW_KEY_EVENT_SEVERITY = 0xA2, /* UINT8  EVENTS_LEVEL_TYPE */
  ESPNOW_KEY_EVENT_STATE = 0xA3,    /* UINT8  EVENTS_STATE_TYPE */
  ESPNOW_KEY_EVENT_COUNT = 0xA4,    /* UINT8  occurrences since boot */
  /* 0xA5 RETIRED. Was "UINT8 event specific payload byte". The payload is signed and wider
     than a byte (deci-Celsius temperatures), so per the compatibility rules above the key was
     retired rather than redefined, and ESPNOW_KEY_EVENT_DATA_I16 allocated in its place. */
  ESPNOW_KEY_EVENT_MILLIS = 0xA6,  /* UINT64 millis64() at the last occurrence */
  ESPNOW_KEY_EVENT_MESSAGE = 0xA7, /* STR    human readable description, " (Battery N)" appended
                                        when the event refers to one specific pack */
  ESPNOW_KEY_EVENT_INDEX = 0xA8,   /* UINT8  position in the replay batch, 0 = most recent */
  ESPNOW_KEY_EVENT_TOTAL = 0xA9,   /* UINT8  events in this replay batch, 1..ESPNOW_EVENT_REPLAY */
  ESPNOW_KEY_EVENT_DATA_I16 = 0xAA /* INT16  event specific payload, replaces retired 0xA5 */
};

void init_espnow();
void update_espnow();

#endif  // _ESPNOW_H_
