#include "../include.h"
#ifdef BYD_KOSTAL_RS485
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "KOSTAL-RS485.h"

static const uint8_t KOSTAL_FRAMEHEADER[5] = {0x62, 0xFF, 0x02, 0xFF, 0x29};
static const uint8_t KOSTAL_FRAMEHEADER2[5] = {0x63, 0xFF, 0x02, 0xFF, 0x29};
static uint16_t discharge_current_dA = 0;
static uint16_t charge_current_dA = 0;
static int16_t average_temperature_dC = 0;

union f32b {
  float f;
  byte b[4];
};

uint8_t frame1[40] = {
    0x06, 0xE2, 0xFF, 0x02, 0xFF, 0x29, 0x01, 0x08, 0x80, 0x43,  // 256.063 Max Charge?      first byte 0x01 or 0x04
    0xE4, 0x70, 0x8A, 0x5C, 0xB5, 0x02, 0xD3, 0x01, 0x01, 0x05, 0xC8, 0x41,  // 25.0024  TEMP??
    0xC2, 0x18, 0x01, 0x03, 0x59, 0x42, 0x01, 0x01, 0x01, 0x02, 0x05, 0x02, 0xA0, 0x01, 0x01, 0x02, 0x4D, 0x00};

// values in frame2 will be overwritten at update_modbus_registers_inverter()
uint8_t frame2[64] = {0x0A, 0xE2, 0xFF, 0x02, 0xFF, 0x29,  // frame Header
                      0x1D, 0x5A, 0x85, 0x43,              // Voltage     (float)           Modbus register 216, Bit 6-9

                      0x01, 0x03, 0x8D, 0x43,  // Max Voltage (2 byte float), Bit 10-13
                                               // 0x8D43 = 36163 (361.63) DALA: Is this nominal voltage?
                      0x01, 0x03, 0xAC, 0x41,  // Temp        (2 byte float)    Modbus register 214, Bit 14-17
                      0x01, 0x01, 0x01, 0x01,  // Current, Bit 18-21
                      0x01, 0x01, 0x01, 0x01,  // Current, Bit 22-25

                      0x01, 0x03, 0x48, 0x42,  // Peak discharge current (2 byte float), Bit 26-29

                      0x01, 0x03, 0xC8, 0x41,  // Nominal discharge I (2 byte float) , Bit 30-33

                      0x01, 0x01,  // Unknown
                      0x01, 0x05,  //  Max charge? (2 byte float) Bit 36-37

                      0xCD, 0xCC, 0xB4, 0x41,  // MaxCellTemp (4 byte float) Bit 38-41

                      0x01, 0x0C,  0xA4, 0x41,  // MinCellTemp (4 byte float) Bit 42-45

                      0xA4, 0x70, 0x55, 0x40,  // MaxCellVolt  (float), Bit 46-49

                      0x7D, 0x3F, 0x55, 0x40,  // MinCellVolt  (float), Bit 50-53

                      0xFE,        // Cylce count , Bit 54
                      0x04,        // Cycle count? , Bit 55
                      0x01, 0x40,  // ??  , Bit 56, 57
                      0x64,        // SOC , Bit 58
                      0x01,        // Unknown, Mostly 0x01, seen also 0x02
                      0x01,        // Unknown, Seen only 0x01
                      0x02,        // Unknown, Mostly 0x02. seen also 0x01
                      0x00,        // CRC (inverted sum of bytes 1-62 + 0xC0), Bit 62
                      0x00};

uint8_t frame3[9] = {
    0x08, 0xE2, 0xFF, 0x02, 0xFF, 0x29,  //header
    0x06,                                //Unknown
    0xEF,                                //CRC
    0x00                                 //endbyte
};

uint8_t frameB1[10] = {0x07, 0x63, 0xFF, 0x02, 0xFF, 0x29, 0x5E, 0x02, 0x16, 0x00};
uint8_t frameB1b[10] = {0x07, 0xE3, 0xFF, 0x02, 0xFF, 0x29, 0xF4, 0x00};

uint8_t RS485_RXFRAME[10];

bool register_content_ok = false;

void float2frame(byte* arr, float value, byte framepointer) {
  f32b g;
  g.f = value;
  arr[framepointer] = g.b[0];
  arr[framepointer + 1] = g.b[1];
  arr[framepointer + 2] = g.b[2];
  arr[framepointer + 3] = g.b[3];
}

void float2frameMSB(byte* arr, float value, byte framepointer) {
  f32b g;
  g.f = value;
  arr[framepointer + 0] = g.b[2];
  arr[framepointer + 1] = g.b[3];
}

