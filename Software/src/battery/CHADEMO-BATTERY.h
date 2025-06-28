#ifndef CHADEMO_BATTERY_H
#define CHADEMO_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "CanBattery.h"

#ifdef CHADEMO_BATTERY
#define SELECTED_BATTERY_CLASS ChademoBattery

//Contactor control is required for CHADEMO support
#define CONTACTOR_CONTROL
#endif

class ChademoBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr char* Name = "Chademo V2X mode";

 private:
  void process_vehicle_charging_minimums(CAN_frame rx_frame);
  void process_vehicle_charging_maximums(CAN_frame rx_frame);
  void process_vehicle_charging_session(CAN_frame rx_frame);
  void process_vehicle_charging_limits(CAN_frame rx_frame);
  void process_vehicle_discharge_estimate(CAN_frame rx_frame);
  void process_vehicle_dynamic_control(CAN_frame rx_frame);
  void process_vehicle_vendor_ID(CAN_frame rx_frame);
  void evse_init();
  void update_evse_capabilities(CAN_frame& f);
  void update_evse_status(CAN_frame& f);
  void update_evse_discharge_estimate(CAN_frame& f);
  void update_evse_discharge_capabilities(CAN_frame& f);
  void handle_chademo_sequence();

  static const int MAX_EVSE_POWER_CHARGING = 3300;
  static const int MAX_EVSE_OUTPUT_VOLTAGE = 410;
  static const int MAX_EVSE_OUTPUT_CURRENT = 11;

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

    uint8_t StateOfCharge = 50;  //6 state of charge?
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
    uint16_t MinimumDischargeVoltage = 260;  //Initialized to a semi-sane value, updates via CAN later
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

  unsigned long setupMillis = 0;
  unsigned long handlerBeforeMillis = 0;
  unsigned long handlerAfterMillis = 0;

  /* Do not change code below unless you are sure what you are doing */
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
  unsigned long previousMillis5000 = 0;  // will store last time a 5s threshold was reached for display during debug

  bool plug_inserted = false;
  bool vehicle_can_initialized = false;
  bool vehicle_can_received = false;
  bool vehicle_permission = false;
  bool evse_permission = false;

  bool precharge_low = false;
  bool positive_high = false;
  bool contactors_ready = false;

  uint8_t framecount = 0;

  uint8_t max_discharge_current = 0;  //TODO not sure on this one, but really influenced by inverter capability

  bool high_current_control_enabled = false;  // set to true when high current control is operating
                                              //  if true, values from 110.1 and 110.2 should be used instead of 102.3
                                              //  and 118 should be used for evse responses
                                              //  permissible rate of change is -20A/s to 20A/s relative to 102.3

  Mode EVSE_mode = CHADEMO_DISCHARGE;
  CHADEMO_STATE CHADEMO_Status = CHADEMO_IDLE;

  /* Charge/discharge sequence, indicating applicable V2H guideline 
 * If sequence number is not agreed upon via H201/H209 between EVSE and Vehicle,
 * V2H 1.1 is assumed
 * Use CHADEMO_seq to decide whether emitting 209 is necessary
 *	0x0	1.0 and earlier
 *	0x1	2.0 appendix A
 *	0x2	2.0 appendix B
 * Unused for now.
uint8_t CHADEMO_seq = 0x0;
 */

  bool x201_received = false;
  bool x209_sent = false;

  struct x100_Vehicle_Charging_Limits x100_chg_lim = {};
  struct x101_Vehicle_Charging_Estimate x101_chg_est = {};
  struct x102_Vehicle_Charging_Session x102_chg_session = {};
  struct x110_Vehicle_Dynamic_Control x110_vehicle_dyn = {};
  struct x200_Vehicle_Discharge_Limits x200_discharge_limits = {};
  struct x201_Vehicle_Discharge_Estimate x201_discharge_estimate = {};
  struct x700_Vehicle_Vendor_ID x700_vendor_id = {};

  struct x209_EVSE_Discharge_Estimate x209_evse_dischg_est;
  struct x108_EVSE_Capabilities x108_evse_cap;
  struct x109_EVSE_Status x109_evse_state;
  struct x118_EVSE_Dynamic_Control x118_evse_dyn;
  struct x208_EVSE_Discharge_Capability x208_evse_dischg_cap;

  CAN_frame CHADEMO_108 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x108,
                           .data = {0x01, 0xF4, 0x01, 0x0F, 0xB3, 0x01, 0x00, 0x00}};
  CAN_frame CHADEMO_109 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x109,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x01, 0x20, 0xFF, 0xFF}};
  //For chademo v2.0 only
  CAN_frame CHADEMO_118 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x118,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // OLD value from skeleton implementation, indicates dynamic control is possible.
  // Hardcode above as being incompatible for simplicity in current incarnation.
  // .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};

  //    0x200 : From vehicle-side. A V2X-ready vehicle will send this message to broadcast its “Maximum discharger current”. (It is a similar logic to the limits set in 0x100 or 0x102 during a DC charging session)
  //    0x208 : From EVSE-side. A V2X EVSE will use this to send the “present discharger current” during the session, and the “available input current”. (uses similar logic to 0x108 and 0x109 during a DC charging session)
  CAN_frame CHADEMO_208 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x208,
                           .data = {0xFF, 0xF4, 0x01, 0xF0, 0x00, 0x00, 0xFA, 0x00}};
  CAN_frame CHADEMO_209 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x209,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
