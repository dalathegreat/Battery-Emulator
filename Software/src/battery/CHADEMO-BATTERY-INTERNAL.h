#ifndef CHADEMO_BATTERY_TYPES_H
#define CHADEMO_BATTERY_TYPES_H

#define MAX_EVSE_POWER_CHARGING 3300
#define MAX_EVSE_OUTPUT_VOLTAGE 410
#define MAX_EVSE_OUTPUT_CURRENT 11

enum CHADEMO_STATE {
  CHADEMO_FAULT,
  CHADEMO_STOP,
  CHADEMO_IDLE,
  CHADEMO_CONNECTED,
  CHADEMO_INIT,  // intermediate state indicating CAN from Vehicle not yet received after connection
  CHADEMO_NEGOTIATE,
  CHADEMO_EV_ALLOWED,
  CHADEMO_EVSE_PREPARE,
  CHADEMO_EVSE_START,
  CHADEMO_EVSE_CONTACTORS_ENABLED,
  CHADEMO_POWERFLOW,
};

enum Mode { CHADEMO_CHARGE, CHADEMO_DISCHARGE, CHADEMO_BIDIRECTIONAL };

/* Charge/discharge sequence, indicating applicable V2H guideline 
 * If sequence number is not agreed upon via H201/H209 between EVSE and Vehicle,
 * V2H 1.1 is assumed, which..is somehow between 0x0 and 0x1 ? TODO: better understanding here
 * Use CHADEMO_seq to decide whether emitting 209 is necessary
 *	0x0	1.0 and earlier
 *	0x1	2.0 appendix A
 *	0x2	2.0 appendix B
 * TODO: is this influenced by x109->CHADEMO_protocol_number, or x102->ControlProtocolNumberEV ??
 * Unused for now.
uint8_t CHADEMO_seq = 0x0;
 */

/*----------- CHARGING SUPPORT V2X --------------------------------------------------------------*/
/* ---------- VEHICLE Data structures */

//H100 - Vehicle - Minimum charging expectations
//TODO decide whether default values for vehicle-origin frames is even appropriate
struct x100_Vehicle_Charging_Limits {
  uint8_t MinimumChargeCurrent = 0;
  uint16_t MinimumBatteryVoltage = 300;
  uint16_t MaximumBatteryVoltage = 402;
  uint8_t ConstantOfChargingRateIndication = 0;
};
//H101 - Vehicle - Maximum charging expectations
struct x101_Vehicle_Charging_Estimate {
  uint8_t MaxChargingTime10sBit = 0;
  uint8_t MaxChargingTime1minBit = 0;
  uint8_t EstimatedChargingTime = 0;
  uint16_t RatedBatteryCapacity = 0;
};

//H102 - Vehicle - Charging targets and Status
// peer to x109 from EVSE
// termination triggers in both
// TODO see also Table A.26—Charge control termination command patterns
struct x102_Vehicle_Charging_Session {  //Frame byte
  uint8_t ControlProtocolNumberEV = 0;  // 0
  uint16_t TargetBatteryVoltage = 0;    // 1-2
  uint8_t ChargingCurrentRequest = 0;   // 3	Note: per spec, units for this changed from kWh --> %

  union {
    uint8_t faults;
    struct {
      bool unused_3 : 1;
      bool unused_2 : 1;
      bool unused_1 : 1;
      bool FaultBatteryVoltageDeviation : 1;  // 4
      bool FaultHighBatteryTemperature : 1;   // 3
      bool FaultBatteryCurrentDeviation : 1;  // 2
      bool FaultBatteryUnderVoltage : 1;      // 1
      bool FaultBatteryOverVoltage : 1;       // 0
    } fault;
  } f;

  union {
    uint8_t packed;
    struct {
      bool StatusVehicleDischargeCompatible : 1;  //5.7
      bool unused_2 : 1;                          //5.6
      bool unused_1 : 1;                          //5.5
      bool StatusNormalStopRequest : 1;           //5.4
      bool StatusVehicle : 1;                     //5.3
      bool StatusChargingError : 1;               //5.2
      bool StatusVehicleShifterPosition : 1;      //5.1
      bool StatusVehicleChargingEnabled : 1;      //5.0 - bit zero is TODO. Vehicle charging enabled ==1  *AND* charge
                                                  // permission signal k needs to be active for charging to be
                                                  // permitted -- TODO document bits per byte for these flags
                                                  // and update variables to be more appropriate
    } status;
  } s;

  uint8_t StateOfCharge = 0;  //6 state of charge?
};

/* ---------- CHARGING: EVSE Data structures */
struct x108_EVSE_Capabilities {                                 // Frame byte
  bool contactor_weld_detection = 1;                            //  0
  uint16_t available_output_voltage = MAX_EVSE_OUTPUT_VOLTAGE;  //  1,2
  uint8_t available_output_current = MAX_EVSE_OUTPUT_CURRENT;   //  3
  uint16_t threshold_voltage = 297;                             //  4,5	 		voltage that EVSE will stop if car fails to
      //  			perhaps vehicle minus 3%, hardcoded initially to 96*2.95
      //  6,7 = unused
};

