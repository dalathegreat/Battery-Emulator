#include "../include.h"
#ifdef BYD_KOSTAL_RS485
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "KOSTAL-RS485.h"

#define RS485_HEALTHY \
  12  // How many value updates we can go without inverter gets reported as missing \
      // e.g. value set to 12, 12*5sec=60seconds without comm before event is raised
static const uint8_t KOSTAL_FRAMEHEADER[5] = {0x62, 0xFF, 0x02, 0xFF, 0x29};
static const uint8_t KOSTAL_FRAMEHEADER2[5] = {0x63, 0xFF, 0x02, 0xFF, 0x29};
static uint16_t nominal_voltage_dV = 0;
static int16_t average_temperature_dC = 0;
static uint8_t incoming_message_counter = RS485_HEALTHY;
static int8_t f2_startup_count = 0;

static boolean B1_delay = false;
static unsigned long B1_last_millis = 0;
static unsigned long currentMillis;
static unsigned long startupMillis = 0;
static unsigned long contactorMillis = 0;

static boolean RX_allow = false;

union f32b {
  float f;
  byte b[4];
};

uint8_t BATTERY_INFO[40] = {
    0x06, 0xE2, 0xFF, 0x02, 0xFF, 0x29,  // Frame header
    0x01, 0x08, 0x80, 0x43,              // 256.063 Nominal voltage / 5*51.2=256      first byte 0x01 or 0x04
    0xE4, 0x70, 0x8A, 0x5C,              // These might be Umin & Unax, Uint16
    0xB5, 0x02, 0xD3, 0x01,              // Battery Serial number? Modbus register 527
    0x01, 0x05, 0xC8, 0x41,              // 25.0024  ?
    0xC2, 0x18,                          // Battery Firmware, modbus register 586
    0x01, 0x03, 0x59, 0x42,              // 0x00005942 = 54.25 ??
    0x01, 0x01, 0x01, 0x02, 0x05, 0x02, 0xA0, 0x01, 0x01, 0x02,
    0x4D,   // CRC
    0x00};  //

// values in CyclicData will be overwritten at update_modbus_registers_inverter()

uint8_t CyclicData[64] = {
    0x00,                          // First zero byte pointer
    0xE2, 0xFF, 0x02, 0xFF, 0x29,  // frame Header
    0x1D, 0x5A, 0x85, 0x43,        // Current Voltage  (float)                        Modbus register 216, Bytes 6-9
    0x00, 0x00, 0x8D, 0x43,        // Max Voltage      (2 byte float),                                     Bytes 12-13
    0x00, 0x00, 0xAC, 0x41,        // BAttery Temperature        (2 byte float)       Modbus register 214, Bytes 16-17
    0x00, 0x00, 0x00, 0x00,        // Peak Current (1s period?),  Bytes 18-21
    0x00, 0x00, 0x00, 0x00,        // Avg current  (1s period?),  Bytes 22-25
    0x00, 0x00, 0x48, 0x42,        // Max discharge current (2 byte float), Bit 26-29,  // Sunspec: ADisChaMax
    0x00, 0x00, 0xC8, 0x41,        // Battery gross capacity, Ah (2 byte float) , Bytes 30-33, Modbus 512
    0x00, 0x00, 0xA0, 0x41,        // Max charge current (2 byte float) Bit 36-37, ZERO WHEN SOC=100 // Sunspec: AChaMax
    0xCD, 0xCC, 0xB4, 0x41,        // MaxCellTemp (4 byte float) Bit 38-41
    0x00, 0x00, 0xA4, 0x41,        // MinCellTemp (4 byte float) Bit 42-45
    0xA4, 0x70, 0x55, 0x40,        // MaxCellVolt  (float), Bit 46-49
    0x7D, 0x3F, 0x55, 0x40,        // MinCellVolt  (float), Bit 50-53

    0xFE, 0x04,  // Cycle count,
    0x00,        // Byte 56
    0x40,        // When SOC=100 Byte57=0x40, at startup 0x03 (about 7 times), otherwise 0x02
    0x64,        // SOC , Bit 58
    0x00,        // Unknown,
    0x00,        // Unknown,
    0x02,        // Unknown, Mostly 0x02. seen also 0x01
    0x00,        // CRC (inverted sum of bytes 1-62 + 0xC0), Bit 62
    0x00};

