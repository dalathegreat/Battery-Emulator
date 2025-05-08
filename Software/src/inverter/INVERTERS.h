#ifndef INVERTERS_H
#define INVERTERS_H

// The abstract base class for all inverter protocols
class InverterProtocol {
 public:
  virtual void setup() = 0;
};

// The abstract base class for all Modbus inverter protocols
class ModbusInverterProtocol : public InverterProtocol {
 public:
  virtual void update_modbus_registers() = 0;
  virtual void handle_static_data();
};

class CanInverterProtocol : public InverterProtocol {
 public:
  virtual void send_intial_data() = 0;

  //This function maps all the values fetched from battery CAN to the correct CAN messages
  virtual void update_values() = 0;

  virtual void transmit_can(unsigned long currentMillis) = 0;

  virtual void map_can_frame_to_variable(CAN_frame rx_frame) = 0;
};

#include "../../USER_SETTINGS.h"

#ifdef AFORE_CAN
#include "AFORE-CAN.h"
#endif

#ifdef BYD_CAN_DEYE
#define BYD_CAN
#endif

#ifdef BYD_CAN
#include "BYD-CAN.h"
#endif

#ifdef BYD_MODBUS
#include "BYD-MODBUS.h"
#endif

#ifdef BYD_KOSTAL_RS485
#include "KOSTAL-RS485.h"
#endif

#ifdef FERROAMP_CAN
#include "FERROAMP-CAN.h"
#endif

#ifdef FOXESS_CAN
#include "FOXESS-CAN.h"
#endif

#ifdef GROWATT_HV_CAN
#include "GROWATT-HV-CAN.h"
#endif

#ifdef GROWATT_LV_CAN
#include "GROWATT-LV-CAN.h"
#endif

#ifdef PYLON_CAN
#include "PYLON-CAN.h"
#endif

#ifdef PYLON_LV_CAN
#include "PYLON-LV-CAN.h"
#endif

#ifdef SCHNEIDER_CAN
#include "SCHNEIDER-CAN.h"
#endif

#ifdef SMA_BYD_H_CAN
#include "SMA-BYD-H-CAN.h"
#endif

#ifdef SMA_BYD_HVS_CAN
#include "SMA-BYD-HVS-CAN.h"
#endif

#ifdef SMA_LV_CAN
#include "SMA-LV-CAN.h"
#endif

#ifdef SMA_TRIPOWER_CAN
#include "SMA-TRIPOWER-CAN.h"
#endif

#ifdef SOFAR_CAN
#include "SOFAR-CAN.h"
#endif

#ifdef SOLAX_CAN
#include "SOLAX-CAN.h"
#endif

#ifdef SUNGROW_CAN
#include "SUNGROW-CAN.h"
#endif

#ifdef CAN_INVERTER_SELECTED
void update_values_can_inverter();
void map_can_frame_to_variable_inverter(CAN_frame rx_frame);
void transmit_can_inverter(unsigned long currentMillis);
void setup_inverter();
#endif

#ifdef MODBUS_INVERTER_SELECTED
void update_modbus_registers_inverter();
void setup_inverter();
void handle_static_data_modbus();
#endif

#ifdef RS485_INVERTER_SELECTED
void receive_RS485();
void update_RS485_registers_inverter();
void setup_inverter();
#endif

#endif
