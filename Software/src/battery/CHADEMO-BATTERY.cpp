#include "../include.h"
#ifdef CHADEMO_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "CHADEMO-BATTERY-TYPES.h"
#include "CHADEMO-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static uint8_t errorCode = 0;                //stores if we have an error code active from battery control logic

bool plug_inserted = false;
bool vehicle_can_received = false;
bool vehicle_permission = false;
bool evse_permission = false;

bool precharge_low = false;
bool positive_high = false;
bool contactors_ready = false;
uint8_t maximum_soc = 90;
uint8_t minimum_soc = 10;

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

CAN_frame_t CHADEMO_108 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x108,
                           .data = {0x01, 0xF4, 0x01, 0x0F, 0xB3, 0x01, 0x00, 0x00}};
CAN_frame_t CHADEMO_109 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x109,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x01, 0x20, 0xFF, 0xFF}};
//For chademo v2.0 only
CAN_frame_t CHADEMO_118 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x118,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
// OLD value from skeleton implementation, indicates dynamic control is possible.
// Hardcode above as being incompatible for simplicity in current incarnation.
// .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};

//    0x200 : From vehicle-side. A V2X-ready vehicle will send this message to broadcast its “Maximum discharger current”. (It is a similar logic to the limits set in 0x100 or 0x102 during a DC charging session)
//    0x208 : From EVSE-side. A V2X EVSE will use this to send the “present discharger current” during the session, and the “available input current”. (uses similar logic to 0x108 and 0x109 during a DC charging session)
CAN_frame_t CHADEMO_208 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x208,
                           .data = {0xFF, 0xF4, 0x01, 0xF0, 0x00, 0x00, 0xFA, 0x00}};

CAN_frame_t CHADEMO_209 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x209,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

//This function maps all the values fetched via CAN to the correct parameters used for the inverter
void update_values_battery() {

  datalayer.battery.status.real_soc = x102_chg_session.StateOfCharge;

  datalayer.battery.status.max_discharge_power_W =
      (x200_discharge_limits.MaximumDischargeCurrent * x100_chg_lim.MaximumBatteryVoltage);  //In Watts, Convert A to P

  datalayer.battery.status.voltage_dV = x102_chg_session.TargetBatteryVoltage;  //TODO: scaling?

  datalayer.battery.info.total_capacity_Wh =
      ((x101_chg_est.RatedBatteryCapacity / 0.11) *
       1000);  //(Added in CHAdeMO v1.0.1), maybe handle hardcoded on lower protocol version?

  /* TODO max charging rate = 
   * 	x200_discharge_limits.MaxRemainingCapacityForCharging /
   * 	    x101_chg_est.RatedBatteryCapacity * 100;
   */

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  /* To simulate or NOT to simulate battery cell voltages, that is .. A question.
   * Answer for now: Not, because they are not available in any direct manner.
   * This will impact Solax inverter support, which uses cell min/max mV to populate
   * CAN frames.
   */

#ifdef DEBUG_VIA_USB
  Serial.print("SOC 0x100: ");
  Serial.println(x100_chg_lim.ConstantOfChargingRateIndication);
#endif
}

//TODO see Table A.26—Charge control termination command pattern on pg58
//for stop conditions
void evse_stop() {
  /*
	/// Sets 109.5.5 high
	x109_evse_state.status_charger_stop_control = true;
        x109_evse_state.output_voltage = 0.0;
        x109_evse_state.output_current = 0;
        x109_evse_state.remaining_charging_time_10s_bit = 0;
        x109_evse_state.remaining_charging_time_1min_bit = 0;
        x109_evse_state.status.fault_battery_incompatibility = false;
        x109_evse_state.status.fault_charging_system_malfunction = false;
        x109_evse_state.status.fault_station_malfunction = false;
	*/
}
void evse_charge_start() {
  /*
        x109_evse_state.status_charger_stop_control = false;
        x109_evse_state.status_station = true;
	x109_evse_state.remaining_charging_time_10s_bit = 255;
        x109_evse_state.remaining_charging_time_1min_bit = 60;
	*/
}

inline void process_vehicle_charging_minimums(CAN_frame_t rx_frame) {
  x100_chg_lim.MinimumChargeCurrent = rx_frame.data.u8[0];
  x100_chg_lim.MinimumBatteryVoltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
  x100_chg_lim.MaximumBatteryVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
  x100_chg_lim.ConstantOfChargingRateIndication = rx_frame.data.u8[6];
}