// FE 04 01 40 xx 01 01 02 yy (fully charged)
// FE 02 01 02 xx 01 01 02 yy (charging or discharging)

uint8_t frame3[9] = {
    0x08, 0xE2, 0xFF, 0x02, 0xFF, 0x29,  //header
    0x06,                                //Unknown
    0xEF,                                //CRC
    0x00                                 //endbyte
};

uint8_t frame4[8] = {0x07, 0xE3, 0xFF, 0x02, 0xFF, 0x29, 0xF4, 0x00};

uint8_t frameB1[10] = {0x07, 0x63, 0xFF, 0x02, 0xFF, 0x29, 0x5E, 0x02, 0x16, 0x00};
uint8_t frameB1b[8] = {0x07, 0xE3, 0xFF, 0x02, 0xFF, 0x29, 0xF4, 0x00};

uint8_t RS485_RXFRAME[300];

bool register_content_ok = false;

void float2frame(byte* arr, float value, byte framepointer) {
  f32b g;
  g.f = value;
  arr[framepointer] = g.b[0];
  arr[framepointer + 1] = g.b[1];
  arr[framepointer + 2] = g.b[2];
  arr[framepointer + 3] = g.b[3];
}

static void dbg_timestamp(void) {
#ifdef DEBUG_KOSTAL_RS485_DATA
  logging.print("[");
  logging.print(millis());
  logging.print(" ms] ");
#endif
}

static void dbg_frame(byte* frame, int len, const char* prefix) {
  dbg_timestamp();
#ifdef DEBUG_KOSTAL_RS485_DATA
  logging.print(prefix);
  logging.print(": ");
  for (uint8_t i = 0; i < len; i++) {
    if (frame[i] < 0x10) {
      logging.print("0");
    }
    logging.print(frame[i], HEX);
    logging.print(" ");
  }
  logging.println("");
#endif
}

static void dbg_message(const char* msg) {
  dbg_timestamp();
#ifdef DEBUG_KOSTAL_RS485_DATA
  logging.println(msg);
#endif
}

/* https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples */

void null_stuffer(byte* lfc, int len) {
  int last_null_byte = 0;
  for (int i = 0; i < len; i++) {
    if (lfc[i] == '\0') {
      lfc[last_null_byte] = (byte)(i - last_null_byte);
      last_null_byte = i;
    }
  }
}

static void send_kostal(byte* frame, int len) {
  dbg_frame(frame, len, "TX");
  Serial2.write(frame, len);
}

byte calculate_kostal_crc(byte* lfc, int len) {
  unsigned int sum = 0;
  if (lfc[0] != 0) {
    logging.printf("WARNING: first byte should be 0, but is 0x%02x\n", lfc[0]);
  }
  for (int i = 1; i < len; i++) {
    sum += lfc[i];
  }
  return (byte)(-sum & 0xff);
}

byte calculate_frame1_crc(byte* lfc, int lastbyte) {
  unsigned int sum = 0;
  for (int i = 0; i < lastbyte; ++i) {
    sum += lfc[i];
  }
  return ((byte) ~(sum - 0x28) & 0xff);
}

bool check_kostal_frame_crc() {
  unsigned int sum = 0;
  for (int i = 1; i < 8; ++i) {
    sum += RS485_RXFRAME[i];
  }
  if (((~sum + 1) & 0xff) == (RS485_RXFRAME[8] & 0xff)) {
    return (true);
  } else {
    return (false);
  }
}

