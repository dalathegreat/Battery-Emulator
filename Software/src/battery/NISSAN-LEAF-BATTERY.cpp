#include "NISSAN-LEAF-BATTERY.h"
#include <cstring>  //For unit test
#include "../charger/CHARGERS.h"
#include "../charger/CanCharger.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"     //For "More battery info" webpage
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

uint16_t Temp_fromRAW_to_F(uint16_t temperature);
//Cryptographic functions
void decodeChallengeData(unsigned int SeedInput, unsigned char* Crypt_Output_Buffer);
unsigned int CyclicXorHash16Bit(unsigned int param_1, unsigned int param_2);
unsigned int ComputeMaskedXorProduct(unsigned int param_1, unsigned int param_2, unsigned int param_3);
short ShortMaskedSumAndProduct(short param_1, short param_2);
unsigned int MaskedBitwiseRotateMultiply(unsigned int param_1, unsigned int param_2);
unsigned int CryptAlgo(unsigned int param_1, unsigned int param_2, unsigned int param_3);

// Note this should only be allowed/used on 2011-2017 24/30kWh batteries!
bool NissanLeafBattery::supports_reset_SOH() {
  return LEAF_battery_Type != ZE1_BATTERY;
}

void NissanLeafBattery::set_balancing_status(balancing_status_enum new_status) {
  if (new_status == datalayer_battery->status.balancing_status) {
    return;
  }
  if (new_status == BALANCING_STATUS_ACTIVE) {
    set_event_latched(EVENT_BALANCING_START, 0);
  } else if (datalayer_battery->status.balancing_status == BALANCING_STATUS_ACTIVE) {
    set_event(EVENT_BALANCING_END, 0);  //Only fired when leaving ACTIVE, never on the initial UNKNOWN transition
  }
  datalayer_battery->status.balancing_status = new_status;
}