inline void process_vehicle_charging_maximums(CAN_frame_t rx_frame) {
  x101_chg_est.MaxChargingTime10sBit = rx_frame.data.u8[1];
  x101_chg_est.MaxChargingTime1minBit = rx_frame.data.u8[2];
  x101_chg_est.EstimatedChargingTime = rx_frame.data.u8[3];
  x101_chg_est.RatedBatteryCapacity = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
}

inline void process_vehicle_charging_session(CAN_frame_t rx_frame) {
  uint16_t newTargetBatteryVoltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
  uint16_t priorChargingCurrentRequest = x102_chg_session.ChargingCurrentRequest;
  uint8_t priorTargetBatteryVoltage = x102_chg_session.TargetBatteryVoltage;
  uint8_t newChargingCurrentRequest = rx_frame.data.u8[3];

  vehicle_permission = digitalRead(CHADEMO_PIN_4);

  x102_chg_session.ControlProtocolNumberEV = rx_frame.data.u8[0];

  x102_chg_session.f.fault.FaultBatteryOverVoltage = bitRead(rx_frame.data.u8[4], 0);
  x102_chg_session.f.fault.FaultBatteryUnderVoltage = bitRead(rx_frame.data.u8[4], 1);
  x102_chg_session.f.fault.FaultBatteryCurrentDeviation = bitRead(rx_frame.data.u8[4], 2);
  x102_chg_session.f.fault.FaultHighBatteryTemperature = bitRead(rx_frame.data.u8[4], 3);
  x102_chg_session.f.fault.FaultBatteryVoltageDeviation = bitRead(rx_frame.data.u8[4], 4);

  x102_chg_session.s.status.StatusVehicleChargingEnabled = bitRead(rx_frame.data.u8[5], 0);
  x102_chg_session.s.status.StatusVehicleShifterPosition = bitRead(rx_frame.data.u8[5], 1);
  x102_chg_session.s.status.StatusChargingError = bitRead(rx_frame.data.u8[5], 2);
  x102_chg_session.s.status.StatusVehicle = bitRead(rx_frame.data.u8[5], 3);
  x102_chg_session.s.status.StatusNormalStopRequest = bitRead(rx_frame.data.u8[5], 4);

  x102_chg_session.StateOfCharge = rx_frame.data.u8[6];

  x102_chg_session.ChargingCurrentRequest = newChargingCurrentRequest;
  x102_chg_session.TargetBatteryVoltage = newTargetBatteryVoltage;

  //Table A.26—Charge control termination command patterns -- should echo x108 handling

  /*  charge/discharge permission signal from vehicle on pin 4 should NOT be sensed before first CAN received from vehicle.
   *  Also, vehicle CAN should not simultaneously indicate enabled while permissions signal is absent
   *
   *  Either is a logical inconsistency per spec (vehicle has lost state, some wire/pin is broken, etc)
   *  	this should trigger stop and teardown
   */
  if ((CHADEMO_Status == CHADEMO_INIT && vehicle_permission) ||
      (x102_chg_session.s.status.StatusVehicleChargingEnabled && !vehicle_permission)) {
#ifdef DEBUG_VIA_USB
    Serial.println("Inconsistent charge/discharge state.");
#endif
    CHADEMO_Status = CHADEMO_FAULT;
    return;
  }

  if (!vehicle_permission || !x102_chg_session.s.status.StatusVehicleChargingEnabled) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle indicates dis/charging should cease");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryOverVoltage) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle indicates fault, battery over voltage.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryUnderVoltage) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle indicates fault, battery under voltage.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryCurrentDeviation) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle indicates fault, battery current deviation. Possible EVSE issue?");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryVoltageDeviation) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle indicates fault, battery voltage deviation. Possible EVSE issue?");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  // end
  if (priorTargetBatteryVoltage > 0 && newTargetBatteryVoltage == 0) {
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.StateOfCharge < minimum_soc) {
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  //FIXME condition nesting or more stanzas needed here for clear determination of cessation reason
  if (CHADEMO_Status == CHADEMO_POWERFLOW && EVSE_mode == CHADEMO_CHARGE &&
      (x102_chg_session.StateOfCharge >= maximum_soc || !vehicle_permission)) {
#ifdef DEBUG_VIA_USB
    Serial.println("State of charge ceiling reached, stop charging");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (vehicle_permission && CHADEMO_Status == CHADEMO_NEGOTIATE) {
    CHADEMO_Status = CHADEMO_EV_ALLOWED;
#ifdef DEBUG_VIA_USB
    Serial.println("STATE shift to CHADEMO_EV_ALLOWED in process_vehicle_charging_session()");
#endif
    return;
  }

  // TODO this and the next stanza influence state/control
  //  and probably don't belong in this function
  // consider relocating
  if (vehicle_permission && CHADEMO_Status == CHADEMO_EVSE_PREPARE && priorTargetBatteryVoltage == 0 &&
      newTargetBatteryVoltage > 0 && x102_chg_session.s.status.StatusVehicleChargingEnabled) {
#ifdef DEBUG_VIA_USB
    Serial.println("STATE SHIFT to EVSE_START reached in process_vehicle_charging_session()");
#endif
    CHADEMO_Status = CHADEMO_EVSE_START;
    return;
  }

  if (vehicle_permission && evse_permission && CHADEMO_Status == CHADEMO_POWERFLOW) {
#ifdef DEBUG_VIA_USB
    Serial.println("updating vehicle request in process_vehicle_charging_session()");
#endif
    return;
  }

#ifdef DEBUG_VIA_USB
  Serial.println("UNHANDLED STATE IN process_vehicle_charging_session()");
#endif
  return;
}

/* x200 Vehicle, peer to x208 EVSE */
inline void process_vehicle_charging_limits(CAN_frame_t rx_frame) {
  x200_discharge_limits.MaximumDischargeCurrent = rx_frame.data.u8[0];
  x200_discharge_limits.MinimumDischargeVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
  x200_discharge_limits.MinimumBatteryDischargeLevel = rx_frame.data.u8[6];
  x200_discharge_limits.MaxRemainingCapacityForCharging = rx_frame.data.u8[7];
}

/* Vehicle 0x201, peer to EVSE 0x209 
 * HOWEVER, 201 isn't even emitted in any of the v2x canlogs available
 */
inline void process_vehicle_discharge_estimate(CAN_frame_t rx_frame) {
  x201_discharge_estimate.V2HchargeDischargeSequenceNum = rx_frame.data.u8[0];
  x201_discharge_estimate.ApproxDischargeCompletionTime = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
  x201_discharge_estimate.AvailableVehicleEnergy = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
}

inline void process_vehicle_dynamic_control(CAN_frame_t rx_frame) {
  //SM Dynamic Control = Charging station can increase of decrease "available output current" during charging.
  //If you set 0x110 byte 0, bit 0 to 1 you say you can do dynamic control.
  //Charging station communicates this in 0x118 byte 0, bit 0
  x110_vehicle_dyn.u.status.HighVoltageControlStatus = bitRead(rx_frame.data.u8[0], 2);
  x110_vehicle_dyn.u.status.HighCurrentControlStatus = bitRead(rx_frame.data.u8[0], 1);
  x110_vehicle_dyn.u.status.DynamicControlStatus = bitRead(rx_frame.data.u8[0], 0);
}

inline void process_vehicle_vendor_ID(CAN_frame_t rx_frame) {
  x700_vendor_id.AutomakerCode = rx_frame.data.u8[0];
  x700_vendor_id.OptionalContent =
      ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);  //Actually more bytes, but not needed for our purpose
}

