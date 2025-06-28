#include "CHADEMO-BATTERY.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"
#include "CHADEMO-SHUNTS.h"

#ifdef CHADEMO_PIN_2  // Only support chademo for certain platforms

/* CHADEMO handling runs at 6.25 times the rate of most other code, so, rather than the
 *  default value of 12 (for 12 iterations of the 5s value update loop) * 5 for a 60s timeout,
 *  instead use 75 for 75*0.8s = 60s
 */
#undef CAN_STILL_ALIVE
#define CAN_STILL_ALIVE 75
//#define CH_CAN_DEBUG

//This function maps all the values fetched via CAN to the correct parameters used for the inverter
void ChademoBattery::update_values() {

  datalayer.battery.status.real_soc = x102_chg_session.StateOfCharge * 100;  //Convert % to pptt

  datalayer.battery.status.max_discharge_power_W =
      (x200_discharge_limits.MaximumDischargeCurrent * x100_chg_lim.MaximumBatteryVoltage);  //In Watts, Convert A to P

  if (vehicle_can_received) {  // Only update the value sent towards inverter if vehicle is connected (avoids false positive events)
    datalayer.battery.status.voltage_dV = get_measured_voltage() * 10;
  }

  datalayer.battery.info.total_capacity_Wh = (x101_chg_est.RatedBatteryCapacity * 100);
  //(Added in CHAdeMO v1.0.1), maybe handle hardcoded on lower protocol version?

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

  if (vehicle_can_received) {
    uint8_t chargingrate = 0;
    if (x100_chg_lim.ConstantOfChargingRateIndication > 0) {
      chargingrate = x102_chg_session.StateOfCharge / x100_chg_lim.ConstantOfChargingRateIndication * 100;
    }
  }
}

//TODO simplified start/stop helper functions
//see IEEE Table A.26—Charge control termination command pattern on pg58
//for stop conditions

void ChademoBattery::process_vehicle_charging_minimums(CAN_frame rx_frame) {
  x100_chg_lim.MinimumChargeCurrent = rx_frame.data.u8[0];
  x100_chg_lim.MinimumBatteryVoltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
  x100_chg_lim.MaximumBatteryVoltage = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);
  x100_chg_lim.ConstantOfChargingRateIndication = rx_frame.data.u8[6];
}

void ChademoBattery::process_vehicle_charging_maximums(CAN_frame rx_frame) {
  x101_chg_est.MaxChargingTime10sBit = rx_frame.data.u8[1];
  x101_chg_est.MaxChargingTime1minBit = rx_frame.data.u8[2];
  x101_chg_est.EstimatedChargingTime = rx_frame.data.u8[3];
  x101_chg_est.RatedBatteryCapacity = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[5]);
}

void ChademoBattery::process_vehicle_charging_session(CAN_frame rx_frame) {
  uint16_t newTargetBatteryVoltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[1]);
  uint16_t priorTargetBatteryVoltage = x102_chg_session.TargetBatteryVoltage;
  uint8_t newChargingCurrentRequest = rx_frame.data.u8[3];
  uint8_t priorChargingCurrentRequest = x102_chg_session.ChargingCurrentRequest;

  vehicle_can_initialized = true;

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
  x102_chg_session.s.status.StatusVehicleDischargeCompatible = bitRead(rx_frame.data.u8[5], 7);

  x102_chg_session.StateOfCharge = rx_frame.data.u8[6];

  //NOTE: behavior differs in the case of high current control (x110 support TBD)
  // In that mode, ChargingCurrentRequest is set to 0xFF when >= 1 A is specified in
  // in “request charging current (for extended) (x110.1, x110.2),” and then afterward in x102,
  // it will not be updated
  x102_chg_session.ChargingCurrentRequest = newChargingCurrentRequest;
  x102_chg_session.TargetBatteryVoltage = newTargetBatteryVoltage;

#ifdef DEBUG_LOG
  //Note on p131
  uint8_t chargingrate = 0;
  if (x100_chg_lim.ConstantOfChargingRateIndication > 0) {
    chargingrate = x102_chg_session.StateOfCharge / x100_chg_lim.ConstantOfChargingRateIndication * 100;
    logging.print("Charge Rate (kW): ");
    logging.println(chargingrate);
  }