void send_kostal(byte* arr, int alen) {
#ifdef DEBUG_KOSTAL_RS485_DATA
  Serial.print("TX: ");
  for (int i = 0; i < alen; i++) {
    if (arr[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(arr[i], HEX);
    Serial.print(" ");
  }
  Serial.println("\n");
#endif
  Serial2.write(arr, alen);
}

byte calculate_longframe_crc(byte* lfc, int lastbyte) {
  unsigned int sum = 0;
  for (int i = 0; i < lastbyte; ++i) {
    sum += lfc[i];
  }
  return ((byte) ~(sum + 0xc0) & 0xff);
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

static uint8_t rx_index = 0;

void receive_RS485()  // Runs as fast as possible to handle the serial stream
{
  if (Serial2.available()) {
    RS485_RXFRAME[rx_index] = Serial2.read();
    rx_index++;
    if (RS485_RXFRAME[rx_index - 1] == 0x00) {
      if ((rx_index == 10) && (RS485_RXFRAME[0] == 0x09) && register_content_ok) {
#ifdef DEBUG_KOSTAL_RS485_DATA
        Serial.print("RX: ");
        for (uint8_t i = 0; i < 10; i++) {
          Serial.print(RS485_RXFRAME[i], HEX);
          Serial.print(" ");
        }
        Serial.println("");
#endif
        rx_index = 0;
        if (check_kostal_frame_crc()) {
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
            Serial2.flush();
            delay(1);
            send_kostal(frameB1b, 10);
          }

          if (headerA && (RS485_RXFRAME[6] == 0x4A) && (RS485_RXFRAME[7] == 0x08)) {  // "frame 1"
            send_kostal(frame1, 40);
          }
          if (headerA && (RS485_RXFRAME[6] == 0x4A) && (RS485_RXFRAME[7] == 0x04)) {  // "frame 2"
            byte tmpframe[64];  //copy values to prevent data manipulation during rewrite/crc calculation
            memcpy(tmpframe, frame2, 64);
            for (int i = 1; i < 63; i++) {
              if (tmpframe[i] == 0x00) {
                tmpframe[i] = 0x01;
              }
            }
            tmpframe[62] = calculate_longframe_crc(tmpframe, 62);
            send_kostal(tmpframe, 64);
          }
          if (headerA && (RS485_RXFRAME[6] == 0x53) && (RS485_RXFRAME[7] == 0x03)) {  // "frame 3"
            send_kostal(frame3, 9);
          }
        }
      }
      rx_index = 0;
    }
    if (rx_index >= 10) {
      rx_index = 0;
    }
  }
}

void update_RS485_registers_inverter() {

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    charge_current_dA =
        ((datalayer.battery.status.max_charge_power_W * 10) /
         datalayer.battery.status.voltage_dV);  //Charge power in W , max volt in V+1decimal (P=UI, solve for I)
    //The above calculation results in (30 000*10)/3700=81A
    charge_current_dA = (charge_current_dA * 10);  //Value needs a decimal before getting sent to inverter (81.0A)

    discharge_current_dA =
        ((datalayer.battery.status.max_discharge_power_W * 10) /
         datalayer.battery.status.voltage_dV);  //Charge power in W , max volt in V+1decimal (P=UI, solve for I)
    //The above calculation results in (30 000*10)/3700=81A
    discharge_current_dA = (discharge_current_dA * 10);  //Value needs a decimal before getting sent to inverter (81.0A)
  }

  if (charge_current_dA > datalayer.battery.info.max_charge_amp_dA) {
    charge_current_dA =
        datalayer.battery.info
            .max_charge_amp_dA;  //Cap the value to the max allowed Amp. Some inverters cannot handle large values.
  }

  if (discharge_current_dA > datalayer.battery.info.max_discharge_amp_dA) {
    discharge_current_dA =
        datalayer.battery.info
            .max_discharge_amp_dA;  //Cap the value to the max allowed Amp. Some inverters cannot handle large values.
  }

  average_temperature_dC =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);
  if (datalayer.battery.status.temperature_min_dC < 0) {
    average_temperature_dC = 0;
  }

  float2frame(frame2, (float)(datalayer.battery.status.voltage_dV / 10), 6);  // Confirmed OK mapping

  float2frameMSB(frame2, (float)(datalayer.battery.info.max_design_voltage_dV / 10), 12);

  float2frameMSB(frame2, (float)(average_temperature_dC / 10), 16);

  float2frameMSB(frame2, (float)datalayer.battery.status.current_dA / 10, 20);  // Peak discharge? current (2 byte float)
  float2frameMSB(frame2, (float)datalayer.battery.status.current_dA / 10, 24);

  float2frameMSB(frame2, (float)(discharge_current_dA / 10), 28);  // Nominal discharge? I (2 byte float)
  float2frameMSB(frame2, (float)(discharge_current_dA / 10), 32);

  float2frame(frame2, (float)(datalayer.battery.status.temperature_max_dC / 10), 40);
  float2frame(frame2, (float)(datalayer.battery.status.temperature_min_dC / 10), 44);

  float2frame(frame2, (float)(datalayer.battery.status.cell_max_voltage_mV / 1000), 46);
  float2frame(frame2, (float)(datalayer.battery.status.cell_min_voltage_mV / 1000), 50);

  frame2[58] = (byte)(datalayer.battery.status.reported_soc / 100);  // Confirmed OK mapping

  register_content_ok = true;
}
#endif
