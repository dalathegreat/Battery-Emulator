#include "comm_rs485.h"
#include "../../include.h"

// Parameters

#ifdef MODBUS_INVERTER_SELECTED
#define MB_RTU_NUM_VALUES 13100
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);
#endif

// Initialization functions

void init_rs485() {
// Set up Modbus RTU Server
#ifdef RS485_EN_PIN
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);
#endif  // RS485_EN_PIN
#ifdef RS485_SE_PIN
  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);
#endif  // RS485_SE_PIN
#ifdef PIN_5V_EN
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);
#endif  // PIN_5V_EN
#if defined(RS485_INVERTER_SELECTED) || defined(RS485_BATTERY_SELECTED)
  Serial2.begin(RS485_BAUDRATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
#endif  // RS485_INVERTER_SELECTED || RS485_BATTERY_SELECTED
#ifdef MODBUS_INVERTER_SELECTED
#ifdef BYD_MODBUS
  // Init Static data to the RTU Modbus
  handle_static_data_modbus_byd();
#endif  // BYD_MODBUS
  // Init Serial2 connected to the RTU Modbus
  RTUutils::prepareHardwareSerial(Serial2);
  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  // Register served function code worker for server
  MBserver.registerWorker(MBTCP_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.registerWorker(MBTCP_ID, WRITE_HOLD_REGISTER, &FC06);
  MBserver.registerWorker(MBTCP_ID, WRITE_MULT_REGISTERS, &FC16);
  MBserver.registerWorker(MBTCP_ID, R_W_MULT_REGISTERS, &FC23);
  // Start ModbusRTU background task
  MBserver.begin(Serial2, MODBUS_CORE);
#endif  // MODBUS_INVERTER_SELECTED
}