/* Does double duty for charging and discharging */
struct x109_EVSE_Status {                  // Frame byte
  uint8_t CHADEMO_protocol_number = 0x02;  //  0
  uint16_t setpoint_HV_VDC =
      0;  //  1,2  NOTE: charger_setpoint_HV_VDC elsewhere is a float. THIS is protocol-defined as an int. cast float->int and lose some precision for protocol adherence
  uint8_t setpoint_HV_IDC = 0;       //  3
                                     //
  bool discharge_compatible = true;  //  4, bit 0. bits
                                     //  4, bit 7-6 (?) unused. spec typo? maybe 1-7 unused
  union {
    uint8_t packed;
    struct {
      bool EVSE_status : 1;       //  5, bit 0
      bool EVSE_error : 1;        //  5, bit 1
      bool connector_locked : 1;  //  5, bit 2 //NOTE: treated as connector_lock during discharge, but
                                  //  seen as 'energizing' during charging mode

      bool battery_incompatible : 1;  //  5, bit 3
      bool ChgDischError : 1;         //  5, bit 4

      bool ChgDischStopControl : 1;  //  5, bit 5 - set to false for initialization to indicate 'preparing to charge'
                                     //  set to false when ready to charge/discharge

    } status;
  } s;

  // Either, or; not both.
  //  seconds field set to 0xFF by default
  //  indicating only the minutes field is used instead
  //  BOTH observed initially set to 0xFF in logs, so use
  //  	that as the initialzed value
  uint8_t remaining_time_10s = 0xFF;  //  6
  uint8_t remaining_time_1m = 0xFF;   //  7
};

/*----------- DISCHARGING SUPPORT V2X --------------------------------------------------------------*/
/* ---------- VEHICLE Data structures */
//H200 - Vehicle - Discharge limits
struct x200_Vehicle_Discharge_Limits {
  uint8_t MaximumDischargeCurrent = 0xFF;
  uint16_t MinimumDischargeVoltage = 0;
  uint16_t MinimumBatteryDischargeLevel = 0;
  uint16_t MaxRemainingCapacityForCharging = 0;
};

/* TODO When charge/discharge sequence control number (ID201/209) is not received, the vehicle or the EVSE
should determine that the other is the EVSE or the vehicle of the model before the V2H guideline 1.1. */
//H201 - Vehicle - Estimated capacity available
//	Intended primarily for display purposes.
//	Peer to H209
//	NOTE: in available CAN logs from a Leaf, 209 is sent with no 201 reply, so < 1.1 must be the inferred version
struct x201_Vehicle_Discharge_Estimate {
  uint8_t V2HchargeDischargeSequenceNum = 0;
  uint16_t ApproxDischargeCompletionTime = 0;
  uint16_t AvailableVehicleEnergy = 0;
};

/* ---------- EVSE Data structures */
struct x208_EVSE_Discharge_Capability {      // Frame byte
  uint8_t present_discharge_current = 0xFF;  //  0
  uint16_t available_input_voltage = 500;    //  1,2 -- poorly named as both 'available' and minimum input voltage
  uint16_t available_input_current = 250;    //  3   -- poorly named as both 'available' and maximum input current
                                             //  spec idiosyncracy in naming/description
                                             //  4,5 = unused
  uint16_t lower_threshold_voltage = 0;      //  6,7
};

// H209 - EVSE - Estimated Discharge Duration
// peer to Vehicle's 201 event (Note: 209 seen
// in CAN logs even when 201 is not)
struct x209_EVSE_Discharge_Estimate {          // Frame byte
  uint8_t sequence_control_number = 0x2;       //  0
  uint16_t remaining_discharge_time = 0x0000;  // 0x0000 == unused
};

/*----------- DYNAMIC CONTROL SUPPORT --------------------------------------------------------------*/
/* ---------- VEHICLE Data structures */
struct x110_Vehicle_Dynamic_Control {  //Frame byte
  union {
    uint8_t packed;
    struct {
      bool PermissionResetMaxChgTime : 1;  // bit 5 or 6? is this only x118 not x110?
      bool unused_3 : 1;
      bool unused_2 : 1;
      bool unused_1 : 1;
      bool HighVoltageControlStatus : 1;  //  	bit 2 = High voltage control support
      bool HighCurrentControlStatus : 1;  //  	bit 1 = High current control support
                                          //  		rate of change is -20A/s to 20A/s relative to 102.3
      bool DynamicControlStatus : 1;      // bit 0 = Dynamic Control support
    } status;
  } u;
};

/* ---------- EVSE Data structures */
// TODO 118
//H118
//see also table a.59 page 104 IEEE
struct x118_EVSE_Dynamic_Control {  // Frame byte
  union {
    uint8_t packed;
    struct {
      bool PermissionResetMaxChgTime : 1;  // bit 5 or 6?
      bool unused_3 : 1;
      bool unused_2 : 1;
      bool unused_1 : 1;
      bool HighVoltageControlStatus : 1;  //  	bit 2 = High voltage control support
      bool HighCurrentControlStatus : 1;  //  	bit 1 = High current control support
                                          //  		rate of change is -20A/s to 20A/s relative to 102.3
      bool DynamicControlStatus : 1;      // bit 0 = Dynamic Control support
    } status;
  } u;
};

/*----------- MANUFACTURER ID SUPPORT --------------------------------------------------------------*/
/* ---------- VEHICLE Data structures */
//H700 - Vehicle - Manufacturer identification
//Peer to H708
//Used to adapt to manufacturer-prescribed optional specification
struct x700_Vehicle_Vendor_ID {
  uint8_t AutomakerCode = 0;    // 0 = set to 0x0 to indicate incompatibility. Best as a starting place
  uint8_t OptionalContent = 0;  // 1-7, variable per vendor spec
};

void handle_chademo_sequence();

#endif