//#define CH_CAN_DEBUG
void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive =
      CAN_STILL_ALIVE;  //We are getting CAN messages from the vehicle, inform the watchdog

#ifdef CH_CAN_DEBUG
  Serial.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  Serial.print("  ");
  Serial.print(rx_frame.MsgID, HEX);
  Serial.print("  ");
  Serial.print(rx_frame.FIR.B.DLC);
  Serial.print("  ");
  for (int i = 0; i < rx_frame.FIR.B.DLC; ++i) {
    Serial.print(rx_frame.data.u8[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
#endif

  /*  e.g., CHADEMO_INIT state is a transient, used to indicate when CAN
   *  has not yet been receied from a vehicle 
   */
  if (CHADEMO_Status == CHADEMO_INIT) {
    // First CAN messages received, entering into negotiation
    CHADEMO_Status = CHADEMO_NEGOTIATE;
    // TODO consider tracking delta since transition time for expiry
  }

  // used for testing vehicle sanity
  vehicle_can_received = true;

  switch (rx_frame.MsgID) {
    case 0x100:
      process_vehicle_charging_minimums(rx_frame);
      break;
    case 0x101:
      process_vehicle_charging_maximums(rx_frame);
      break;
    case 0x102:
      process_vehicle_charging_session(rx_frame);
      break;
    case 0x200:  //For V2X
      process_vehicle_charging_limits(rx_frame);
      break;
    case 0x201:  //For V2X
      x201_received = true;
      process_vehicle_discharge_estimate(rx_frame);
      break;
    case 0x110:  //Only present on Chademo v2.0
      process_vehicle_dynamic_control(rx_frame);
    case 0x700:
      process_vehicle_vendor_ID(rx_frame);
      break;
    default:
      break;
  }

  handle_chademo_sequence();
}

/* (re)initialize evse structures to pre-charge/discharge states */
void evse_init() {
  // Held at 1 until start of charge when set to 0
  // returns to 1 when ceasing power flow
  // mutually exclusive values
  x109_evse_state.s.status.ChgDischStopControl = 1;
  x109_evse_state.s.status.EVSE_status = 0;

  x109_evse_state.s.status.connector_locked = 0;
  x109_evse_state.s.status.battery_incompatible = 0;
  x109_evse_state.s.status.ChgDischError = 0;

  /* values set during object initialization
	x109_evse_state.CHADEMO_protocol_number
	x109_evse_state.remaining_time_10s
	x109_evse_state.remaining_time_1m
	*/
}

/* updates for x108 */
void update_evse_capabilities(CAN_frame_t& f) {

  /* TODO use charger defines/runtime config?
   * for now..leave as a future tweak.
   * use vehicle requests as a ceiling
   */

  /* interpret this as mostly a timing concern, so indicate yes we support EV contactor weld detection
	 * expectations: <1s after vehicle contactors open, charger output will drop to <=25% of prior voltage
	 * 		 <2s after vehicle contactors open, charter output voltage will drop to <=10V
      	 *
	 * see A.10.2.1 Example of welding detection logic on the vehicle
	 */
  x108_evse_cap.contactor_weld_detection = 0x1;

  x108_evse_cap.available_output_voltage = x102_chg_session.TargetBatteryVoltage;

  /* calculate max threshold to protect battery - using vehicle-provided max minus 2% */
  x108_evse_cap.threshold_voltage =
      x100_chg_lim.MaximumBatteryVoltage - (int)(x100_chg_lim.MaximumBatteryVoltage / 100 * 2);

  // Power and voltage may be best derived from config/defines not from the x108 settings elsewhere, ideally
  // only set current when voltage > 0, as it is set by x102 TargetBatteryVoltage
  if (x108_evse_cap.available_output_voltage) {
    x108_evse_cap.available_output_current = MAX_EVSE_POWER_CHARGING / x108_evse_cap.available_output_voltage;
  }

  /* update Frame - byte 6 and 7 are unused */
  CHADEMO_108.data.u8[0] = x108_evse_cap.contactor_weld_detection;
  CHADEMO_108.data.u8[1] = lowByte(x108_evse_cap.available_output_voltage);
  CHADEMO_108.data.u8[2] = highByte(x108_evse_cap.available_output_voltage);
  CHADEMO_108.data.u8[3] = x108_evse_cap.available_output_current;
  CHADEMO_108.data.u8[4] = lowByte(x108_evse_cap.threshold_voltage);
  CHADEMO_108.data.u8[5] = highByte(x108_evse_cap.threshold_voltage);
}

/* updates for x109 */
void update_evse_status(CAN_frame_t& f) {

  x109_evse_state.s.status.EVSE_status = 1;
  x109_evse_state.s.status.EVSE_error = 0;
  x109_evse_state.s.status.ChgDischError = 0;
  x109_evse_state.s.status.ChgDischStopControl = 0;

  /* updated only in state handler 
	 * TODO..why?
	x109_evse_state.s.connector_locked = 0;
	*/

  if (EVSE_mode == CHADEMO_DISCHARGE) {
    /* Occasionally oberved as set to 0 when discharging
     * this may be true for all V2H versions
     * unless it was a logging discrepancy
    x109_evse_state.setpoint_HV_VDC = 0;
    x109_evse_state.setpoint_HV_IDC = 0;
     */
    x109_evse_state.setpoint_HV_VDC =
        min(x102_chg_session.TargetBatteryVoltage, x108_evse_cap.available_output_voltage);
    x109_evse_state.setpoint_HV_IDC =
        min(x102_chg_session.ChargingCurrentRequest, x108_evse_cap.available_output_current);

    /* TODO calculate remaining discharge time : for now == 60m */
    x109_evse_state.remaining_time_1m = 60;

  } else if (EVSE_mode == CHADEMO_CHARGE) {
    //FIXME these are supposed to be measured values, e.g., from a shunt
    //for now we are literally saying they're equivalent to the request or max charger capability
    //this is wrong
    x109_evse_state.setpoint_HV_VDC =
        min(x102_chg_session.TargetBatteryVoltage, x108_evse_cap.available_output_voltage);
    x109_evse_state.setpoint_HV_IDC =
        min(x102_chg_session.ChargingCurrentRequest, x108_evse_cap.available_output_current);
    /* The spec suggests throwing a 109.5.4 = 1 if vehicle curr request 102.3 > evse curr available 108.3, 
     *  but realistically many chargers seem to act tolerant here and stay under limits and supply whatever they are able
     */

    /* if power overcommitted, back down to just below while maintaining voltage target */
    if (x109_evse_state.setpoint_HV_IDC * x109_evse_state.setpoint_HV_VDC > MAX_EVSE_POWER_CHARGING) {
      x109_evse_state.setpoint_HV_IDC = floor(MAX_EVSE_POWER_CHARGING / x109_evse_state.setpoint_HV_VDC);
    }

    /* TODO calculate remaining charge time : for now == 60m */
    x109_evse_state.remaining_time_1m = 60;
  }

  /* Table A.26 - See also Charge control termination command patterns 
	 * This handling must be mirrored in handling for vehicle x102
	 *
	 */
  if ((x102_chg_session.TargetBatteryVoltage > x108_evse_cap.available_output_voltage) ||
      (x100_chg_lim.MaximumBatteryVoltage > x108_evse_cap.threshold_voltage)) {

    ///Battery incompatibility” flag #109.5.3 to 1
    x109_evse_state.s.status.EVSE_error = 1;
    x109_evse_state.s.status.battery_incompatible = 1;
    x109_evse_state.s.status.ChgDischStopControl = 1;
    CHADEMO_Status = CHADEMO_FAULT;
  }

  //CHADEMO_109.data.u8[0] hardcoded to 0x2 for CHAdeMO v1, 1.0.1, 1.1, 1.2
  // in initialization
  CHADEMO_109.data.u8[0] = x109_evse_state.CHADEMO_protocol_number;
  CHADEMO_109.data.u8[1] = lowByte(x109_evse_state.setpoint_HV_VDC);
  CHADEMO_109.data.u8[2] = highByte(x109_evse_state.setpoint_HV_VDC);
  CHADEMO_109.data.u8[3] = x109_evse_state.setpoint_HV_IDC;
  CHADEMO_109.data.u8[4] = x109_evse_state.discharge_compatible;

  /* clear statuses/faults, then set explicitly */
  CHADEMO_109.data.u8[5] = 0;
  CHADEMO_109.data.u8[5] =
      x109_evse_state.s.status.EVSE_status | x109_evse_state.s.status.EVSE_error << 1 |
      x109_evse_state.s.status.connector_locked << 2 | x109_evse_state.s.status.battery_incompatible << 3 |
      x109_evse_state.s.status.ChgDischError << 4 | x109_evse_state.s.status.ChgDischStopControl << 5;

  CHADEMO_109.data.u8[6] = x109_evse_state.remaining_time_10s;
  CHADEMO_109.data.u8[7] = x109_evse_state.remaining_time_1m;
}

/* Discharge estimates: x209 EVSE = peer to x201 Vehicle
 * NOTE: x209 is emitted in CAN logs when x201 isn't even present
 * 	it may not be understood by leaf (or ignored unless >= a certain protocol version or v2h sequence number
 */
void update_evse_discharge_estimate(CAN_frame_t& f) {

  //x209_evse_dischg_est.remaining_discharge_time_1m = x201_discharge_estimate.ApproxDischargeCompletionTime;

  //x201_discharge_estimate.AvailableVehicleEnergy = 0;

  //do nothing to alter the default initialized values..this may be unneeded
  /* TODO
	if (x200_discharge_limits.MaxRemainingCapacityForCharging = max charge voltage){
	if (x200_discharge_limits.MinimumBatteryDischargeLevel = kwH for v2h<1.0){
	if (x200_discharge_limits.MaxRemainingCapacityForCharging = kwH for v2h<1.0){
	*/

  CHADEMO_209.data.u8[0] = x209_evse_dischg_est.sequence_control_number;
  CHADEMO_209.data.u8[1] = x209_evse_dischg_est.remaining_discharge_time;
}

/* x208 EVSE, peer to 0x200 Vehicle */
void update_evse_discharge_capabilities(CAN_frame_t& f) {
  //FIXME these are supposed to be measured values, e.g., from a shunt
  //we are literally saying theyre arbitrary for now
  //this is wrong
  x208_evse_dischg_cap.present_discharge_current = 0xFF - 6;
  x208_evse_dischg_cap.available_input_current = 0xFF - x200_discharge_limits.MaximumDischargeCurrent;

  x208_evse_dischg_cap.available_input_voltage = x200_discharge_limits.MinimumDischargeVoltage;

  /* calculate min threshold to protect battery - using vehicle-provided minimum plus 2% */
  x208_evse_dischg_cap.lower_threshold_voltage =
      x200_discharge_limits.MinimumDischargeVoltage + (int)(x200_discharge_limits.MinimumDischargeVoltage / 100 * 2);

  /* 0x00 == unused
	if (x200_discharge_limits.MinimumBatteryDischargeLevel > 0 && 
	x208_evse_dischg_cap.minimum_input_voltage < x200_discharge_limits.MinimumBatteryDischargeLevel){
		// stop discharging, but permit charging if mode = bidirectional
	}
   */

  //TODO might be ideal to do the 0xFF subtraction HERE during serialization, rather than above?
  CHADEMO_208.data.u8[0] = x208_evse_dischg_cap.present_discharge_current;
  CHADEMO_208.data.u8[1] = lowByte(x208_evse_dischg_cap.available_input_voltage);
  CHADEMO_208.data.u8[2] = highByte(x208_evse_dischg_cap.available_input_voltage);

  CHADEMO_208.data.u8[3] = x208_evse_dischg_cap.available_input_current;

  CHADEMO_208.data.u8[6] = lowByte(x208_evse_dischg_cap.lower_threshold_voltage);
  CHADEMO_208.data.u8[7] = highByte(x208_evse_dischg_cap.lower_threshold_voltage);
}

void send_can_battery() {

  unsigned long currentMillis = millis();

  handle_chademo_sequence();

  /* no EVSE messages should be sent until the vehicle has
   * initiated
   */
  if (CHADEMO_Status <= CHADEMO_INIT || !vehicle_can_received) {
    return;
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis100 >= INTERVAL_100_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis100));
    }
    previousMillis100 = currentMillis;

    update_evse_capabilities(CHADEMO_108);
    update_evse_status(CHADEMO_109);
    update_evse_discharge_capabilities(CHADEMO_208);
    update_evse_discharge_estimate(CHADEMO_209);

    /* most updates to these EVSE frames are made
     * upon receipt of a Vehicle message, as 
     * that is the limiting factor. Therefore, we
     * can generally send as is without tweaks here.
     */
    ESP32Can.CANWriteFrame(&CHADEMO_108);
    ESP32Can.CANWriteFrame(&CHADEMO_109);

    /* TODO for dynamic control: can send x118 with byte 6 bit 0 set to 0 for 1s (before flipping back to 1) as a way of giving vehicle a chance to update 101.1 and 101.2
     * 	within 6 seconds of x118 toggle.
     * 	Then 109.6 and 109.7 reset remaining charging time
     * 	see A.11.5.3.1.3 Remaining charging time (H’109.6, H’109.7) for a better description
     */

    if (EVSE_mode == CHADEMO_DISCHARGE || EVSE_mode == CHADEMO_BIDIRECTIONAL) {
      ESP32Can.CANWriteFrame(&CHADEMO_208);
      if (x201_received) {
        ESP32Can.CANWriteFrame(&CHADEMO_209);
        x209_sent = true;
      }
    }

    // TODO need an update_evse_dynamic_control(..) function above before we send 118
    // 	110.0.0
    if (x102_chg_session.ControlProtocolNumberEV >= 0x03) {  //Only send the following on Chademo 2.0 vehicles?
#ifdef DEBUG_VIA_USB
      //FIXME REMOVE
      Serial.println("REMOVE: proto 2.0");
#endif
      ESP32Can.CANWriteFrame(&CHADEMO_118);
    }
  }
}

