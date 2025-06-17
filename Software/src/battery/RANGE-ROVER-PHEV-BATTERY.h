#ifndef RANGE_ROVER_PHEV_BATTERY_H
#define RANGE_ROVER_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef RANGE_ROVER_PHEV_BATTERY
#define SELECTED_BATTERY_CLASS RangeRoverPhevBattery
#endif

class RangeRoverPhevBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Range Rover 13kWh PHEV battery (L494/L405)";

 private:
  /* Change the following to suit your battery */
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //TODO: Configure
  static const int MIN_PACK_VOLTAGE_DV = 0;     //TODO: Configure
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_DEVIATION_MV = 150;
  ;
  unsigned long previousMillis50ms = 0;  // will store last time a 50ms CAN Message was sent

  //CAN content from battery
  bool StatusCAT5BPOChg = false;
  bool StatusCAT4Derate = false;
  uint8_t OCMonitorStatus = 0;
  bool StatusCAT3 = false;
  bool IsolationStatus = false;
  bool HVILStatus = false;
  bool ContactorStatus = false;
  uint8_t StatusGpCounter = 0;
  bool WeldCheckStatus = false;
  bool StatusCAT7NowBPO = false;
  bool StatusCAT6DlyBPO = false;
  uint8_t StatusGpCS = 0;
  uint8_t CAT6Count = 0;
  bool EndOfCharge = false;
  bool DerateWarning = false;
  bool PrechargeAllowed = false;
  uint8_t DischargeExtGpCounter = 0;    // Counter 0-15
  uint8_t DischargeExtGpCS = 0;         // CRC
  uint16_t DischargeVoltageLimit = 0;   //Min voltage battery allows discharging to
  uint16_t DischargePowerLimitExt = 0;  //Momentary Discharge power limit kW*0.01 (0-655)
  uint16_t DischargeContPwrLmt = 0;     //Longterm Discharge power limit kW*0.01 (0-655)
  uint8_t PwrGpCS = 0;                  // CRC
  uint8_t PwrGpCounter = 0;             // Counter 0-15
  uint16_t VoltageExt = 370;            // Voltage of the HV Battery
  uint16_t VoltageBus = 0;              // Voltage on the high-voltage DC bus
  int32_t CurrentExt =
      209715;  //Positive - discharge, Negative Charge (0 - 16777215) Scaling: 0.025 Offset: -209715.175 Units: Amps
  bool HVIsolationTestRunning = false;
  uint16_t VoltageOC =
      0;  //The instantaneous equivalent open-circuit voltage of the high voltage battery. This is used by the high-voltage inverter in power prediction and derating calculations.
  uint16_t DchCurrentLimit =
      0;  // A, 'Maximum current that can be delivered by the HV Battery during motoring mode  i.e during discharging.
  uint16_t ChgCurrentLimit =
      0;  // - 1023 A, Maximum current that can be transferred into the HV Battery during generating mode i.e during charging. Charging is neagtive and discharging is positive.
  uint16_t ChargeContPwrLmt = 0;      //Longterm charge power limit kW*0.01 (0-655)
  uint16_t ChargePowerLimitExt = 0;   //Momentary Charge power limit kW*0.01 (0-655)
  uint8_t ChgExtGpCS = 0;             // CRC
  uint8_t ChgExtGpCounter = 0;        //counter 0-15
  uint16_t ChargeVoltageLimit = 500;  //Max voltage limit during charging of the HV Battery.
  uint8_t CurrentWarning = 0;         // 0 normal, 1 cell overcurrent, 2 cell undercurrent
  uint8_t TempWarning = 0;            // 0 normal, 1 cell overtemp, 2 cell undertemp
  int8_t TempUpLimit = 0;             //Upper temperature limit.
  uint8_t CellVoltWarning = 0;        // 0 normal, 1 cell overvoltage, 2 cell undervoltage
  bool CCCVChargeMode = false;        //0 CC, 1 = CV
  uint16_t CellVoltUpLimit = 0;       //mV, Upper cell voltage limit
  uint16_t SOCHighestCell = 0;        //0.01, %
  uint16_t SOCLowestCell = 0;         //0.01, %
  uint16_t SOCAverage = 0;            //0.01, %
  bool WakeUpTopUpReq =
      false;  //The HV Battery can trigger a vehicle wake-up to request its State of Charge to be increased.
  bool WakeUpThermalReq =
      false;  //The HV Battery can trigger a vehicle wake-up in order to be thermally managed (ie. cooled down OR warmed up).
  bool WakeUpDchReq =
      false;  //The HV Battery can trigger a vehicle wake-up to request its State of Charge to be reduced.
  uint16_t StateofHealth = 0;
  uint16_t EstimatedLossChg =
      0;  //fact0.001, kWh Expected energy which will be lost during charging (at the rate given by VSCEstChargePower) due to resistance within the HV Battery.
  bool CoolingRequest =
      false;  //HV Battery cooling request to be cooled by the eAC/chiller as its cooling needs exceed the LTR cooling loop capability.
  uint16_t EstimatedLossDch =
      0;  //fact0.001, kWh Expected energy which will be lost during discharging (at the rate given by VSCEstDischargePower) due to resistance within the HV Battery.
  uint8_t FanDutyRequest =
      0;  //Request from the HV Battery cooling system to demand a change of duty for the electrical engine cooling fan speed (whilst using its LTR cooling loop).
  bool ValveCtrlStat = false;  //0 Chiller/Heater cooling loop requested , 1 LTR cooling loop requested
  uint16_t EstLossDchTgtSoC =
      0;  //fact0.001, kWh Expected energy which will be lost during discharging (at the rate given by VSCEstimatedDchPower) from the target charging SoC (PHEV: HVBattEnergyUsableMax, BEV: HVBattEnergyUsableBulk) down to HVBattEnergyUsableMin, due to resistance within the Traction Battery.
  uint8_t HeatPowerGenChg =
      0;  //fact0.1, kW, Estimated average heat generated by battery if charged at the rate given by VSCEstimatedChgPower.
  uint8_t HeatPowerGenDch =
      0;  //fact0.1, kW, Estimated average heat generated by battery if discharged at the rate given by VSCEstimatedDchPower.
  uint8_t WarmupRateChg =
      0;  //fact0.1, C/min , Expected average rate at which the battery will self-heat if charged at the rate given by VSCEstimatedChgPower.
  uint8_t WarmupRateDch =
      0;  //fact0.1, C/min , Expected average rate at which the battery will self-heat if discharged at the rate given by VSCEstimatedDchPower.
  uint16_t CellVoltageMax = 3700;
  uint16_t CellVoltageMin = 3700;
  int8_t CellTempAverage = 0;     //factor0.5, -40 offset
  int8_t CellTempColdest = 0;     //factor0.5, -40 offset
  int8_t CellTempHottest = 0;     //factor0.5, -40 offset
  uint8_t HeaterCtrlStat = 0;     //factor1, 0 offset
  bool ThermalOvercheck = false;  // 0 OK, 1 NOT OK
  int8_t InletCoolantTemp = 0;    //factor0.5, -40 offset
  bool ClntPumpDiagStat_UB = false;
  bool InletCoolantTemp_UB = false;
  bool CoolantLevel = false;      // Coolant level OK , 1 NOT OK
  bool ClntPumpDiagStat = false;  // 0 Pump OK, 1 NOT OK
  uint8_t MILRequest = 0;         //No req, 1 ON, 2 FLASHING, 3 unused
  uint16_t EnergyAvailable = 0;   //fac0.05 , The total energy available from the HV Battery
  uint16_t EnergyUsableMax = 0;   //fac0.05 , The total energy available from the HV Battery at its maximum SOC
  uint16_t EnergyUsableMin = 0;   //fac0.05 , The total energy available from the HV Battery at its minimum SOC
  uint16_t TotalCapacity =
      0;  //fac0.1 , Total Battery capacity in Kwh. This will reduce over the lifetime of the HV Battery.

  //CAN messages needed by battery (LOG needed!)
  CAN_frame RANGE_ROVER_18B = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x18B,  //CONTENT??? TODO
                               .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
