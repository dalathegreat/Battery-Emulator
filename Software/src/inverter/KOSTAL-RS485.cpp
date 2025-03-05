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

static uint16_t rx_index = 0;
static boolean RX_allow = false;

union f32b {
  float f;
  byte b[4];
};

// clang-format off
uint8_t BATTERY_INFO[40] = {
    0x00,                         // First zero byte pointer
    0xE2, 0xFF, 0x02, 0xFF, 0x29, // Frame header
    0x00, 0x00, 0x80, 0x43,       // 256.063 Nominal voltage / 5*51.2=256
    0xE4, 0x70, 0x8A, 0x5C,       // Manufacture date (Epoch time) (BYD: GetBatteryInfo this[0x10ac])
    0xB5, 0x00, 0xD3, 0x00,       // Battery Serial number? Modbus register 527 - 0x10b0
    0x00, 0x00, 0xC8, 0x41,       // Nominal Capacity (0x10b4)
    0xC2, 0x18,                   // Battery Firmware, modbus register 586  (0x10b8)
    0x00,                         // Static (BYD: GetBatteryInfo this[0x10ba])
    0x00,                         // ?
    0x59, 0x42,                   // Vendor identifier
                                  //       0x59 0x42 -> 'YB' -> BYD
                                  //       0x59 0x44 -> 'YD' -> Dyness
    0x00, 0x00,                   // Static (BYD: GetBatteryInfo this[0x10be])
    0x00, 0x00,
    0x05, 0x00,                   // Number of blocks in series (uint16)
    0xA0, 0x00, 0x00, 0x00,
    0x4D, // CRC
    0x00};
// clang-format on

// values in CYCLIC_DATA will be overwritten at update_modbus_registers_inverter()

// clang-format off
uint8_t CYCLIC_DATA[64] = {
    0x00,                          // First zero byte pointer
    0xE2, 0xFF, 0x02, 0xFF, 0x29,  // Frame header
    0x1D, 0x5A, 0x85, 0x43,        // Current Voltage            (float32)   Bytes  6- 9    Modbus register 216
    0x00, 0x00, 0x8D, 0x43,        // Max Voltage                (float32)   Bytes 10-13
    0x00, 0x00, 0xAC, 0x41,        // Battery Temperature        (float32)   Bytes 14-17    Modbus register 214
    0x00, 0x00, 0x00, 0x00,        // Peak Current (1s period?)  (float32)   Bytes 18-21
    0x00, 0x00, 0x00, 0x00,        // Avg current  (1s period?)  (float32)   Bytes 22-25
    0x00, 0x00, 0x48, 0x42,        // Max discharge current      (float32)   Bytes 26-29    Sunspec: ADisChaMax
    0x00, 0x00, 0xC8, 0x41,        // Battery gross capacity, Ah (float32)   Bytes 30-33    Modbus register 512
    0x00, 0x00, 0xA0, 0x41,        // Max charge current         (float32)   Bytes 34-37    0.0f when SoC is 100%, Sunspec: AChaMax
    0xCD, 0xCC, 0xB4, 0x41,        // MaxCellTemp                (float32)   Bytes 38-41
    0x00, 0x00, 0xA4, 0x41,        // MinCellTemp                (float32)   Bytes 42-45
    0xA4, 0x70, 0x55, 0x40,        // MaxCellVolt                (float32)   Bytes 46-49
    0x7D, 0x3F, 0x55, 0x40,        // MinCellVolt                (float32)   Bytes 50-53

    0xFE, 0x04,  // Bytes 54-55, Cycle count (uint16)
    0x00,        // Byte 56, charge/discharge control, 0=disable, 1=enable
    0x00,        // Byte 57, When SoC is 100%, seen as 0x40
    0x64,        // Byte 58, SoC (uint8)
    0x00,        // Byte 59, Unknown
    0x00,        // Byte 60, Unknown
    0x01,        // Byte 61, Unknown, 1 only at first frame, 0 otherwise
    0x00,        // Byte 62, CRC
    0x00};
// clang-format on

// FE 04 01 40 xx 01 01 02 yy (fully charged)
// FE 02 01 02 xx 01 01 02 yy (charging or discharging)

uint8_t STATUS_FRAME[9] = {
    0x00, 0xE2, 0xFF, 0x02, 0xFF, 0x29,  //header
    0x06,                                //Unknown (battery status/error?)
    0xEF,                                //CRC
    0x00                                 //endbyte
};

uint8_t ACK_FRAME[8] = {0x07, 0xE3, 0xFF, 0x02, 0xFF, 0x29, 0xF4, 0x00};

uint8_t RS485_RXFRAME[300];