void NissanLeafBattery::
    update_values() { /* This function maps all the values fetched via CAN to the correct parameters used for modbus */
  /* Start with mapping all values */

  datalayer_battery->status.soh_pptt = (battery_StateOfHealth * 100);  //Increase range from 99% -> 99.00%

  datalayer_battery->status.real_soc = (battery_SOC * 10);

  datalayer_battery->status.voltage_dV =
      (battery_Total_Voltage2 * 5);  //0.5V/bit, multiply by 5 to get Voltage+1decimal (350.5V = 701)

  datalayer_battery->status.current_dA =
      (battery_Current2 * 5);  //0.5A/bit, multiply by 5 to get Amp+1decimal (5,5A = 11)

  datalayer_battery->info.total_capacity_Wh = ((battery_Max_GIDS * WH_PER_GID * battery_StateOfHealth) / 100);

  datalayer_battery->status.remaining_capacity_Wh = battery_Wh_Remaining;

  //Update temperature readings. Method depends on which generation LEAF battery is used
  if (LEAF_battery_Type == ZE0_BATTERY) {
    //Since we only have average value, send the minimum as -1.0 degrees below average
    datalayer_battery->status.temperature_min_dC =
        ((battery_AverageTemperature * 10) - 10);  //Increase range from C to C+1, remove 1.0C
    datalayer_battery->status.temperature_max_dC = (battery_AverageTemperature * 10);  //Increase range from C to C+1
  } else if (LEAF_battery_Type == AZE0_BATTERY) {
    //Use the value sent constantly via CAN in 5C0 (only available on AZE0)
    //Only update when both values have been read from the muxed message
    if ((battery_HistData_Temperature_MIN != 86) && (battery_HistData_Temperature_MAX != 86)) {
      datalayer_battery->status.temperature_min_dC =
          (battery_HistData_Temperature_MIN * 10);  //Increase range from C to C+1
      datalayer_battery->status.temperature_max_dC =
          (battery_HistData_Temperature_MAX * 10);  //Increase range from C to C+1
    }
  } else {  // ZE1 (TODO: Once the muxed value in 5C0 becomes known, switch to using that instead of this complicated polled value)
    if (battery_temp_raw_min != 0)  //We have a polled value available
    {
      battery_temp_polled_min = ((Temp_fromRAW_to_F(battery_temp_raw_min) - 320) * 5) / 9;  //Convert from F to C
      battery_temp_polled_max = ((Temp_fromRAW_to_F(battery_temp_raw_max) - 320) * 5) / 9;  //Convert from F to C
      if (battery_temp_polled_min < battery_temp_polled_max) {  //Catch any edge cases from Temp_fromRAW_to_F function
        datalayer_battery->status.temperature_min_dC = battery_temp_polled_min;
        datalayer_battery->status.temperature_max_dC = battery_temp_polled_max;
      } else {
        datalayer_battery->status.temperature_min_dC = battery_temp_polled_max;
        datalayer_battery->status.temperature_max_dC = battery_temp_polled_min;
      }
    }
  }

  datalayer_battery->status.max_discharge_power_W = (battery_Discharge_Power_Limit * 1000);  //kW to W

  datalayer_battery->status.max_charge_power_W = (battery_Charge_Power_Limit * 1000);  //kW to W

  //Allow contactors to close
  if (battery_can_alive && allows_contactor_closing) {
    *allows_contactor_closing = true;
  }

  /*Extra safety functions below*/
  if (battery_GIDS < 10)  //700Wh left in battery!
  {                       //Battery is running abnormally low, some discharge logic might have failed. Zero it all out.
    set_event(EVENT_BATTERY_EMPTY, 0, battery_index);
    datalayer_battery->status.real_soc = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
  }

  if (battery_Full_CHARGE_flag) {  //Battery reports that it is fully charged stop all further charging incase it hasn't already
    set_event(EVENT_BATTERY_FULL, 0, battery_index);
    datalayer_battery->status.max_charge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_FULL, battery_index);
  }

  if (battery_Capacity_Empty) {  //Battery reports that it is fully discharged. Stop all further discharging incase it hasn't already
    set_event(EVENT_BATTERY_EMPTY, 0, battery_index);
    datalayer_battery->status.max_discharge_power_W = 0;
  } else {
    clear_event(EVENT_BATTERY_EMPTY, battery_index);
  }

  if (battery_Total_Voltage2 == 0x3FF) {  //Battery reports critical measurement unavailable
    set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, 0, battery_index);
  } else {
    clear_event(EVENT_BATTERY_VALUE_UNAVAILABLE, battery_index);
  }

  if (battery_Relay_Cut_Request) {  //battery_FAIL, BMS requesting shutdown and contactors to be opened
    //Note, this is sometimes triggered during the night while idle, and the BMS recovers after a while. Removed latching from this scenario
    datalayer_battery->status.max_discharge_power_W = 0;
    datalayer_battery->status.max_charge_power_W = 0;
  }

  if (battery_Failsafe_Status > 0)  // 0 is normal, start charging/discharging
  {
    switch (battery_Failsafe_Status) {
      case (1):
        //Normal Stop Request
        //This means that battery is fully discharged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
        break;
      case (2):
        //Charging Mode Stop Request
        //This means that battery is fully charged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
        break;
      case (3):
        //Charging Mode Stop Request & Normal Stop Request
        //Normal stop request. For stationary storage we don't disconnect contactors, so we ignore this.
        break;
      case (4):
        //Caution Lamp Request
        set_event(EVENT_BATTERY_CAUTION, 0, battery_index);
        break;
      case (5):
        //Caution Lamp Request & Normal Stop Request
        set_event(EVENT_BATTERY_DISCHG_STOP_REQ, 0, battery_index);
        break;
      case (6):
        //Caution Lamp Request & Charging Mode Stop Request
        set_event(EVENT_BATTERY_CHG_STOP_REQ, 0, battery_index);
        break;
      case (7):
        //Caution Lamp Request & Charging Mode Stop Request & Normal Stop Request
        set_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ, 0, battery_index);
        break;
      default:
        break;
    }
  } else {  //battery_Failsafe_Status == 0
    clear_event(EVENT_BATTERY_DISCHG_STOP_REQ, battery_index);
    clear_event(EVENT_BATTERY_CHG_STOP_REQ, battery_index);
    clear_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ, battery_index);
  }

  if (user_selected_LEAF_interlock_mandatory) {
    //If user requires both large 80kW and small 6kW interlock to be seated for operation
    if (!battery_Interlock) {
      set_event(EVENT_HVIL_FAILURE, 0);
    } else {
      clear_event(EVENT_HVIL_FAILURE);
    }
  }

  if (datalayer_battery->status.cell_max_voltage_mV > 60000 || datalayer_battery->status.cell_min_voltage_mV > 60000) {
    set_event(EVENT_12V_LOW, 0);
    //This is a bit of a hack, but we don't have a dedicated event for "12V low" and this is the first indicator of low 12V
  } else {
    clear_event(EVENT_12V_LOW);
  }

  if (battery_HeatExist) {
    if (battery_Heating_Stop) {
      set_event(EVENT_BATTERY_WARMED_UP, 0, battery_index);
    }
    if (battery_Heating_Start) {
      set_event(EVENT_BATTERY_REQUESTS_HEAT, 0, battery_index);
    }
  }

  // Classify balancing from the per-cell shunt bits polled from LBC group 0x06. A populated bitmap on
  // its own does not mean the pack is balancing: the LBC flags a set of cells and can hold it for a
  // day at a time while it waits for the pack to settle, at one point flagging every shunt it has and
  // holding that perfectly static for 20 hours. During a real balance it duty-cycles the shunts,
  // bleeding cells and re-deciding the set after each measurement. How *many* shunts move per read is
  // a poor measure of that, because it climbs steadily as the balance proceeds and the flagged set
  // shrinks. How *often* a read comes back completely unchanged is far more stable: across four
  // balancing sessions of a 2017 30 kWh pack it stayed at 12% of reads whatever the pack state, while
  // every pending phase sat at 82-100%. So count unchanged reads over a sliding window and compare
  // that against a pair of thresholds, holding the previous status in between so the state does not
  // chatter at the boundary.
  // Evaluated only on a complete group 0x06 response (~70s apart), never on the 1s update tick.
  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    balancing_bitmap_valid = false;  //LBC is being power cycled, the previous classification is void
    balancing_unchanged_window = 0;
    balancing_window_fill = 0;
    balancing_low_reads = 0;
    set_balancing_status(BALANCING_STATUS_UNKNOWN);
  }

  if (balancing_data_fresh) {
    balancing_data_fresh = false;

    uint32_t balancing_bitmap[3] = {0, 0, 0};
    uint8_t balancing_active_cells = 0;
    for (uint8_t i = 0; i < 96; i++) {
      if (battery_balancing_shunts[i]) {
        balancing_bitmap[i / 32] |= (1UL << (i % 32));
        balancing_active_cells++;
      }
    }

    if (balancing_active_cells < BALANCING_READY_BELOW_CELLS) {
      //Nothing flagged. Wait for this to repeat before believing it: an incomplete group 0x06 response
      //reads as all-clear for a single poll, which would otherwise end the phase early.
      if (balancing_low_reads < BALANCING_READY_DEBOUNCE_READS) {
        balancing_low_reads++;
      }
      if (balancing_low_reads >= BALANCING_READY_DEBOUNCE_READS) {
        balancing_bitmap_valid = false;  //Phase is over, start clean if balancing ever comes back
        balancing_unchanged_window = 0;
        balancing_window_fill = 0;
        set_balancing_status(BALANCING_STATUS_READY);
      }
    } else {
      //Compare against the previous read. Skip the read that follows a low-count one, since the
      //bitmap it would be compared against is the suspect all-clear sample.
      bool comparison_valid = balancing_bitmap_valid && (balancing_low_reads == 0);
      balancing_low_reads = 0;

      if (comparison_valid) {
        bool unchanged = (memcmp(balancing_bitmap, balancing_bitmap_prev, sizeof(balancing_bitmap)) == 0);

        //Shift the window along, recording whether this read came back unchanged
        balancing_unchanged_window = (uint16_t)(balancing_unchanged_window << 1) | (unchanged ? 1 : 0);
        balancing_unchanged_window &= (uint16_t)((1UL << BALANCING_WINDOW_READS) - 1);

        if (balancing_window_fill < BALANCING_WINDOW_READS) {
          balancing_window_fill++;
        }

        uint8_t unchanged_reads = 0;
        for (uint16_t bits = balancing_unchanged_window; bits; bits &= bits - 1) {
          unchanged_reads++;
        }

        //A partly filled window cannot be told apart from a busy one: both report few unchanged reads.
        //Decide nothing until it is full, so the status stays UNKNOWN after a boot or a BMS reset
        //rather than reporting a balance that has not been observed yet.
        if (balancing_window_fill < BALANCING_WINDOW_READS) {
          //Not enough history yet, hold the current status
        } else if (unchanged_reads >= BALANCING_UNCHANGED_FOR_IDLE) {
          set_balancing_status(BALANCING_STATUS_BLOCKED);  //Holding a set: flagged, but not yet at rest
        } else if (unchanged_reads <= BALANCING_UNCHANGED_FOR_ACTIVE) {
          set_balancing_status(BALANCING_STATUS_ACTIVE);  //Re-deciding the set steadily: really balancing
        }
        //else: between the thresholds, hold the current status
      }

      memcpy(balancing_bitmap_prev, balancing_bitmap, sizeof(balancing_bitmap));
      balancing_bitmap_valid = true;
    }
  }

  // Update webserver datalayer
  if (datalayer_nissan) {
    memcpy(datalayer_nissan->BatterySerialNumber, BatterySerialNumber, sizeof(BatterySerialNumber));
    memcpy(datalayer_nissan->BatteryPartNumber, BatteryPartNumber, sizeof(BatteryPartNumber));
    datalayer_nissan->LEAF_gen = LEAF_battery_Type;
    if (allows_contactor_closing) {  //Only the main battery names the protocol shown on the status page
      //setup() already wrote Name, so only the "battery" part gets replaced by the detected generation
      static_assert(Name[sizeof("Nissan LEAF ") - 1] == 'b', "Name must start with \"Nissan LEAF \"");
      static const char LEAF_gen_name[3][5] = {"ZE0", "AZE0", "ZE1"};
      strcpy(datalayer.system.info.battery_protocol + sizeof("Nissan LEAF ") - 1, LEAF_gen_name[LEAF_battery_Type]);
    }
    datalayer_nissan->GIDS = battery_GIDS;
    datalayer_nissan->ChargePowerLimit = battery_Charge_Power_Limit;
    datalayer_nissan->MaxPowerForCharger = battery_MAX_POWER_FOR_CHARGER;
    datalayer_nissan->Interlock = battery_Interlock;
    datalayer_nissan->Insulation = battery_insulation;
    datalayer_nissan->RelayCutRequest = battery_Relay_Cut_Request;
    datalayer_nissan->FailsafeStatus = battery_Failsafe_Status;
    datalayer_nissan->Full = battery_Full_CHARGE_flag;
    datalayer_nissan->Empty = battery_Capacity_Empty;
    datalayer_nissan->MainRelayOn = battery_MainRelayOn_flag;
    datalayer_nissan->HeatExist = battery_HeatExist;
    datalayer_nissan->HeatingStop = battery_Heating_Stop;
    datalayer_nissan->HeatingStart = battery_Heating_Start;
    datalayer_nissan->HeaterSendRequest = battery_Batt_Heater_Mail_Send_Request;
    datalayer_nissan->battery_HX_pptt = battery_HX_pptt;
    datalayer_nissan->ChargeCountQC = battery_charge_count_qc;
    datalayer_nissan->ChargeCountL1L2 = battery_charge_count_l1l2;
    datalayer_nissan->temperature1 = ((Temp_fromRAW_to_F(battery_temp_raw_1) - 320) * 5) / 9;  //Convert from F to C
    datalayer_nissan->temperature2 = ((Temp_fromRAW_to_F(battery_temp_raw_2) - 320) * 5) / 9;  //Convert from F to C
    datalayer_nissan->temperature3 = ((Temp_fromRAW_to_F(battery_temp_raw_3) - 320) * 5) / 9;  //Convert from F to C
    datalayer_nissan->temperature4 = ((Temp_fromRAW_to_F(battery_temp_raw_4) - 320) * 5) / 9;  //Convert from F to C
#ifndef SMALL_FLASH_DEVICE
    datalayer_nissan->CryptoChallenge = incomingChallenge;
    datalayer_nissan->SolvedChallengeMSB =
        ((solvedChallenge[7] << 24) | (solvedChallenge[6] << 16) | (solvedChallenge[5] << 8) | solvedChallenge[4]);
    datalayer_nissan->SolvedChallengeLSB =
        ((solvedChallenge[3] << 24) | (solvedChallenge[2] << 16) | (solvedChallenge[1] << 8) | solvedChallenge[0]);
    datalayer_nissan->challengeFailed = challengeFailed;

    // Update requests from webserver datalayer
    if (UserRequestSOHreset) {
      stateMachineClearSOH = 0;  //Start the statemachine
      UserRequestSOHreset = false;
    }

#endif
  }
}

void NissanLeafBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1DB:
      if (is_message_corrupt(rx_frame)) {
        datalayer_battery->status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_Current2 = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
      if (battery_Current2 & 0x0400) {
        // negative so extend the sign bit
        battery_Current2 |= 0xf800;
      }  //BatteryCurrentSignal , 2s comp, 1lSB = 0.5A/bit

      battery_TEMP = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6);  //0.5V/bit
      if (battery_TEMP != 0x3ff) {  //3FF is unavailable value. Can happen directly on reboot.
        battery_Total_Voltage2 = battery_TEMP;
      }

      //Collect various data from the BMS
      battery_Relay_Cut_Request = ((rx_frame.data.u8[1] & 0x18) >> 3);
      battery_Failsafe_Status = (rx_frame.data.u8[1] & 0x07);
      battery_MainRelayOn_flag = (bool)((rx_frame.data.u8[3] & 0x20) >> 5);
      battery_Full_CHARGE_flag = (bool)((rx_frame.data.u8[3] & 0x10) >> 4);
      battery_Interlock = (bool)((rx_frame.data.u8[3] & 0x08) >> 3);
      break;
    case 0x1DC:
      if (is_message_corrupt(rx_frame)) {
        datalayer_battery->status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
      battery_Charge_Power_Limit = (((rx_frame.data.u8[1] & 0x3F) << 4 | rx_frame.data.u8[2] >> 4) / 4.0);
      battery_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
      break;
    case 0x55B:
      if (is_message_corrupt(rx_frame)) {
        datalayer_battery->status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_TEMP = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
      if (battery_TEMP != 0x3ff) {  //3FF is unavailable value
        battery_SOC = battery_TEMP;
      }
      battery_Capacity_Empty = (bool)((rx_frame.data.u8[6] & 0x80) >> 7);
      break;
    case 0x5BC:
      battery_can_alive = true;
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN

      battery_MAX = ((rx_frame.data.u8[5] & 0x10) >> 4);
      if (battery_MAX) {
        battery_Max_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        //Max gids active, do nothing
        //Only the 30/40/62kWh packs have this mux
      } else {  //Normal current GIDS value is transmitted
        battery_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
        battery_Wh_Remaining = (battery_GIDS * WH_PER_GID);
      }

      if (LEAF_battery_Type == ZE0_BATTERY) {
        battery_AverageTemperature = (rx_frame.data.u8[3] - 40);  //In celcius, -40 to +55
      }

      battery_TEMP = (rx_frame.data.u8[4] >> 1);
      if (battery_TEMP != 0) {
        battery_StateOfHealth = (uint8_t)battery_TEMP;  //Collect state of health from battery
      }
      break;
    case 0x5C0:
      //This temperature only works for 2013-2017 AZE0 LEAF packs, the mux is different on other generations
      if (LEAF_battery_Type == AZE0_BATTERY) {
        if ((rx_frame.data.u8[0] >> 6) ==
            1) {  // Battery MAX temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          battery_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
        }
        if ((rx_frame.data.u8[0] >> 6) ==
            3) {  // Battery MIN temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
          battery_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
        }
      }

      battery_HeatExist = (rx_frame.data.u8[4] & 0x01);
      battery_Heating_Stop = ((rx_frame.data.u8[0] & 0x10) >> 4);
      battery_Heating_Start = ((rx_frame.data.u8[0] & 0x20) >> 5);
      battery_Batt_Heater_Mail_Send_Request = (rx_frame.data.u8[1] & 0x01);

      break;
    case 0x59E:
      //AZE0 2013-2017 or ZE1 2018-2023 battery detected
      //Only detect as AZE0 if not already set as ZE1
      if (LEAF_battery_Type != ZE1_BATTERY) {
        LEAF_battery_Type = AZE0_BATTERY;
      }
      break;
    case 0x380:
    case 0x5EB:
    case 0x5BF:
    case 0x1ED:
    case 0x1C2:
      //ZE1 2018-2023 battery detected!
      LEAF_battery_Type = ZE1_BATTERY;
      break;
    case 0x79B:
      stop_battery_query = true;             //Someone is trying to read data with Leafspy, stop our own polling!
      hold_off_with_polling_10seconds = 10;  //Polling is paused for 100s
      break;
    case 0x7BB:

      // Any traffic here means the LBC is mid-response. Recorded before any handling below, so a
      // pending DTC request holds off until the channel has been quiet for DTC_BUS_IDLE_MS.
      last_7bb_millis = millis();

      // Follow the ISO-TP framing of whatever answer is arriving, regardless of which service it
      // belongs to, so we know when the LBC has finished replying and is ready for a new request.
      if (uds_busy) {
        uint8_t rx_pci = rx_frame.data.u8[0] & 0xF0;
        if (rx_pci == 0x00) {  //Single frame: the whole answer, complete on arrival
          uds_busy = false;
        } else if (rx_pci == 0x10) {  //First frame: six payload bytes here, rest in the follow-ups
          uint16_t announced = ((rx_frame.data.u8[0] & 0x0F) << 8) | rx_frame.data.u8[1];
          uds_rx_remaining = (announced > 6) ? (announced - 6) : 0;
          if (uds_rx_remaining == 0) {
            uds_busy = false;
          }
        } else if (rx_pci == 0x20) {      //Consecutive frame: up to seven more payload bytes
          uds_request_millis = millis();  //Still answering, so restart the no-answer timer
          uds_rx_remaining = (uds_rx_remaining > 7) ? (uds_rx_remaining - 7) : 0;
          if (uds_rx_remaining == 0) {
            uds_busy = false;
          }
        }
      }

#ifndef SMALL_FLASH_DEVICE
      // This section checks if we are doing a SOH reset towards BMS. If we do, all 7BB handling is halted
      if (stateMachineClearSOH < 255) {
        //Intercept the messages based on state machine
        if (rx_frame.data.u8[0] == 0x06) {  // Incoming challenge data!
                                            // BMS should reply with (challenge) 06 67 65 (02 DD 86 43) FF
          incomingChallenge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[4] << 16) | (rx_frame.data.u8[5] << 8) |
                               rx_frame.data.u8[6]);
        }
        //Error checking
        if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x7F)) {
          challengeFailed = true;
        }
        break;
      }