#endif

  //Table A.26—Charge control termination command patterns -- should echo x108 handling

  /*  charge/discharge permission signal from vehicle on pin 4 should NOT be sensed before first CAN received from vehicle.
   *  Also, vehicle CAN should not simultaneously indicate enabled while permissions signal is absent
   *
   *  Either is a logical inconsistency per spec (vehicle has lost state, some wire/pin is broken, etc)
   *  	this should trigger stop and teardown
   */
  if ((CHADEMO_Status == CHADEMO_INIT && vehicle_permission) ||
      (x102_chg_session.s.status.StatusVehicleChargingEnabled && !vehicle_permission)) {
#ifdef DEBUG_LOG
    logging.println("Inconsistent charge/discharge state.");
#endif
    CHADEMO_Status = CHADEMO_FAULT;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryOverVoltage) {
#ifdef DEBUG_LOG
    logging.println("Vehicle indicates fault, battery over voltage.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryUnderVoltage) {
#ifdef DEBUG_LOG
    logging.println("Vehicle indicates fault, battery under voltage.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryCurrentDeviation) {
#ifdef DEBUG_LOG
    logging.println("Vehicle indicates fault, battery current deviation. Possible EVSE issue?");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (x102_chg_session.f.fault.FaultBatteryVoltageDeviation) {
#ifdef DEBUG_LOG
    logging.println("Vehicle indicates fault, battery voltage deviation. Possible EVSE issue?");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  // end
  if (priorTargetBatteryVoltage > 0 && newTargetBatteryVoltage == 0) {
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  //FIXME condition nesting or more stanzas needed here for clear determination of cessation reason
  if (CHADEMO_Status == CHADEMO_POWERFLOW && EVSE_mode == CHADEMO_CHARGE && !vehicle_permission) {
#ifdef DEBUG_LOG
    logging.println("State of charge ceiling reached or charging interrupted, stop charging");
#endif
    CHADEMO_Status = CHADEMO_STOP;
    return;
  }

  if (vehicle_permission && CHADEMO_Status == CHADEMO_NEGOTIATE) {
    CHADEMO_Status = CHADEMO_EV_ALLOWED;
#ifdef DEBUG_LOG
    logging.println("STATE shift to CHADEMO_EV_ALLOWED in process_vehicle_charging_session()");
#endif
    return;
  }

  // TODO this and the next stanza influence state/control
  //  and probably don't belong in this function
  // consider relocating
  if (vehicle_permission && CHADEMO_Status == CHADEMO_EVSE_PREPARE && priorTargetBatteryVoltage == 0 &&
      newTargetBatteryVoltage > 0 && x102_chg_session.s.status.StatusVehicleChargingEnabled) {
#ifdef DEBUG_LOG
    logging.println("STATE SHIFT to EVSE_START reached in process_vehicle_charging_session()");
#endif
    CHADEMO_Status = CHADEMO_EVSE_START;
    return;
  }

  if (vehicle_permission && evse_permission && CHADEMO_Status == CHADEMO_POWERFLOW) {
#ifdef DEBUG_LOG
    logging.println("updating vehicle request in process_vehicle_charging_session()");
#endif
    return;
  }

#ifdef DEBUG_LOG
  logging.println("UNHANDLED CHADEMO STATE, try unplugging chademo cable, reboot emulator, and retry!");
#endif
  return;
}

/* x200 Vehicle, peer to x208 EVSE */
void ChademoBattery::process_vehicle_charging_limits(CAN_frame rx_frame) {

  x200_discharge_limits.MaximumDischargeCurrent = rx_frame.data.u8[0];
  x200_discharge_limits.MinimumDischargeVoltage = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);
  x200_discharge_limits.MinimumBatteryDischargeLevel = rx_frame.data.u8[6];
  x200_discharge_limits.MaxRemainingCapacityForCharging = rx_frame.data.u8[7];

#ifdef DEBUG_LOG
/*  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
    logging.println("x200 Max remaining capacity for charging/discharging:");
    // initially this is set to 0, which is represented as 0xFF
    logging.println(0xFF - x200_discharge_limits.MaxRemainingCapacityForCharging);
  }
  */
#endif

  if (get_measured_voltage() <= x200_discharge_limits.MinimumDischargeVoltage && CHADEMO_Status > CHADEMO_NEGOTIATE) {
#ifdef DEBUG_LOG
    logging.println("x200 minimum discharge voltage met or exceeded, stopping.");
    logging.print("Measured: ");
    logging.print(get_measured_voltage());
    logging.print("Minimum voltage: ");
    logging.print(x200_discharge_limits.MinimumDischargeVoltage);
#endif
    CHADEMO_Status = CHADEMO_STOP;
  }
}

/* Vehicle 0x201, peer to EVSE 0x209 
 * HOWEVER, 201 isn't even emitted in any of the v2x canlogs available
 */
void ChademoBattery::process_vehicle_discharge_estimate(CAN_frame rx_frame) {
  unsigned long currentMillis = millis();

  x201_discharge_estimate.V2HchargeDischargeSequenceNum = rx_frame.data.u8[0];
  x201_discharge_estimate.ApproxDischargeCompletionTime = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[1]);
  x201_discharge_estimate.AvailableVehicleEnergy = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[3]);

#ifdef DEBUG_LOG
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
    logging.print("x201 availabile vehicle energy, completion time: ");
    logging.println(x201_discharge_estimate.AvailableVehicleEnergy);
    logging.print("x201 approx vehicle completion time: ");
    logging.println(x201_discharge_estimate.ApproxDischargeCompletionTime);
  }
#endif
}