bool register_content_ok = false;

static void float2frame(byte* arr, float value, byte framepointer) {
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
#ifdef DEBUG_KOSTAL_RS485_DATA_USB
  Serial.print(prefix);
  Serial.print(": ");
  for (uint8_t i = 0; i < len; i++) {
    if (frame[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
#endif
}

static void dbg_message(const char* msg) {
  dbg_timestamp();
#ifdef DEBUG_KOSTAL_RS485_DATA
  logging.println(msg);
#endif
}

/* https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples */

static void null_stuffer(byte* lfc, int len) {
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

static byte calculate_kostal_crc(byte* lfc, int len) {
  unsigned int sum = 0;
  if (lfc[0] != 0) {
    logging.printf("WARNING: first byte should be 0, but is 0x%02x\n", lfc[0]);
  }
  for (int i = 1; i < len; i++) {
    sum += lfc[i];
  }
  return (byte)(-sum & 0xff);
}

static bool check_kostal_frame_crc(int len) {
  unsigned int sum = 0;
  int zeropointer = RS485_RXFRAME[0];
  int last_zero = 0;
  for (int i = 1; i < len - 2; ++i) {
    if (i == zeropointer + last_zero) {
      zeropointer = RS485_RXFRAME[i];
      last_zero = i;
      RS485_RXFRAME[i] = 0x00;
    }
    sum += RS485_RXFRAME[i];
  }

  if ((-sum & 0xff) == (RS485_RXFRAME[len - 2] & 0xff)) {
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
    float2frame(CYCLIC_DATA, (float)datalayer.battery.status.voltage_dV / 10, 6);  // Confirmed OK mapping
  } else {
    float2frame(CYCLIC_DATA, 0.0, 6);
  }
  // Set nominal voltage to value between min and max voltage set by battery (Example 400 and 300 results in 350V)
  nominal_voltage_dV =
      (((datalayer.battery.info.max_design_voltage_dV - datalayer.battery.info.min_design_voltage_dV) / 2) +
       datalayer.battery.info.min_design_voltage_dV);
  float2frame(BATTERY_INFO, (float)nominal_voltage_dV / 10, 6);

  float2frame(CYCLIC_DATA, (float)datalayer.battery.info.max_design_voltage_dV / 10, 10);

  float2frame(CYCLIC_DATA, (float)average_temperature_dC / 10, 14);

#ifdef BMW_SBOX
  float2frame(CYCLIC_DATA, (float)(datalayer.shunt.measured_amperage_mA / 100) / 10, 18);
  float2frame(CYCLIC_DATA, (float)(datalayer.shunt.measured_avg1S_amperage_mA / 100) / 10, 22);

  if (datalayer.shunt.contactors_engaged) {
    CYCLIC_DATA[59] = 0;
  } else {
    CYCLIC_DATA[59] = 2;
  }

  if (datalayer.shunt.precharging || datalayer.shunt.contactors_engaged) {
    CYCLIC_DATA[56] = 1;
    float2frame(CYCLIC_DATA, (float)datalayer.battery.status.max_discharge_current_dA / 10,
                26);  // Maximum discharge current
    float2frame(CYCLIC_DATA, (float)datalayer.battery.status.max_charge_current_dA / 10, 34);  // Maximum charge current
  } else {
    CYCLIC_DATA[56] = 0;
    float2frame(CYCLIC_DATA, 0.0, 26);
    float2frame(CYCLIC_DATA, 0.0, 34);
  }

#else

  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.current_dA / 10, 18);  // Last current
  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.current_dA / 10, 22);  // Should be Avg current(1s)

  // On startup, byte 56 seems to be always 0x00 couple of frames,.
  if (f2_startup_count < 9) {
    CYCLIC_DATA[56] = 0x00;
  } else {
    CYCLIC_DATA[56] = 0x01;
  }

  // On startup, byte 59 seems to be always 0x02 couple of frames,.
  if (f2_startup_count < 14) {
    CYCLIC_DATA[59] = 0x02;
  } else {
    CYCLIC_DATA[59] = 0x00;
  }

#endif

  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.max_discharge_current_dA / 10, 26);

  // When SoC is 100%, drop down allowed charge current.
  if ((datalayer.battery.status.reported_soc / 100) < 100) {
    float2frame(CYCLIC_DATA, (float)datalayer.battery.status.max_charge_current_dA / 10, 34);
  } else {
    float2frame(CYCLIC_DATA, 0.0, 34);
  }

  if (nominal_voltage_dV > 0) {
    float2frame(CYCLIC_DATA, (float)(datalayer.battery.info.total_capacity_Wh / nominal_voltage_dV * 10),
                30);  // Battery capacity Ah
  }
  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.temperature_max_dC / 10, 38);
  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.temperature_min_dC / 10, 42);

  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.cell_max_voltage_mV / 1000, 46);
  float2frame(CYCLIC_DATA, (float)datalayer.battery.status.cell_min_voltage_mV / 1000, 50);

  CYCLIC_DATA[58] = (byte)(datalayer.battery.status.reported_soc / 100);

  register_content_ok = true;

  if (incoming_message_counter > 0) {
    incoming_message_counter--;
  }

  if (incoming_message_counter == 0) {
    set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  } else {
    clear_event(EVENT_MODBUS_INVERTER_MISSING);
  }
}