#endif

      // ClearDiagnosticInformation is acknowledged with a single-frame 54. Only then are the stored
      // codes known to be gone. The read timestamp is reset too, so the page goes back to
      // "not read yet": an erase says nothing about what the LBC will report from here on.
      if (dtc_clear_in_progress && rx_frame.data.u8[0] == 0x01 && rx_frame.data.u8[1] == 0x54) {
        dtc_clear_in_progress = false;
        datalayer_battery->dtc.dtc_count = 0;
        datalayer_battery->dtc.dtc_reported_count = 0;
        datalayer_battery->dtc.dtc_read_failed = false;
        datalayer_battery->dtc.dtc_last_read_millis = 0;
        break;
      }

      // A DTC readout answers on 0x7BB just like the group polling below, and its first frame would
      // otherwise be mistaken for group 0x02 (cell voltages). Intercept it while a read is in
      // flight. The 0x59 service reply byte is what tells the two apart: a group reply carries 0x61.
      if (dtc_read_in_progress) {
        uint8_t pci = rx_frame.data.u8[0] & 0xF0;

        if (pci == 0x00 && rx_frame.data.u8[1] == 0x59) {  //Single frame: reply fits in one message
          dtc_rx_len = rx_frame.data.u8[0] & 0x0F;
          if (dtc_rx_len > 7) {
            dtc_rx_len = 7;
          }
          dtc_rx_total = dtc_rx_len;  //A single frame is the whole answer
          for (uint8_t i = 0; i < dtc_rx_len; i++) {
            dtc_buffer[i] = rx_frame.data.u8[1 + i];
          }
          parseDTCResponse();
          break;
        }

        if (pci == 0x10 && rx_frame.data.u8[2] == 0x59) {  //First frame of a multi-frame reply
          dtc_rx_total = ((rx_frame.data.u8[0] & 0x0F) << 8) | rx_frame.data.u8[1];
          dtc_rx_seen = 0;
          dtc_rx_len = 0;
          dtc_rx_active = true;
          for (uint8_t i = 2; i < 8 && dtc_rx_seen < dtc_rx_total; i++) {
            if (dtc_rx_len < DTC_BUFFER_SIZE) {
              dtc_buffer[dtc_rx_len++] = rx_frame.data.u8[i];
            }
            dtc_rx_seen++;
          }
          if (dtc_rx_seen >= dtc_rx_total) {
            parseDTCResponse();
          } else {
            transmit_can_frame(&LEAF_NEXT_LINE_REQUEST);  //Flow control, ask for the rest
          }
          break;
        }

        if (dtc_rx_active && pci == 0x20) {  //Consecutive frame
          // Keep acknowledging frames right to the end even once the buffer is full. Falling silent
          // mid-transfer leaves the LBC waiting on a flow control that never comes, and it will not
          // take another request until that has timed out on its side.
          for (uint8_t i = 1; i < 8 && dtc_rx_seen < dtc_rx_total; i++) {
            if (dtc_rx_len < DTC_BUFFER_SIZE) {
              dtc_buffer[dtc_rx_len++] = rx_frame.data.u8[i];
            }
            dtc_rx_seen++;
          }
          dtc_request_millis = millis();  //Progress, so the readout timeout measures silence
          if (dtc_rx_seen >= dtc_rx_total) {
            parseDTCResponse();
          } else {
            transmit_can_frame(&LEAF_NEXT_LINE_REQUEST);
          }
          break;
        }

        if (rx_frame.data.u8[1] == 0x7F && rx_frame.data.u8[2] == 0x19) {  //Request rejected by LBC
          dtc_read_in_progress = false;
          dtc_rx_active = false;
          datalayer_battery->dtc.dtc_read_failed = true;
          datalayer_battery->dtc.dtc_last_read_millis = millis();
          break;
        }
      }

      if (stop_battery_query) {  //Leafspy is active, stop our own polling
        break;
      }

      //First check which group data we are getting
      if (rx_frame.data.u8[0] == 0x10) {  //First message of a group
        group_7bb = rx_frame.data.u8[3];
        //Remember how long the reply is. The group 1 layout differs between LEAF generations, and
        //the announced length is what identifies which one the LBC just sent.
        group_7bb_length = rx_frame.data.u8[1];
      }

      transmit_can_frame(&LEAF_NEXT_LINE_REQUEST);  //Request the next frame for the group

      if (group_7bb == 0x01)  //High precision SOC, Current, voltages etc.
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame
          //High precision Battery_current_1 resides here, but has been deemed unusable by 62kWh owners
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame
          //High precision Battery_current_2 resides here, but has been deemed unusable by 62kWh owners
        }

        if (rx_frame.data.u8[0] == 0x23) {  // Fourth frame
          battery_insulation = (uint16_t)((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
          if (battery_insulation > 0) {
            datalayer_battery->status.insulation_resistance_kOhm = battery_insulation;
            datalayer_battery->status.insulation_resistance_available = true;
          }
        }

        if (rx_frame.data.u8[0] == 0x24) {  // Fifth frame
          // Hx sits at a different payload offset and uses a different scale depending on which
          // layout the LBC answered with, so the reply length decides how to read it:
          //   0x29 (ZE0 24kWh) / 0x2B (AZE0 30kWh) -> payload[26..27], already in hundredths of a %
          //   0x35 (ZE1 40/62kWh)                  -> payload[28..29], raw / 102.4 = percent
          // This frame carries payload[25..31] in u8[1..7]. Any other length is a layout we do not
          // know (a ZE1 answers 0x2C shortly after wakeup), so leave the last good value in place.
          if (group_7bb_length == 0x35) {  //ZE1
            uint16_t battery_HX_raw = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
            //raw / 102.4 * 100 == raw * 125 / 128, rounded to nearest
            battery_HX_pptt = (uint16_t)(((uint32_t)battery_HX_raw * 125u + 64u) / 128u);
          } else if (group_7bb_length == 0x29 || group_7bb_length == 0x2B) {  //ZE0 / AZE0
            battery_HX_pptt = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
          }
        }
      }

      if (group_7bb == 0x02)  //Cell Voltages
      {
        if (rx_frame.data.u8[0] == 0x10) {  //first frame is anomalous
          battery_request_idx = 0;
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          break;
        }
        if (rx_frame.data.u8[6] == 0xFF && rx_frame.data.u8[0] == 0x2C) {  //Last frame
          //Last frame does not contain any cell data, calculate the result

          //Map all cell voltages to the global array
          memcpy(datalayer_battery->status.cell_voltages_mV, battery_cell_voltages, 96 * sizeof(uint16_t));

          //calculate min/max voltages
          battery_min_max_voltage[0] = 9999;
          battery_min_max_voltage[1] = 0;
          for (battery_cellcounter = 0; battery_cellcounter < 96; battery_cellcounter++) {
            if (battery_min_max_voltage[0] > battery_cell_voltages[battery_cellcounter])
              battery_min_max_voltage[0] = battery_cell_voltages[battery_cellcounter];
            if (battery_min_max_voltage[1] < battery_cell_voltages[battery_cellcounter])
              battery_min_max_voltage[1] = battery_cell_voltages[battery_cellcounter];
          }

          datalayer_battery->status.cell_max_voltage_mV = battery_min_max_voltage[1];
          datalayer_battery->status.cell_min_voltage_mV = battery_min_max_voltage[0];

          break;
        }

        if ((rx_frame.data.u8[0] % 2) == 0) {  //even frames
          battery_cell_voltages[battery_request_idx++] |= rx_frame.data.u8[1];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        } else {  //odd frames
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          battery_cell_voltages[battery_request_idx++] = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
          battery_cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
        }
      }

      if (group_7bb == 0x04) {              //Temperatures
        if (rx_frame.data.u8[0] == 0x10) {  //First message
          battery_temp_raw_1 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          battery_temp_raw_2_highnibble = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second message
          battery_temp_raw_2 = (battery_temp_raw_2_highnibble << 8) | rx_frame.data.u8[1];
          battery_temp_raw_3 = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          battery_temp_raw_4 = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third message
          //All values read, let's figure out the min/max!

          if (battery_temp_raw_3 == 65535) {  //We are on a 2013+ pack that only has three temp sensors.
            //Start with finding max value
            battery_temp_raw_max = battery_temp_raw_1;
            if (battery_temp_raw_2 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_2;
            }
            if (battery_temp_raw_4 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_4;
            }
            //Then find min
            battery_temp_raw_min = battery_temp_raw_1;
            if (battery_temp_raw_2 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_2;
            }
            if (battery_temp_raw_4 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_4;
            }
          } else {  //All 4 temp sensors available on 2011-2012
            //Start with finding max value
            battery_temp_raw_max = battery_temp_raw_1;
            if (battery_temp_raw_2 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_2;
            }
            if (battery_temp_raw_3 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_3;
            }
            if (battery_temp_raw_4 > battery_temp_raw_max) {
              battery_temp_raw_max = battery_temp_raw_4;
            }
            //Then find min
            battery_temp_raw_min = battery_temp_raw_1;
            if (battery_temp_raw_2 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_2;
            }
            if (battery_temp_raw_3 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_2;
            }
            if (battery_temp_raw_4 < battery_temp_raw_min) {
              battery_temp_raw_min = battery_temp_raw_4;
            }
          }
        }
      }

      if (group_7bb == 0x06)  //Balancing resistor status
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (10 1A 61 06 [14 55 55 51])
          balancing_frames_seen = 0x01;     //Start of a new response
          for (int i = 0; i < 8; i++) {
            // Byte 4 - 7 (bits 0-31)
            for (int byte_i = 0; byte_i < 4; byte_i++) {
              battery_balancing_shunts[byte_i * 8 + i] = (rx_frame.data.u8[4 + byte_i] & (1 << i)) >> i;
            }
          }
        }
        if (rx_frame.data.u8[0] == 0x21) {  // Second frame (21 [50 55 41 2B 56 54 15])
          balancing_frames_seen |= 0x02;
          for (int i = 0; i < 8; i++) {
            // Byte 1 to 7 (bits 32-87)
            for (int byte_i = 0; byte_i < 7; byte_i++) {
              battery_balancing_shunts[32 + byte_i * 8 + i] = (rx_frame.data.u8[1 + byte_i] & (1 << i)) >> i;
            }
          }
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third frame (22 51 FF FF FF FF FF FF)
          balancing_frames_seen |= 0x04;
          for (int i = 0; i < 8; i++) {
            // Byte 1 (bits 88-95)
            battery_balancing_shunts[88 + i] = (rx_frame.data.u8[1] & (1 << i)) >> i;
          }
          //Only publish once all three frames of this response arrived. A dropped frame would otherwise
          //leave part of the array holding the previous response, which reads as a spurious change.
          if (balancing_frames_seen == 0x07) {
            memcpy(datalayer_battery->status.cell_balancing_status, battery_balancing_shunts, 96 * sizeof(bool));
            balancing_data_fresh = true;
          }
          balancing_frames_seen = 0;
        }

        if (rx_frame.data.u8[0] == 0x23) {  //Fourth frame (23 FF FF FF FF FF FF FF)
        }
      }

      if (group_7bb == 0x62) {              //Lifetime charge counters
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (10 76 61 62 08 00 01 5A)
          //Both counters are carried in the first frame, no need to walk the rest of the reply:
          //payload[0..1] holds the L1/L2 (AC) charges, payload[2..3] the quick (CHAdeMO) charges.
          //A counter the LBC has no value for reads back as 0xFFFF. A used pack always has AC
          //charges, so a zero L1/L2 count means "not read yet" and keeps the group in the rotation.
          uint16_t count_l1l2 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          uint16_t count_qc = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          if (count_l1l2 != 0xFFFF && count_qc != 0xFFFF) {
            battery_charge_count_l1l2 = count_l1l2;
            battery_charge_count_qc = count_qc;
          }
        }
      }

      if (group_7bb == 0x83)  //BatteryPartNumber
      {
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (101A6183334E4B32)
          BatteryPartNumber[0] = rx_frame.data.u8[4];
          BatteryPartNumber[1] = rx_frame.data.u8[5];
          BatteryPartNumber[2] = rx_frame.data.u8[6];
          BatteryPartNumber[3] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame (2141524205170000)
          BatteryPartNumber[4] = rx_frame.data.u8[1];
          BatteryPartNumber[5] = rx_frame.data.u8[2];
          BatteryPartNumber[6] = rx_frame.data.u8[3];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third frame (2200000002101311)
        }

        if (rx_frame.data.u8[0] == 0x23) {  //Fourth frame (23000000000080FF)
        }
      }
      if (group_7bb == 0x84) {              //BatterySerialNumber
        if (rx_frame.data.u8[0] == 0x10) {  //First frame (10 16 61 84 32 33 30 55)
          BatterySerialNumber[0] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x21) {  //Second frame (21 4B 31 31 39 32 45 30)
          BatterySerialNumber[1] = rx_frame.data.u8[1];
          BatterySerialNumber[2] = rx_frame.data.u8[2];
          BatterySerialNumber[3] = rx_frame.data.u8[3];
          BatterySerialNumber[4] = rx_frame.data.u8[4];
          BatterySerialNumber[5] = rx_frame.data.u8[5];
          BatterySerialNumber[6] = rx_frame.data.u8[6];
          BatterySerialNumber[7] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x22) {  //Third frame (22 30 31 34 38 32 20 A0)
          BatterySerialNumber[8] = rx_frame.data.u8[1];
          BatterySerialNumber[9] = rx_frame.data.u8[2];
          BatterySerialNumber[10] = rx_frame.data.u8[3];
          BatterySerialNumber[11] = rx_frame.data.u8[4];
          BatterySerialNumber[12] = rx_frame.data.u8[5];
          BatterySerialNumber[13] = rx_frame.data.u8[6];
          BatterySerialNumber[14] = rx_frame.data.u8[7];
        }
        if (rx_frame.data.u8[0] == 0x23) {  //Fourth frame (23 00 00 00 00 00 00 00)
        }
      }

      break;
    default:
      break;
  }
}