void update_RS485_registers_inverter() {

  average_temperature_dC =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);
  if (datalayer.battery.status.temperature_min_dC < 0) {
    average_temperature_dC = 0;
  }

  if (datalayer.system.status.battery_allows_contactor_closing &
      datalayer.system.status.inverter_allows_contactor_closing) {
    float2frame(CyclicData, (float)datalayer.battery.status.voltage_dV / 10, 6);  // Confirmed OK mapping
    CyclicData[0] = 0x0A;
  } else {
    CyclicData[0] = 0x06;
    float2frame(CyclicData, 0.0, 6);
  }
  // Set nominal voltage to value between min and max voltage set by battery (Example 400 and 300 results in 350V)
  nominal_voltage_dV =
      (((datalayer.battery.info.max_design_voltage_dV - datalayer.battery.info.min_design_voltage_dV) / 2) +
       datalayer.battery.info.min_design_voltage_dV);
  float2frame(BATTERY_INFO, (float)nominal_voltage_dV / 10, 6);

  float2frame(CyclicData, (float)datalayer.battery.info.max_design_voltage_dV / 10, 10);

  float2frame(CyclicData, (float)average_temperature_dC / 10, 14);

  //  Some current values causes communication error, must be resolved, why.
  //  float2frame(CyclicData, (float)datalayer.battery.status.current_dA / 10, 18);  // Peak discharge? current (2 byte float)
  //  float2frame(CyclicData, (float)datalayer.battery.status.current_dA / 10, 22);

  float2frame(CyclicData, (float)datalayer.battery.status.max_discharge_current_dA / 10, 26);

  // When SOC = 100%, drop down allowed charge current down.

  if ((datalayer.battery.status.reported_soc / 100) < 100) {
    float2frame(CyclicData, (float)datalayer.battery.status.max_charge_current_dA / 10, 34);
  } else {
    float2frame(CyclicData, 0.0, 34);
  }

  // On startup, byte 56 seems to be always 0x00 couple of frames,.
  if (f2_startup_count < 9) {
    CyclicData[56] = 0x00;
  } else {
    CyclicData[56] = 0x01;
  }

  // On startup, byte 59 seems to be always 0x02 couple of frames,.
  if (f2_startup_count < 14) {
    CyclicData[59] = 0x02;
  } else {
    CyclicData[59] = 0x00;
  }

  if (nominal_voltage_dV > 0) {
    float2frame(CyclicData, (float)(datalayer.battery.info.total_capacity_Wh / nominal_voltage_dV * 10),
                30);  // BAttery capacity Ah
  }
  float2frame(CyclicData, (float)datalayer.battery.status.temperature_max_dC / 10, 38);
  float2frame(CyclicData, (float)datalayer.battery.status.temperature_min_dC / 10, 42);

  float2frame(CyclicData, (float)datalayer.battery.status.cell_max_voltage_mV / 1000, 46);
  float2frame(CyclicData, (float)datalayer.battery.status.cell_min_voltage_mV / 1000, 50);

  CyclicData[58] = (byte)(datalayer.battery.status.reported_soc / 100);  // Confirmed OK mapping

  register_content_ok = true;

  BATTERY_INFO[38] = calculate_frame1_crc(BATTERY_INFO, 38);

  if (incoming_message_counter > 0) {
    incoming_message_counter--;
  }

  if (incoming_message_counter == 0) {
    set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  } else {
    clear_event(EVENT_MODBUS_INVERTER_MISSING);
  }
}

static uint8_t rx_index = 0;