void receive_RS485()  // Runs as fast as possible to handle the serial stream
{
  currentMillis = millis();

  if (datalayer.system.status.battery_allows_contactor_closing & !contactorMillis) {
    contactorMillis = currentMillis;
  }
  if (currentMillis - contactorMillis >= INTERVAL_2_S & !RX_allow) {
    dbg_message("RX_allow -> true");
    RX_allow = true;
  }

  if (Serial2.available()) {
    RS485_RXFRAME[rx_index] = Serial2.read();
    if (RX_allow) {
      rx_index++;
      if (RS485_RXFRAME[rx_index - 1] == 0x00) {
        if ((rx_index > 9) && register_content_ok) {
          dbg_frame(RS485_RXFRAME, 10, "RX");
          if (check_kostal_frame_crc(rx_index)) {
            incoming_message_counter = RS485_HEALTHY;

            if (RS485_RXFRAME[1] == 'c') {
              if (RS485_RXFRAME[6] == 0x47) {
                // Set time function - Do nothing.
                send_kostal(ACK_FRAME, 8);  // ACK
              }
              if (RS485_RXFRAME[6] == 0x5E) {
                // Set State function
                if (RS485_RXFRAME[7] == 0x00) {
                  // Allow contactor closing
                  datalayer.system.status.inverter_allows_contactor_closing = true;
                  dbg_message("inverter_allows_contactor_closing -> true");
                  send_kostal(ACK_FRAME, 8);  // ACK
                } else if (RS485_RXFRAME[7] == 0x04) {
                  // INVALID STATE, no ACK sent
                } else {
                  // Battery deep sleep?
                  send_kostal(ACK_FRAME, 8);  // ACK
                }
              }
            } else if (RS485_RXFRAME[1] == 'b') {
              if (RS485_RXFRAME[6] == 0x50) {
                //Reverse polarity, do nothing
              } else {
                int code = RS485_RXFRAME[6] + RS485_RXFRAME[7] * 0x100;
                if (code == 0x44a) {
                  //Send cyclic data
                  update_values_battery();
                  update_RS485_registers_inverter();
                  if (f2_startup_count < 15) {
                    f2_startup_count++;
                  }
                  byte tmpframe[64];  //copy values to prevent data manipulation during rewrite/crc calculation
                  memcpy(tmpframe, CYCLIC_DATA, 64);
                  tmpframe[62] = calculate_kostal_crc(tmpframe, 62);
                  null_stuffer(tmpframe, 64);
                  send_kostal(tmpframe, 64);
                  CYCLIC_DATA[61] = 0x00;
                }
                if (code == 0x84a) {
                  //Send  battery info
                  byte tmpframe[40];  //copy values to prevent data manipulation during rewrite/crc calculation
                  memcpy(tmpframe, BATTERY_INFO, 40);
                  tmpframe[38] = calculate_kostal_crc(tmpframe, 38);
                  null_stuffer(tmpframe, 40);
                  send_kostal(tmpframe, 40);
                  if (!startupMillis) {
                    startupMillis = currentMillis;
                  }
                }
                if (code == 0x353) {
                  //Send  battery error/status
                  byte tmpframe[9];  //copy values to prevent data manipulation during rewrite/crc calculation
                  memcpy(tmpframe, STATUS_FRAME, 9);
                  tmpframe[7] = calculate_kostal_crc(tmpframe, 7);
                  null_stuffer(tmpframe, 9);
                  send_kostal(tmpframe, 9);
                }
              }
            }
          }
          rx_index = 0;
        }
        rx_index = 0;
      }
    }
    if (rx_index >= 299) {
      rx_index = 0;
    }
  }
}

void setup_inverter(void) {  // Performs one time setup at startup
  datalayer.system.status.inverter_allows_contactor_closing = false;
  dbg_message("inverter_allows_contactor_closing -> false");
  strncpy(datalayer.system.info.inverter_protocol, "BYD battery via Kostal RS485", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
#endif