// Parses a reassembled UDS ReadDTCInformation reply out of dtc_buffer: a 3-byte header
// (59 02 <statusAvailabilityMask>) followed by 4 bytes per DTC, being a 3-byte code plus one status
// byte. Only the raw codes are stored here; the web renderer formats them into the 5-character
// Nissan strings (P33D7, U1000) and looks their descriptions up in nissan_leaf_dtc.json.
void NissanLeafBattery::parseDTCResponse() {
  const uint16_t DTC_HEADER_LEN = 3;

  dtc_read_in_progress = false;
  dtc_rx_active = false;
  datalayer_battery->dtc.dtc_last_read_millis = millis();

  if (dtc_rx_len < DTC_HEADER_LEN || dtc_buffer[0] != 0x59 || dtc_buffer[1] != 0x02) {
    datalayer_battery->dtc.dtc_read_failed = true;
    return;
  }

  // What the battery actually reported, which can be more than we have slots for.
  uint16_t reported = (dtc_rx_total > DTC_HEADER_LEN) ? ((dtc_rx_total - DTC_HEADER_LEN) / 4) : 0;

  uint16_t count = (dtc_rx_len - DTC_HEADER_LEN) / 4;
  if (count > DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT) {
    count = DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT;
  }

  for (uint16_t i = 0; i < count; i++) {
    uint16_t offset = DTC_HEADER_LEN + (i * 4);
    datalayer_battery->dtc.dtc_codes[i] = ((uint32_t)dtc_buffer[offset] << 16) |
                                          ((uint32_t)dtc_buffer[offset + 1] << 8) | (uint32_t)dtc_buffer[offset + 2];
    datalayer_battery->dtc.dtc_status[i] = dtc_buffer[offset + 3];
  }

  datalayer_battery->dtc.dtc_count = count;
  datalayer_battery->dtc.dtc_reported_count = reported;
  datalayer_battery->dtc.dtc_read_failed = false;
}

