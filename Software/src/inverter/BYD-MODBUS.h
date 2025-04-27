#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H
#include "../include.h"
#include "Inverter.h"

#define MODBUS_INVERTER_SELECTED

#define MB_RTU_NUM_VALUES 13100
#define MAX_POWER 40960  //BYD Modbus specific value

extern uint16_t mbPV[MB_RTU_NUM_VALUES];

void handle_static_data_modbus_byd();
void verify_temperature_modbus();
void verify_inverter_modbus();
void handle_update_data_modbusp201_byd();
void handle_update_data_modbusp301_byd();

class BydModbusInverter : public InverterProtocol {
 public:
  virtual void handle_static_data_modbus();

  virtual const char* name() { return Name; }
  static constexpr char* Name = "BYD 11kWh HVM battery over Modbus RTU";

 private:
  uint16_t si_data[2] = {21321, 1};
  uint16_t byd_data[16] = {16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t battery_data[16] = {16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752,
                               20594, 25965, 26997, 27936, 18518, 0,     0,     0};
  uint16_t volt_data[16] = {13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12854, 0, 0, 0, 0, 0, 0};
  uint16_t serial_data[16] = {20528, 13104, 21552, 12848, 23089, 14641, 12593, 14384, 12336, 12337, 0, 0, 0, 0, 0, 0};
  uint16_t static_data[2] = {1, 0};
  uint16_t* data_array_pointers[6] = {si_data, byd_data, battery_data, volt_data, serial_data, static_data};
  uint16_t data_sizes[6] = {sizeof(si_data),   sizeof(byd_data),    sizeof(battery_data),
                            sizeof(volt_data), sizeof(serial_data), sizeof(static_data)};
};

#endif