/* A lot of the heavy lifting happens here. This is essentially the state hander. SOME
 *  state transitions happen in functions before/after this is called.
 *
 * stages according to IEEE SPEC, with our states mapped into each.
 *    1) Standby stage
 *      CHADEMO_IDLE
 *    2) Preparation stage
 *      CHADEMO_CONNECTED
 *      CHADEMO_INIT
 *      CHADEMO_NEGOTIATE
 *      CHADEMO_EV_ALLOWED
 *      CHADEMO_EVSE_PREPARE
 *       CHADEMO_EVSE_START
 *      CHADEMO_EVSE_CONTACTORS_ENABLED
 *    3) Charging/Discharging stage
 *      CHADEMO_POWERFLOW
 *    4) Termination stage
 *      CHADEMO_STOP
 *    5) Emergency stop stage
 *      CHADEMO_FAULT
 */
void handle_chademo_sequence() {

  precharge_low = digitalRead(PRECHARGE_PIN) == LOW;
  positive_high = digitalRead(POSITIVE_CONTACTOR_PIN) == HIGH;
  contactors_ready = precharge_low && positive_high;
  vehicle_permission = digitalRead(CHADEMO_PIN_4);

  /* -------------------    State override conditions checks	------------------- */
  /* ------------------------------------------------------------------------------ */
  if (CHADEMO_Status >= CHADEMO_EV_ALLOWED && !x102_chg_session.s.status.StatusVehicleShifterPosition) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle is not parked, abort.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
  }

  if (CHADEMO_Status >= CHADEMO_EV_ALLOWED && !vehicle_permission) {
#ifdef DEBUG_VIA_USB
    Serial.println("Vehicle charge/discharge permission ended, stop.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
  }

  /* ------------------- 		STATE HANDLER		------------------- */
  /* ------------------------------------------------------------------------------ */
  switch (CHADEMO_Status) {
    case CHADEMO_IDLE:
      /* this is where we can unlock connector? */
      digitalWrite(CHADEMO_LOCK, LOW);
      plug_inserted = digitalRead(CHADEMO_PIN_7);

      if (!plug_inserted) {
#ifdef DEBUG_VIA_USB
//        Commented unless needed for debug
//        Serial.println("CHADEMO plug is not inserted.");
//
#endif
        return;
      }

      CHADEMO_Status = CHADEMO_CONNECTED;
#ifdef DEBUG_VIA_USB
      Serial.println("CHADEMO plug is inserted. Provide EVSE power to vehicle to trigger initialization.");
#endif

      break;
    case CHADEMO_CONNECTED:

      /* plug_inserted is .. essentially a volatile of sorts, so verify */
      if (plug_inserted) {
        /* If connection is detectable, jumpstart handshake by 
			 * indicate that the EVSE is ready to begin
			 */
        digitalWrite(CHADEMO_PIN_2, HIGH);

        /* State change to initializing. We will re-enter the handler upon receipt of CAN */
        CHADEMO_Status = CHADEMO_INIT;
      } else {
        /* this potentially-viewed-as-redundant condition checking is candidly
	 * an expression racy-relaties of the real world. Depending upon 
	 * testing/performance, it may be better to pepper this state handler
	 * with timers to have higher confidence of certain conditions hitting
	 * a steady state
	 */
#ifdef DEBUG_VIA_USB
        Serial.println("CHADEMO plug is not inserted, cannot connect d2 relay to begin initialization.");
#endif
        CHADEMO_Status = CHADEMO_IDLE;
      }
      break;
    case CHADEMO_INIT:
      /* Transient state while awaiting CAN from Vehicle.
       * Used for triggers/error handling elsewhere;
       * State change to CHADEMO_NEGOTIATE occurs in receive_can_battery(..)
       */
#ifdef DEBUG_VIA_USB
      Serial.println("Awaiting initial vehicle CAN to trigger negotiation");
#endif
      evse_init();
      break;
    case CHADEMO_NEGOTIATE:
      /* Vehicle and EVSE dance */
      //TODO if pin 4 / j goes high,

      Serial.print("State of charge: ");
      Serial.println(x102_chg_session.StateOfCharge);
      Serial.print("Parked?: ");
      Serial.println(x102_chg_session.s.status.StatusVehicleShifterPosition);
      Serial.print("Target Battery Voltage: ");
      Serial.println(x102_chg_session.TargetBatteryVoltage);

      x109_evse_state.s.status.ChgDischStopControl = 1;
      break;
    case CHADEMO_EV_ALLOWED:
      // pin 4 (j) reads high
      if (vehicle_permission) {
        //lock connector here
        digitalWrite(CHADEMO_LOCK, HIGH);

        //TODO spec requires test to validate solenoid has indeed engaged.
        // example uses a comparator/current consumption check around solenoid
        x109_evse_state.s.status.connector_locked = true;
      }
      CHADEMO_Status = CHADEMO_EVSE_PREPARE;

      break;
    case CHADEMO_EVSE_PREPARE:
      /* TODO voltage check of output < 10v */
      /* insulation test hypothetically happens here before triggering PIN 10 high */

      digitalWrite(CHADEMO_PIN_10, HIGH);
      evse_permission = true;

      // likely unnecessary but just to be sure. consider removal
      x109_evse_state.s.status.ChgDischStopControl = 1;
      x109_evse_state.s.status.EVSE_status = 0;
      //state changes only upon receipt of charging session request
      break;
    case CHADEMO_EVSE_START:
      datalayer.system.status.battery_allows_contactor_closing = true;
      x109_evse_state.s.status.ChgDischStopControl = 1;
      x109_evse_state.s.status.EVSE_status = 0;

      CHADEMO_Status = CHADEMO_EVSE_CONTACTORS_ENABLED;

      /* break rather than fall through because contactors are not instantaneous; 
       * worth giving it a cycle to finish
       */

      break;
    case CHADEMO_EVSE_CONTACTORS_ENABLED:

      /* check whether contactors ready, because externally dependent upon inverter allow during discharge */
      if (contactors_ready) {
        /* transition to POWERFLOW state if discharge compatible on both sides */
        if (x109_evse_state.discharge_compatible && x102_chg_session.s.status.StatusVehicleDischargeCompatible &&
            (EVSE_mode == CHADEMO_DISCHARGE || EVSE_mode == CHADEMO_BIDIRECTIONAL)) {
          CHADEMO_Status = CHADEMO_POWERFLOW;
          x109_evse_state.s.status.ChgDischStopControl = 0;
          x109_evse_state.s.status.EVSE_status = 1;
        }

        if (EVSE_mode == CHADEMO_CHARGE) {
          //TODO not supported currently
          //CHADEMO_Status = CHADEMO_POWERFLOW;
          //x109_evse_state.s.status.ChgDischStopControl = 0;
        }
      }

      /* break or fall through ? TODO */
      break;
    case CHADEMO_POWERFLOW:
      /* POWERFLOW for charging, discharging, and bidirectional */
      /* Interpretation */
      if (x102_chg_session.s.status.StatusVehicleShifterPosition) {
        /* Vehicle is no longer in park */
        // vehicle will switch k off, EVSE MAY see j (pin 4) go low
        // EVEN IF evse reads read pin 4 to see high, if this condition is true then trigger EVSE charge/discharge stop
        // 	per spec
        // SEPARATE check for pin 4/j condition does not depend on this flag. it's an OR condition
      }

      if (x102_chg_session.TargetBatteryVoltage == 0x00) {
        //TODO flag error and do not calculate power in EVSE response?
        //	probably unnecessary as other flags will be set causing this to be caught
      }

      // Potentially unnecessary (set in CHADEMO_EVSE_CONTACTORS_ENABLED stanza), but just in case
      x109_evse_state.s.status.EVSE_status = 1;
      x109_evse_state.s.status.ChgDischStopControl = 0;
      vehicle_permission = digitalRead(CHADEMO_PIN_4);
      break;
    case CHADEMO_STOP:
      /* back to CHADEMO_IDLE after teardown */
      x109_evse_state.s.status.ChgDischStopControl = 1;
      x109_evse_state.s.status.EVSE_status = 0;
      x109_evse_state.s.status.battery_incompatible = 0;
      digitalWrite(CHADEMO_PIN_10, LOW);
      digitalWrite(CHADEMO_PIN_2, LOW);
      evse_permission = false;
      vehicle_permission = false;
      x209_sent = false;
      x201_received = false;
      CHADEMO_Status = CHADEMO_IDLE;
      break;
    case CHADEMO_FAULT:
      /* Once faulted, never departs CHADEMO_FAULT state unless device is power cycled as a safety measure */
      x109_evse_state.s.status.EVSE_error = 1;
      x109_evse_state.s.status.ChgDischError = 1;
      x109_evse_state.s.status.ChgDischStopControl = 1;
#ifdef DEBUG_VIA_USB
      Serial.println("CHADEMO fault encountered, tearing down to make safe");
#endif
      digitalWrite(CHADEMO_PIN_10, LOW);
      digitalWrite(CHADEMO_PIN_2, LOW);
      evse_permission = false;
      vehicle_permission = false;
      x209_sent = false;
      x201_received = false;

      break;
    default:
#ifdef DEBUG_VIA_USB
      Serial.println("UNHANDLED CHADEMO_STATE, setting FAULT");
#endif
      CHADEMO_Status = CHADEMO_FAULT;
      break;
  }

  return;
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Chademo battery selected");
#endif

  CHADEMO_Status = CHADEMO_IDLE;

  /* disallow contactors until permissions is granted by vehicle */
  datalayer.system.status.battery_allows_contactor_closing = false;

  //TODO this is probably fine for a baseline, though CHADEMO can go as low as 150v and as high as 1500v in the latest revision
  //the below is relative to a 96 cell NMC. lower end is possibly too low
  datalayer.battery.info.max_design_voltage_dV =
      4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 2600;  // 260.0V under this, discharging further is disabled

  /* initialize EVSE data, state, and CAN frame representations */
  switch (EVSE_mode) {
    case CHADEMO_DISCHARGE:
    case CHADEMO_BIDIRECTIONAL:
      x109_evse_state.discharge_compatible = true;
      break;
    case CHADEMO_CHARGE:
      x109_evse_state.discharge_compatible = false;
      break;
    default:
      break;
  }

  x109_evse_state.s.status.ChgDischStopControl = 1;

  handle_chademo_sequence();
}
#endif