// Sends any pending DTC request and times out the ones that get no answer. Kept out of
// update_values() so the transmission can be held back until the diagnostic channel is free: the
// LBC drops a request that lands while it is still sending a group poll response, which loses the
// readout silently.
void NissanLeafBattery::handle_DTC_requests(unsigned long currentMillis) {
  // Stop waiting on an answer the LBC is never going to send, otherwise one lost reply would block
  // the channel for good.
  if (uds_busy && (currentMillis - uds_request_millis > UDS_RESPONSE_TIMEOUT_MS)) {
    uds_busy = false;
  }

  // Free to transmit only when our own previous request has been fully answered (uds_busy) and
  // nothing else is talking on the channel either, which is what catches a third party such as
  // LeafSpy polling the same LBC.
  bool channel_idle = !uds_busy && (currentMillis - last_7bb_millis) > DTC_BUS_IDLE_MS;
  // The SOH clear runs its own multi-step exchange over the same request/response pair, so a DTC
  // request must not be slipped in between its steps either.
  bool soh_clear_running = stateMachineClearSOH < 255;
  bool busy = dtc_read_in_progress || dtc_clear_in_progress || soh_clear_running;

  if (UserRequestDTCreadout && !busy && channel_idle) {
    UserRequestDTCreadout = false;
    dtc_read_in_progress = true;
    dtc_rx_active = false;
    dtc_rx_len = 0;
    dtc_rx_total = 0;
    dtc_rx_seen = 0;
    dtc_request_millis = currentMillis;
    datalayer_battery->dtc.dtc_read_failed = false;
    uds_busy = true;
    uds_request_millis = currentMillis;
    transmit_can_frame(&LEAF_READ_DTC);
    return;
  }

  if (UserRequestDTCreset && !busy && channel_idle) {
    UserRequestDTCreset = false;
    dtc_clear_in_progress = true;
    dtc_clear_millis = currentMillis;
    uds_busy = true;
    uds_request_millis = currentMillis;
    transmit_can_frame(&LEAF_CLEAR_DTC);
    return;
  }

  // A readout that goes unanswered is retried before being reported as failed. A request lost to a
  // busy channel is the expected cause, and by the time the timeout expires that channel is free.
  if (dtc_read_in_progress && (currentMillis - dtc_request_millis > DTC_TIMEOUT_MS)) {
    dtc_read_in_progress = false;
    dtc_rx_active = false;
    if (dtc_read_retries < DTC_MAX_RETRIES) {
      dtc_read_retries++;
      UserRequestDTCreadout = true;
    } else {
      datalayer_battery->dtc.dtc_read_failed = true;
      datalayer_battery->dtc.dtc_last_read_millis = currentMillis;
    }
  }

  // Give up waiting for the erase acknowledgement. The previously read list is deliberately left
  // untouched here: an unconfirmed erase is not evidence that the codes are gone.
  if (dtc_clear_in_progress && (currentMillis - dtc_clear_millis > DTC_TIMEOUT_MS)) {
    dtc_clear_in_progress = false;
  }
}