void ChademoBattery::process_vehicle_dynamic_control(CAN_frame rx_frame) {
  //SM Dynamic Control = Charging station can increase of decrease "available output current" during charging.
  //If you set 0x110 byte 0, bit 0 to 1 you say you can do dynamic control.
  //Charging station communicates this in 0x118 byte 0, bit 0
  x110_vehicle_dyn.u.status.HighVoltageControlStatus = bitRead(rx_frame.data.u8[0], 2);
  x110_vehicle_dyn.u.status.HighCurrentControlStatus = bitRead(rx_frame.data.u8[0], 1);
  x110_vehicle_dyn.u.status.DynamicControlStatus = bitRead(rx_frame.data.u8[0], 0);
}

void ChademoBattery::process_vehicle_vendor_ID(CAN_frame rx_frame) {
  x700_vendor_id.AutomakerCode = rx_frame.data.u8[0];
  x700_vendor_id.OptionalContent =
      ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[1]);  //Actually more bytes, but not needed for our purpose
}

void ChademoBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
#ifdef CH_CAN_DEBUG
  logging.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  logging.print("  ");
  logging.print(rx_frame.ID, HEX);
  logging.print("  ");
  logging.print(rx_frame.DLC);
  logging.print("  ");
  for (int i = 0; i < rx_frame.DLC; ++i) {
    logging.print(rx_frame.data.u8[i], HEX);
    logging.print(" ");
  }
  logging.println("");
#endif

  // CHADEMO coexists with a CAN-based shunt. Only process CHADEMO-specific IDs
  // 202 is unknown
  if (!((rx_frame.ID >= 0x100 && rx_frame.ID <= 0x202) || rx_frame.ID == 0x700)) {
    return;
  }

  // used for testing vehicle sanity
  vehicle_can_received = true;
  /*  CHADEMO_INIT state is a transient, used to indicate when CAN
   *  has not yet been receied from a vehicle 
   */

  datalayer.battery.status.CAN_battery_still_alive =
      CAN_STILL_ALIVE;  //We are getting CAN messages from the vehicle, inform the watchdog

  switch (rx_frame.ID) {
    case 0x100:
      process_vehicle_charging_minimums(rx_frame);
      break;
    case 0x101:
      process_vehicle_charging_maximums(rx_frame);
      break;
    case 0x102:
      framecount++;
      //the first few frames start as 0x03, then like 20 of 0x01
      if (vehicle_can_initialized && framecount < 20) {
        return;
      }
      process_vehicle_charging_session(rx_frame);
      /* counter to help discard inital frames with bad SOC data */
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
      break;
    case 0x700:
      process_vehicle_vendor_ID(rx_frame);
      break;
    case 0x202:  // unknown. LEAF specific?
    default:
      break;
  }

  if (CHADEMO_Status == CHADEMO_INIT) {
    // First CAN messages received, entering into negotiation
    // TODO consider tracking delta since transition time for expiry
    CHADEMO_Status = CHADEMO_NEGOTIATE;
  }

  handle_chademo_sequence();

  ISA_handleFrame(&rx_frame);
}

