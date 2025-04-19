#ifndef BATTERY_H
#define BATTERY_H

#include <vector>
#include "../devboard/utils/types.h"

// Enum number values used in user settings. Retain numbering.
enum BatteryType {
  BMWi3 = 1,
  BMWIX = 2,
  BMWPhev = 3,
  BMWSBox = 4,
  BoltAmpera = 5,
  BydAtto3 = 6,
  CellPower = 7,
  Chademo = 8,
  CmfaEv = 33,
  Daly = 9,
  Ecmp = 32,
  Foxess = 10,
  ImievCzeroIon = 11,
  JaguarIpace = 12,
  KiaEGmp = 13,
  KiaHyundai64 = 14,
  KiaHyundaiHybrid = 15,
  Meb = 16,
  Mg5 = 17,
  NissanLeaf = 18,
  Pylon = 19,
  RangeRoverPhev = 20,
  RenaultKangoo = 21,
  RenaultTwizy = 22,
  RenaultZoeGen1 = 23,
  RenaultZoeGen2 = 24,
  SantaFePhev = 25,
  Sono = 26,
  Tesla3Y = 27,
  TeslaSX = 28,
  TestFake = 29,
  VolvoSpa = 30,
  VolvoSpaHybrid = 31,
  HighestBatteryType = 34
};

class BatteryBase {
 public:
  BatteryType type() { return m_type; }
  virtual const char* name() = 0;

  virtual void setup() = 0;
  virtual void update_values() = 0;

  virtual bool usesCAN() = 0;
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) = 0;
  /// @brief Called to ask the battery implementation to transmit any CAN messages it needs to send.
  virtual void transmit_can() = 0;

  virtual bool usesRS485() = 0;
  virtual int rs485_baudrate() { return 0; }
  virtual void transmit_RS485() = 0;
  virtual void receive_RS485() = 0;

  virtual bool requiresPrechargeControl() { return false; }

  static const char* name_for_type(BatteryType type);

 protected:
  BatteryBase(BatteryType type) : m_type(type) {}

  BatteryType m_type;
};

// Abstract base class for all batteries using CAN bus
class CanBattery : public BatteryBase {
 public:
  virtual bool usesCAN() { return true; }

  virtual bool usesRS485() { return false; }
  virtual int rs485_baudrate() { return 0; }
  virtual void transmit_RS485() {}
  virtual void receive_RS485() {}

 protected:
  CanBattery(BatteryType type) : BatteryBase(type) {}
};

// Abstract base class for all batteries using RS485 bus
class RS485Battery : public BatteryBase {
  virtual bool usesCAN() { return false; }
  virtual void handle_incoming_can_frame(CAN_frame rx_frame) {};
  virtual void transmit_can() {};

  virtual bool usesRS485() { return true; }

 protected:
  RS485Battery(BatteryType type) : BatteryBase(type) {}
};

std::vector<BatteryType> supported_battery_types();

BatteryBase* init_battery(BatteryType type);

extern BatteryBase* battery;
extern BatteryType userSelectedBatteryType;

#endif