void NissanLeafBattery::transmit_can(unsigned long currentMillis) {

  handle_DTC_requests(currentMillis);

  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis10 = currentMillis;
    previousMillis100 = currentMillis;
    previousMillis10s = currentMillis;
    return;
  }

  if (battery_can_alive) {

    //Send 10ms message
    if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
      previousMillis10 = currentMillis;

      switch (mprun10) {
        case 0:
          LEAF_1D4.data.u8[4] = 0x07;
          LEAF_1D4.data.u8[7] = 0x12;
          break;
        case 1:
          LEAF_1D4.data.u8[4] = 0x47;
          LEAF_1D4.data.u8[7] = 0xD5;
          break;
        case 2:
          LEAF_1D4.data.u8[4] = 0x87;
          LEAF_1D4.data.u8[7] = 0x19;
          break;
        case 3:
          LEAF_1D4.data.u8[4] = 0xC7;
          LEAF_1D4.data.u8[7] = 0xDE;
          break;
      }
      //Only send this message when NISSANLEAF_CHARGER is not defined (otherwise it will collide!)
      //TODO, this breaks double/triple battery setups when using PDM for charging
      if (!charger || charger->type() != ChargerType::NissanLeaf) {
        transmit_can_frame(&LEAF_1D4);
      }

      switch (mprun10r) {
        case (0):
          LEAF_1F2.data.u8[3] = 0xB0;
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x8F;
          break;
        case (1):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x80;
          break;
        case (2):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x81;
          break;
        case (3):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x82;
          break;
        case (4):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x8F;
          break;
        case (5):  // Set 2
          LEAF_1F2.data.u8[3] = 0xB4;
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x84;
          break;
        case (6):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x85;
          break;
        case (7):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x86;
          break;
        case (8):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x83;
          break;
        case (9):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x84;
          break;
        case (10):  // Set 3
          LEAF_1F2.data.u8[3] = 0xB0;
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x81;
          break;
        case (11):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x82;
          break;
        case (12):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x8F;
          break;
        case (13):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x80;
          break;
        case (14):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x81;
          break;
        case (15):  // Set 4
          LEAF_1F2.data.u8[3] = 0xB4;
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x86;
          break;
        case (16):
          LEAF_1F2.data.u8[6] = 0x00;
          LEAF_1F2.data.u8[7] = 0x83;
          break;
        case (17):
          LEAF_1F2.data.u8[6] = 0x01;
          LEAF_1F2.data.u8[7] = 0x84;
          break;
        case (18):
          LEAF_1F2.data.u8[6] = 0x02;
          LEAF_1F2.data.u8[7] = 0x85;
          break;
        case (19):
          LEAF_1F2.data.u8[6] = 0x03;
          LEAF_1F2.data.u8[7] = 0x86;
          break;
        default:
          break;
      }

      //Only send this message when NISSANLEAF_CHARGER is not defined (otherwise it will collide!)
      //TODO, this breaks double/triple battery setups when using PDM for charging
      if (!charger || charger->type() != ChargerType::NissanLeaf) {
        transmit_can_frame(&LEAF_1F2);
      }

      mprun10r = (mprun10r + 1) % 20;  // 0x1F2 patter repeats after 20 messages. 0-1..19-0

      mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
    }

    //Send 40ms message
    if (currentMillis - previousMillis40 >= INTERVAL_40_MS) {
      previousMillis40 = currentMillis;
      if (LEAF_battery_Type == ZE1_BATTERY) {
        transmit_can_frame(&LEAF_355);
      }
    }

    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      if (stateMachineClearSOH < 255) {  // Enter the ClearSOH statemachine only if we request it
        clearSOH();
      }

      //When battery requests heating pack status change, ack this
      if (battery_Batt_Heater_Mail_Send_Request) {
        LEAF_50B.data.u8[6] = 0x20;  //Batt_Heater_Mail_Send_OK
      } else {
        LEAF_50B.data.u8[6] = 0x00;  //Batt_Heater_Mail_Send_NG
      }

      //If we are on ZE1 battery, handle some extra 100ms messages
      if (LEAF_battery_Type == ZE1_BATTERY) {
        counter_3B8 = (counter_3B8 + 1) % 15;
        LEAF_3B8.data.u8[2] = counter_3B8;  // 0 - 14 (0x00 - 0x0E)
        transmit_can_frame(&LEAF_3B8);      // Sending 3B8 removes U1000 and P318E DTC
        transmit_can_frame(&LEAF_5C5);      // Sending 5C5 removes U214E DTC
        transmit_can_frame(&LEAF_626);      // Sending 625 removes U215B DTC
        if (flip_3B8) {
          flip_3B8 = 0;
          LEAF_3B8.data.u8[1] = 0xC8;
        } else {
          flip_3B8 = 1;
          LEAF_3B8.data.u8[1] = 0xE8;
        }
      }

      // VCM message, containing info if battery should sleep or stay awake
      transmit_can_frame(&LEAF_50B);  // HCM_WakeUpSleepCommand == 11b == WakeUp, and CANMASK = 1

      LEAF_50C.data.u8[3] = mprun100;
      switch (mprun100) {
        case 0:
          LEAF_50C.data.u8[4] = 0x5D;
          LEAF_50C.data.u8[5] = 0xC8;
          break;
        case 1:
          LEAF_50C.data.u8[4] = 0xB2;
          LEAF_50C.data.u8[5] = 0x31;
          break;
        case 2:
          LEAF_50C.data.u8[4] = 0x5D;
          LEAF_50C.data.u8[5] = 0x63;
          break;
        case 3:
          LEAF_50C.data.u8[4] = 0xB2;
          LEAF_50C.data.u8[5] = 0x9A;
          break;
      }
      transmit_can_frame(&LEAF_50C);

      mprun100 = (mprun100 + 1) % 4;  // mprun100 cycles between 0-1-2-3-0-1...
    }

    // Send 500ms CAN Message
    if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
      previousMillis500 = currentMillis;
      if (LEAF_battery_Type == ZE1_BATTERY) {
        transmit_can_frame(&LEAF_5EC);
      }
    }

    //Send 10s CAN messages
    //The first pass through the group list runs at the faster burst interval, so the battery info
    //page is populated within seconds of startup rather than over the following minute.
    if (currentMillis - previousMillis10s >=
        (poll_burst_remaining ? POLL_BURST_INTERVAL_MS : (unsigned long)INTERVAL_10_S)) {
      previousMillis10s = currentMillis;

      //Every 10s, ask diagnostic data from the battery. Don't ask if someone is already polling on the bus (Leafspy?),
      //and don't start a group transfer while a DTC operation is pending or in flight: the two share
      //the 0x79B/0x7BB channel, and whichever request lands second gets dropped by the LBC.
      bool dtc_operation_pending =
          UserRequestDTCreadout || UserRequestDTCreset || dtc_read_in_progress || dtc_clear_in_progress;
      if (!stop_battery_query && !dtc_operation_pending) {

        // Move to the next group, skipping the static ones that already answered. The charge
        // counters and the two identity strings cannot change while the pack is powered, so each
        // is asked for only until its data is in, after which the recurring groups come round
        // faster. Testing the data itself rather than a "seen" flag means a reply that arrived
        // while another tool was polling the bus counts just as well.
        do {
          PIDindex = (PIDindex + 1) % (sizeof(PIDgroups) / sizeof(PIDgroups[0]));
        } while ((PIDgroups[PIDindex] == 0x62 && battery_charge_count_l1l2 != 0) ||
                 (PIDgroups[PIDindex] == 0x84 && BatterySerialNumber[0] != 0) ||
                 (PIDgroups[PIDindex] == 0x83 && BatteryPartNumber[0] != 0));
        LEAF_GROUP_REQUEST.data.u8[2] = PIDgroups[PIDindex];

        if (poll_burst_remaining) {
          poll_burst_remaining--;
        }
        uds_busy = true;
        uds_request_millis = currentMillis;
        transmit_can_frame(&LEAF_GROUP_REQUEST);
      }

      if (hold_off_with_polling_10seconds > 0) {
        hold_off_with_polling_10seconds--;
      } else {
        stop_battery_query = false;
      }
    }
  }
}