void receive_RS485()  // Runs as fast as possible to handle the serial stream
{
  currentMillis = millis();

  if (datalayer.system.status.battery_allows_contactor_closing & !contactorMillis) {
    contactorMillis = currentMillis;
  }
  if (currentMillis - contactorMillis >= INTERVAL_2_S & !RX_allow) {
    RX_allow = true;
    dbg_message("RX_allow -> true");
  }

  if (startupMillis) {
    if (((currentMillis - startupMillis) >= INTERVAL_2_S & currentMillis - startupMillis <= 7000) &
        datalayer.system.status.inverter_allows_contactor_closing) {
      // Disconnect allowed only, when curren zero
      if (datalayer.battery.status.current_dA == 0) {
        datalayer.system.status.inverter_allows_contactor_closing = false;
        dbg_message("inverter_allows_contactor_closing -> false");
      }
    } else if (((currentMillis - startupMillis) >= 7000) &
               datalayer.system.status.inverter_allows_contactor_closing == false) {
      datalayer.system.status.inverter_allows_contactor_closing = true;
      dbg_message("inverter_allows_contactor_closing -> true");
    }
  }

  if (B1_delay) {
    if ((currentMillis - B1_last_millis) > INTERVAL_1_S) {
      send_kostal(frameB1b, 8);
      B1_delay = false;
      dbg_message("B1_delay -> false");
    }
  } else if (Serial2.available()) {
    RS485_RXFRAME[rx_index] = Serial2.read();
    if (RX_allow) {
      rx_index++;
      if (RS485_RXFRAME[rx_index - 1] == 0x00) {
        if ((rx_index == 10) && (RS485_RXFRAME[0] == 0x09) && register_content_ok) {
          dbg_frame(RS485_RXFRAME, 10, "RX");
          rx_index = 0;
          if (check_kostal_frame_crc()) {
            incoming_message_counter = RS485_HEALTHY;
            bool headerA = true;
            bool headerB = true;
            for (uint8_t i = 0; i < 5; i++) {
              if (RS485_RXFRAME[i + 1] != KOSTAL_FRAMEHEADER[i]) {
                headerA = false;
              }
              if (RS485_RXFRAME[i + 1] != KOSTAL_FRAMEHEADER2[i]) {
                headerB = false;
              }
            }

            // "frame B1", maybe reset request, seen after battery power on/partial data
            if (headerB && (RS485_RXFRAME[6] == 0x5E) && (RS485_RXFRAME[7] == 0xFF)) {
              send_kostal(frameB1, 10);
              B1_delay = true;
              dbg_message("B1_delay -> true");
              B1_last_millis = currentMillis;
            }

            // "frame B1", maybe reset request, seen after battery power on/partial data
            if (headerB && (RS485_RXFRAME[6] == 0x5E) && (RS485_RXFRAME[7] == 0x04)) {
              send_kostal(frame4, 8);
              // This needs more reverse engineering, disabled...
            }

            if (headerA && (RS485_RXFRAME[6] == 0x4A) && (RS485_RXFRAME[7] == 0x08)) {  // "frame 1"
              send_kostal(BATTERY_INFO, 40);
              if (!startupMillis) {
                startupMillis = currentMillis;
              }
            }
            if (headerA && (RS485_RXFRAME[6] == 0x4A) && (RS485_RXFRAME[7] == 0x04)) {  // "frame 2"
              update_values_battery();
              update_RS485_registers_inverter();
              if (f2_startup_count < 15) {
                f2_startup_count++;
              }
              byte tmpframe[64];  //copy values to prevent data manipulation during rewrite/crc calculation
              memcpy(tmpframe, CyclicData, 64);

              tmpframe[62] = calculate_kostal_crc(tmpframe, 62);
              null_stuffer(tmpframe, 64);
              send_kostal(tmpframe, 64);
            }
            if (headerA && (RS485_RXFRAME[6] == 0x53) && (RS485_RXFRAME[7] == 0x03)) {  // "frame 3"
              send_kostal(frame3, 9);
            }
          }
        } else {
          dbg_frame(RS485_RXFRAME, 10, "RX (dropped)");
        }
        rx_index = 0;
      }
    }
    if (rx_index >= 10) {
      dbg_frame(RS485_RXFRAME, 10, "RX (!RX_allow)");
      rx_index = 0;
    }
  }
}

void setup_inverter(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.inverter_protocol, "BYD battery via Kostal RS485", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}

#endif