/* (re)initialize evse structures to pre-charge/discharge states */
void ChademoBattery::evse_init() {
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
void ChademoBattery::update_evse_capabilities(CAN_frame& f) {

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

  /* should this be set to MAX_EVSE_OUTPUT_VOLTAGE or x102_chg_session.TargetBatteryVoltage ? */
  x108_evse_cap.available_output_voltage = MAX_EVSE_OUTPUT_VOLTAGE;

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
void ChademoBattery::update_evse_status(CAN_frame& f) {

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
    x109_evse_state.setpoint_HV_VDC = get_measured_voltage();
    x109_evse_state.setpoint_HV_IDC = get_measured_current();

    /*For posterity if anyone is forced to simulate a shunt
      NOTE: these are supposed to be measured values, e.g., from a shunt
      If a sensor is not used, we are literally asserting that the measured value is exactly equivalent to the request or max charger capability
      this is pretty likely to fail on most vehicles
    x109_evse_state.setpoint_HV_VDC =
        min(x102_chg_session.TargetBatteryVoltage, x108_evse_cap.available_output_voltage);
    x109_evse_state.setpoint_HV_IDC =
        min(x102_chg_session.ChargingCurrentRequest, x108_evse_cap.available_output_current);
    */

    /* The spec suggests throwing a 109.5.4 = 1 if vehicle curr request 102.3 > evse curr available 108.3, 
     *  but realistically many chargers seem to act tolerant here and stay under limits and supply whatever they are able
     */

    /* if power overcommitted, back down to just below while maintaining voltage target */
    if (x109_evse_state.setpoint_HV_VDC > 0 &&
        x109_evse_state.setpoint_HV_IDC * x109_evse_state.setpoint_HV_VDC > MAX_EVSE_POWER_CHARGING) {
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
      (x100_chg_lim.MaximumBatteryVoltage < x108_evse_cap.threshold_voltage)) {
    //Toggle battery incompatibility flag 109.5.3
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
void ChademoBattery::update_evse_discharge_estimate(CAN_frame& f) {

  //x209_evse_dischg_est.remaining_discharge_time_1m = x201_discharge_estimate.ApproxDischargeCompletionTime;

  //x201_discharge_estimate.AvailableVehicleEnergy = 0;

  //do nothing to alter the default initialized values..this may be unneeded
  /* TODO
	if (x200_discharge_limits.MaxRemainingCapacityForCharging = max charge voltage){
	if (x200_discharge_limits.MinimumBatteryDischargeLevel = kwH for v2h<1.0){
	if (x200_discharge_limits.MaxRemainingCapacityForCharging = kwH for v2h<1.0){
	*/

  CHADEMO_209.data.u8[0] = x209_evse_dischg_est.sequence_control_number;
  CHADEMO_209.data.u8[1] = lowByte(x209_evse_dischg_est.remaining_discharge_time);
  CHADEMO_209.data.u8[2] = highByte(x209_evse_dischg_est.remaining_discharge_time);
}

/* x208 EVSE, peer to 0x200 Vehicle */
void ChademoBattery::update_evse_discharge_capabilities(CAN_frame& f) {
  //present discharge current is a measured value
  x208_evse_dischg_cap.present_discharge_current = 0xFF - get_measured_current();

  /* Present discharge current is a measured value. In the absence of
     a shunt, the evse here is quite literally lying to the vehicle. The spec
     seems to suggest this is tolerated unless the current measured on the EV
     side continualy exceeds the maximum discharge current by 10amps
  x208_evse_dischg_cap.present_discharge_current = 0xFF - 6;
  */

  //EVSE maximum current input is partly an inverter-influenced value i.e., min(inverter, vehicle_max_discharge)
  //use max_discharge_current variable if nonzero, otherwise tell the vehicle the EVSE will take everything it can give
  if (max_discharge_current) {
    x208_evse_dischg_cap.available_input_current = 0xFF - max_discharge_current;
  } else {
    x208_evse_dischg_cap.available_input_current = 0xFF - x200_discharge_limits.MaximumDischargeCurrent;
  }

  x208_evse_dischg_cap.available_input_voltage = x200_discharge_limits.MinimumDischargeVoltage;

  /* calculate min threshold to protect battery - using vehicle-provided minimum plus 2%
   *
   *As this is partly an inverter-influenced value, should this be a configurable variable backed by a defined default?
   *	It seems sensible to be MAX(lowest usable voltage of the inverter input, lowest tolerable voltage of the vehicle battery)
   *	NOT the vehicle minimumDischargeVoltage.
   *	Thus, why here we are adding a few percent of cushion atop the minimum
   *	This is the reverse treatment of the lower_threshold_voltage of charging mode
   */
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

void ChademoBattery::transmit_can(unsigned long currentMillis) {

  handlerBeforeMillis = currentMillis;
  handle_chademo_sequence();
  handlerAfterMillis = millis();

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    /* no EVSE messages should be sent until the vehicle has
     * initiated
     */
    //  if (CHADEMO_Status <= CHADEMO_INIT || !vehicle_can_received) {
    if (CHADEMO_Status <= CHADEMO_INIT) {
      return;
    }

    update_evse_capabilities(CHADEMO_108);
    update_evse_status(CHADEMO_109);
    update_evse_discharge_capabilities(CHADEMO_208);
    update_evse_discharge_estimate(CHADEMO_209);

    /* most updates to these EVSE frames are made
     * upon receipt of a Vehicle message, as 
     * that is the limiting factor. Therefore, we
     * can generally send as is without tweaks here.
     */
    transmit_can_frame(&CHADEMO_108, can_config.battery);
    transmit_can_frame(&CHADEMO_109, can_config.battery);

    /* TODO for dynamic control: can send x118 with byte 6 bit 0 set to 0 for 1s (before flipping back to 1) as a way of giving vehicle a chance to update 101.1 and 101.2
     * 	within 6 seconds of x118 toggle.
     * 	Then 109.6 and 109.7 reset remaining charging time
     * 	see A.11.5.3.1.3 Remaining charging time (H’109.6, H’109.7) for a better description
     */

    if (EVSE_mode == CHADEMO_DISCHARGE || EVSE_mode == CHADEMO_BIDIRECTIONAL) {
      transmit_can_frame(&CHADEMO_208, can_config.battery);
      if (x201_received) {
        transmit_can_frame(&CHADEMO_209, can_config.battery);
        x209_sent = true;
      }
    }

    // TODO need an update_evse_dynamic_control(..) function above before we send 118
    // 	110.0.0
    if (x102_chg_session.ControlProtocolNumberEV >= 0x03) {  //Only send the following on Chademo 2.0 vehicles?
#ifdef DEBUG_LOG
      //FIXME REMOVE
      logging.println("REMOVE: proto 2.0");
#endif
      transmit_can_frame(&CHADEMO_118, can_config.battery);
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
void ChademoBattery::handle_chademo_sequence() {

  precharge_low = digitalRead(PRECHARGE_PIN) == LOW;
  positive_high = digitalRead(POSITIVE_CONTACTOR_PIN) == HIGH;
  contactors_ready = precharge_low && positive_high;
  vehicle_permission = digitalRead(CHADEMO_PIN_4);

  /* -------------------    State override conditions checks	------------------- */
  /* ------------------------------------------------------------------------------ */
  if (CHADEMO_Status >= CHADEMO_EV_ALLOWED && x102_chg_session.s.status.StatusVehicleShifterPosition) {
#ifdef DEBUG_LOG
    logging.println("Vehicle is not parked, abort.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
  }

  if (CHADEMO_Status >= CHADEMO_EV_ALLOWED && !vehicle_permission) {
#ifdef DEBUG_LOG
    logging.println("Vehicle charge/discharge permission ended, stop.");
#endif
    CHADEMO_Status = CHADEMO_STOP;
  }

  /* ------------------- 		STATE HANDLER		------------------- */
  /* ------------------------------------------------------------------------------ */
  switch (CHADEMO_Status) {
    case CHADEMO_IDLE:
      /* this is where we can unlock connector */
      digitalWrite(CHADEMO_LOCK, LOW);
      plug_inserted = digitalRead(CHADEMO_PIN_7);

      if (!plug_inserted) {
#ifdef DEBUG_LOG
//        Commented unless needed for debug
//        logging.println("CHADEMO plug is not inserted.");
#endif
        return;
      }

      CHADEMO_Status = CHADEMO_CONNECTED;
#ifdef DEBUG_LOG
      logging.println("CHADEMO plug is inserted. Provide EVSE power to vehicle to trigger initialization.");
#endif

      break;
    case CHADEMO_CONNECTED:

#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      //logging.println("CHADEMO_CONNECTED State");
#endif
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
#ifdef DEBUG_LOG
        logging.println("CHADEMO plug is not inserted, cannot connect d2 relay to begin initialization.");
#endif
        CHADEMO_Status = CHADEMO_IDLE;
      }
      break;
    case CHADEMO_INIT:
      /* Transient state while awaiting CAN from Vehicle.
       * Used for triggers/error handling elsewhere;
       * State change to CHADEMO_NEGOTIATE occurs in handle_incoming_can_frame_battery(..)
       */
#ifdef DEBUG_LOG
//      logging.println("Awaiting initial vehicle CAN to trigger negotiation");
#endif
      evse_init();
      break;
    case CHADEMO_NEGOTIATE:
      /* Vehicle and EVSE dance */
      //TODO if pin 4 / j goes high,

#ifdef DEBUG_LOG
//        Commented unless needed for debug
//        logging.println("CHADEMO_NEGOTIATE State");
#endif
      x109_evse_state.s.status.ChgDischStopControl = 1;
      break;
    case CHADEMO_EV_ALLOWED:
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_EV_ALLOWED State");
#endif
      // If we are in this state, vehicle_permission was already set to true...but re-verify
      // that pin 4 (j) reads high
      if (vehicle_permission) {
        //lock connector here
        digitalWrite(CHADEMO_LOCK, HIGH);

        //TODO spec requires test to validate solenoid has indeed engaged.
        // example uses a comparator/current consumption check around solenoid
        x109_evse_state.s.status.connector_locked = true;

        CHADEMO_Status = CHADEMO_EVSE_PREPARE;
      }
      break;
    case CHADEMO_EVSE_PREPARE:
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_EVSE_PREPARE State");
#endif
      /* TODO voltage check of output < 20v 
       * insulation test hypothetically happens here before triggering PIN 10 high
       * see Table A.28—Requirements for the insulation test for output DC circuit
	Note: required that if 102.5.0 == 0, do not perform evse insulation test
	we should not be here in this state unless 102.5.0 was == 1 previously, but check again in case it has changed

	simulate via?
                    if evse_present _voltage + 10 <= vehicle voltage_target {
                        evse_present_voltage += 10;
                    } else {
                        evse_present_voltage = vehicle voltage_target;
                    }
       */
      if (x102_chg_session.s.status.StatusVehicleChargingEnabled) {
        if (get_measured_voltage() < 20) {

          digitalWrite(CHADEMO_PIN_10, HIGH);
          evse_permission = true;
        } else {
          logging.println("Insulation check measures > 20v ");
        }

        // likely unnecessary but just to be sure. consider removal
        x109_evse_state.s.status.ChgDischStopControl = 1;
        x109_evse_state.s.status.EVSE_status = 0;
      } else {
        CHADEMO_Status = CHADEMO_STOP;
      }

      //state changes to CHADEMO_EVSE_START only upon receipt of charging session request
      break;
    case CHADEMO_EVSE_START:
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_EVSE_START State");
#endif
      datalayer.system.status.battery_allows_contactor_closing = true;
      x109_evse_state.s.status.ChgDischStopControl = 1;
      x109_evse_state.s.status.EVSE_status = 0;

      CHADEMO_Status = CHADEMO_EVSE_CONTACTORS_ENABLED;

#ifdef DEBUG_LOG
      logging.println("Initiating contactors");
#endif

      /* break rather than fall through because contactors are not instantaneous; 
       * worth giving it a cycle to finish
       */

      break;
    case CHADEMO_EVSE_CONTACTORS_ENABLED:
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_EVSE_CONTACTORS State");
#endif

      /* check whether contactors ready, because externally dependent upon inverter allow during discharge */
      if (contactors_ready) {
#ifdef DEBUG_LOG
        logging.println("Contactors ready");
        logging.print("Voltage: ");
        logging.println(get_measured_voltage());
#endif
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
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_POWERFLOW State");
#endif
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

      if (get_measured_voltage() <= x200_discharge_limits.MinimumDischargeVoltage) {
#ifdef DEBUG_LOG
        logging.println("x200 minimum discharge voltage met or exceeded, stopping.");
#endif
        CHADEMO_Status = CHADEMO_STOP;
      }

      // Potentially unnecessary (set in CHADEMO_EVSE_CONTACTORS_ENABLED stanza), but just in case
      x109_evse_state.s.status.ChgDischStopControl = 0;
      x109_evse_state.s.status.EVSE_status = 1;
      break;
    case CHADEMO_STOP:
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_STOP State");
#endif
      /* back to CHADEMO_IDLE after teardown */
      x109_evse_state.s.status.ChgDischStopControl = 1;
      x109_evse_state.s.status.EVSE_status = 0;
      x109_evse_state.s.status.battery_incompatible = 0;
      evse_permission = false;
      vehicle_permission = false;
      x209_sent = false;
      x201_received = false;

      /* protection of EV contactors - IEEE A.7.2.9 Protection of EV contactor
       *  see also Table A.29—Charging stage and check item
       *
       * We will re-enter the handler until the amperage drops sufficiently
       * and then transition to CHADEMO_IDLE
       */
      if (get_measured_current() <= 5 && get_measured_voltage() <= 10) {
        /* welding detection ideally here */
        digitalWrite(CHADEMO_PIN_10, LOW);
        digitalWrite(CHADEMO_PIN_2, LOW);
        CHADEMO_Status = CHADEMO_IDLE;
      }

      break;
    case CHADEMO_FAULT:
#ifdef DEBUG_LOG
      //        Commented unless needed for debug
      logging.println("CHADEMO_FAULT State");
#endif
      /* Once faulted, never departs CHADEMO_FAULT state unless device is power cycled as a safety measure */
      x109_evse_state.s.status.EVSE_error = 1;
      x109_evse_state.s.status.ChgDischError = 1;
      x109_evse_state.s.status.ChgDischStopControl = 1;
#ifdef DEBUG_LOG
      logging.println("CHADEMO fault encountered, tearing down to make safe");
#endif
      digitalWrite(CHADEMO_PIN_10, LOW);
      digitalWrite(CHADEMO_PIN_2, LOW);
      evse_permission = false;
      vehicle_permission = false;
      x209_sent = false;
      x201_received = false;

      break;
    default:
#ifdef DEBUG_LOG
      logging.println("UNHANDLED CHADEMO_STATE, setting FAULT");
#endif
      CHADEMO_Status = CHADEMO_FAULT;
      break;
  }

  return;
}

void ChademoBattery::setup(void) {  // Performs one time setup at startup

  pinMode(CHADEMO_PIN_2, OUTPUT);
  digitalWrite(CHADEMO_PIN_2, LOW);
  pinMode(CHADEMO_PIN_10, OUTPUT);
  digitalWrite(CHADEMO_PIN_10, LOW);
  pinMode(CHADEMO_LOCK, OUTPUT);
  digitalWrite(CHADEMO_LOCK, LOW);
  pinMode(CHADEMO_PIN_4, INPUT);
  pinMode(CHADEMO_PIN_7, INPUT);

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  CHADEMO_Status = CHADEMO_IDLE;

  /* disallow contactors until permissions is granted by vehicle */
  datalayer.system.status.battery_allows_contactor_closing = false;

  /* Pretend that we know the SOH, assert that it is 99% */
  datalayer.battery.status.soh_pptt = 9900;

  /* Briefly assert that we're starting at a modest SOC of 30% */
  datalayer.battery.status.real_soc = 300;

  //TODO Must be user configured, most likely. Artificially capped for the time being
  datalayer.battery.status.max_discharge_power_W = 1000;
  datalayer.battery.status.max_charge_power_W = 1000;

  datalayer.battery.status.current_dA = 0;
  datalayer.battery.status.remaining_capacity_Wh = 12000;

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
  //  ISA_deFAULT();        // ISA Setup - it is sufficient to set it once, because it is saved in SUNT
  //  ISA_initialize();     // ISA Setup - it is sufficient to set it once, because it is saved in SUNT
  //  ISA_RESTART();

  setupMillis = millis();
}

#endif