uint8_t NissanLeafBattery::calculate_crc(CAN_frame& rx_frame) {
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable_nissan_leaf[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

bool NissanLeafBattery::is_message_corrupt(CAN_frame rx_frame) {
  uint8_t crc = calculate_crc(rx_frame);
  return crc != rx_frame.data.u8[7];
}

uint16_t Temp_fromRAW_to_F(uint16_t temperature) {  //This function feels horrible, but apparently works well
  if (temperature == 1021) {
    return 10;
  } else if (temperature == 65535) {  //Value unavailable, sensor does not exist
    return 718;                       //0*C final calculation
  } else if (temperature >= 589) {
    return static_cast<uint16_t>(1620 - temperature * 1.81);
  } else if (temperature >= 569) {
    return static_cast<uint16_t>(572 + (579 - temperature) * 1.80);
  } else if (temperature >= 558) {
    return static_cast<uint16_t>(608 + (558 - temperature) * 1.6363636363636364);
  } else if (temperature >= 548) {
    return static_cast<uint16_t>(626 + (548 - temperature) * 1.80);
  } else if (temperature >= 537) {
    return static_cast<uint16_t>(644 + (537 - temperature) * 1.6363636363636364);
  } else if (temperature >= 447) {
    return static_cast<uint16_t>(662 + (527 - temperature) * 1.8);
  } else if (temperature >= 438) {
    return static_cast<uint16_t>(824 + (438 - temperature) * 2);
  } else if (temperature >= 428) {
    return static_cast<uint16_t>(842 + (428 - temperature) * 1.80);
  } else if (temperature >= 365) {
    return static_cast<uint16_t>(860 + (419 - temperature) * 2.0);
  } else if (temperature >= 357) {
    return static_cast<uint16_t>(986 + (357 - temperature) * 2.25);
  } else if (temperature >= 348) {
    return static_cast<uint16_t>(1004 + (348 - temperature) * 2);
  } else if (temperature >= 316) {
    return static_cast<uint16_t>(1022 + (340 - temperature) * 2.25);
  }
  return static_cast<uint16_t>(1094 + (309 - temperature) * 2.5714285714285715);
}

void NissanLeafBattery::clearSOH(void) {
#ifndef SMALL_FLASH_DEVICE
  stop_battery_query = true;
  hold_off_with_polling_10seconds = 10;  // Active battery polling is paused for 100 seconds

  switch (stateMachineClearSOH) {
    case 0:  // Wait until polling actually stops
      challengeFailed = false;
      stateMachineClearSOH = 1;
      break;
    case 1:  // Set CAN_PROCESS_FLAG to 0xC0
      LEAF_CLEAR_SOH.data = {0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      // BMS should reply 02 50 C0 FF FF FF FF FF
      stateMachineClearSOH = 2;
      break;
    case 2:  // Set something ?
      LEAF_CLEAR_SOH.data = {0x02, 0x3E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      // BMS should reply 7E FF FF FF FF FF FF
      stateMachineClearSOH = 3;
      break;
    case 3:  // Request challenge to solve
      LEAF_CLEAR_SOH.data = {0x02, 0x27, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      // BMS should reply with (challenge) 06 67 65 (02 DD 86 43) FF
      stateMachineClearSOH = 4;
      break;
    case 4:  // Send back decoded challenge data
      decodeChallengeData(incomingChallenge, solvedChallenge);
      LEAF_CLEAR_SOH.data = {
          0x10, 0x0A, 0x27, 0x66, solvedChallenge[0], solvedChallenge[1], solvedChallenge[2], solvedChallenge[3]};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      // BMS should reply 7BB 8 30 01 00 FF FF FF FF FF // Proceed with more data (PID ACK)
      stateMachineClearSOH = 5;
      break;
    case 5:  // Reply with even more decoded challenge data
      LEAF_CLEAR_SOH.data = {
          0x21, solvedChallenge[4], solvedChallenge[5], solvedChallenge[6], solvedChallenge[7], 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      // BMS should reply 02 67 66 FF FF FF FF FF // Thank you for the data
      stateMachineClearSOH = 6;
      break;
    case 6:  // Check if solved data was OK
      LEAF_CLEAR_SOH.data = {0x03, 0x31, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      //7BB 8 03 71 03 01 FF FF FF FF // If all is well, BMS replies with 03 71 03 01.
      //Incase you sent wrong challenge, you get 03 7f 31 12
      stateMachineClearSOH = 7;
      break;
    case 7:  // Reset SOH% request
      LEAF_CLEAR_SOH.data = {0x03, 0x31, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      //7BB 8 03 71 03 02 FF FF FF FF // 03 71 03 02 means that BMS accepted command.
      //7BB 03 7f 31 12 means your challenge was wrong, so command ignored
      stateMachineClearSOH = 8;
      break;
    case 8:  // Please proceed with resetting SOH
      LEAF_CLEAR_SOH.data = {0x02, 0x10, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00};
      transmit_can_frame(&LEAF_CLEAR_SOH);
      // 7BB 8 02 50 81 FF FF FF FF FF // SOH reset OK
      stateMachineClearSOH = 255;
      break;
    default:
      break;
  }
#endif
}

#ifndef SMALL_FLASH_DEVICE

unsigned int CyclicXorHash16Bit(unsigned int param_1, unsigned int param_2) {
  bool bVar1;
  unsigned int uVar2, uVar3, uVar4, uVar5, uVar6, uVar7, uVar8, uVar9, uVar10, uVar11, iVar12;

  param_1 = param_1 & 0xffff;
  param_2 = param_2 & 0xffff;
  uVar10 = 0xffff;
  iVar12 = 2;
  do {
    uVar2 = param_2;
    if ((param_1 & 1) == 1) {
      uVar2 = param_1 >> 1;
    }
    uVar3 = param_2;
    if ((param_1 >> 1 & 1) == 1) {
      uVar3 = param_1 >> 2;
    }
    uVar4 = param_2;
    if ((param_1 >> 2 & 1) == 1) {
      uVar4 = param_1 >> 3;
    }
    uVar5 = param_2;
    if ((param_1 >> 3 & 1) == 1) {
      uVar5 = param_1 >> 4;
    }
    uVar6 = param_2;
    if ((param_1 >> 4 & 1) == 1) {
      uVar6 = param_1 >> 5;
    }
    uVar7 = param_2;
    if ((param_1 >> 5 & 1) == 1) {
      uVar7 = param_1 >> 6;
    }
    uVar11 = param_1 >> 7;
    uVar8 = param_2;
    if ((param_1 >> 6 & 1) == 1) {
      uVar8 = uVar11;
    }
    param_1 = param_1 >> 8;
    uVar9 = param_2;
    if ((uVar11 & 1) == 1) {
      uVar9 = param_1;
    }
    uVar10 =
        (((((((((((((((uVar10 & 0x7fff) << 1 ^ uVar2) & 0x7fff) << 1 ^ uVar3) & 0x7fff) << 1 ^ uVar4) & 0x7fff) << 1 ^
                uVar5) &
               0x7fff)
                  << 1 ^
              uVar6) &
             0x7fff)
                << 1 ^
            uVar7) &
           0x7fff)
              << 1 ^
          uVar8) &
         0x7fff)
            << 1 ^
        uVar9;
    bVar1 = iVar12 != 1;
    iVar12 = iVar12 + -1;
  } while (bVar1);
  return uVar10;
}
unsigned int ComputeMaskedXorProduct(unsigned int param_1, unsigned int param_2, unsigned int param_3) {
  return ((param_3 ^ 0x7F88) | (param_2 ^ 0x8FE7)) * ((((param_1 & 0xffff) >> 8) ^ (param_1 & 0xff))) & 0xffff;
}

short ShortMaskedSumAndProduct(short param_1, short param_2) {
  unsigned short uVar1;

  uVar1 = (param_2 + (param_1 * 0x0006)) & 0xff;
  return (uVar1 + param_1) * (uVar1 + param_2);
}

unsigned int MaskedBitwiseRotateMultiply(unsigned int param_1, unsigned int param_2) {
  unsigned int uVar1;

  param_1 = param_1 & 0xffff;
  param_2 = param_2 & 0xffff;
  uVar1 = param_2 & (param_1 | 0x0006) & 0xf;
  return ((unsigned int)param_1 >> uVar1 | param_1 << (0x10 - (uVar1 & 0x1f))) *
             (param_2 << uVar1 | (unsigned int)param_2 >> (0x10 - (uVar1 & 0x1f))) &
         0xffff;
}

unsigned int CryptAlgo(unsigned int param_1, unsigned int param_2, unsigned int param_3) {
  unsigned int uVar1, uVar2, iVar3, iVar4;

  uVar1 = MaskedBitwiseRotateMultiply(param_2, param_3);
  uVar2 = ShortMaskedSumAndProduct(param_2, param_3);
  uVar1 = ComputeMaskedXorProduct(param_1, uVar1, uVar2);
  uVar2 = ComputeMaskedXorProduct(param_1, uVar2, uVar1);
  iVar3 = CyclicXorHash16Bit(uVar1, 0x8421);
  iVar4 = CyclicXorHash16Bit(uVar2, 0x8421);
  return iVar4 + iVar3 * 0x10000;
}

void decodeChallengeData(unsigned int incomingChallenge, unsigned char* solvedChallenge) {
  unsigned int uVar1, uVar2;

  uVar1 = CryptAlgo(0x54e9, 0x3afd, incomingChallenge >> 0x10);
  uVar2 = CryptAlgo(incomingChallenge & 0xffff, incomingChallenge >> 0x10, 0x54e9);
  *solvedChallenge = (unsigned char)uVar1;
  solvedChallenge[1] = (unsigned char)uVar2;
  solvedChallenge[2] = (unsigned char)((unsigned int)uVar2 >> 8);
  solvedChallenge[3] = (unsigned char)((unsigned int)uVar1 >> 8);
  solvedChallenge[4] = (unsigned char)((unsigned int)uVar2 >> 0x10);
  solvedChallenge[5] = (unsigned char)((unsigned int)uVar1 >> 0x10);
  solvedChallenge[6] = (unsigned char)((unsigned int)uVar2 >> 0x18);
  solvedChallenge[7] = (unsigned char)((unsigned int)uVar1 >> 0x18);
  return;
}

#endif

void NissanLeafBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
