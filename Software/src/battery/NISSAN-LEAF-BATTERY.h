#ifndef NISSAN_LEAF_BATTERY_H
#define NISSAN_LEAF_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "NISSAN-LEAF-HTML.h"

extern bool user_selected_LEAF_interlock_mandatory;
//CHG_STA_RQ value transmitted in 0x1F2 as the BMS starts up: 0 = 00b (no request), 1 = 01b
//(normal charge), 2 = 10b (quick charge). Only ever holds one of those three.
extern uint8_t user_selected_LEAF_chg_sta_rq;

class NissanLeafBattery : public CanBattery {
 public:
  // Use the default constructor to create the first or single battery.battery_Total_Voltage2
  NissanLeafBattery() : renderer(&datalayer.battery, &datalayer_extended.nissanleaf) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_nissan = &datalayer_extended.nissanleaf;
  }
  // Use this constructor for the second battery.
  NissanLeafBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_NISSAN_LEAF* extended,
                    CAN_Interface targetCan)
      : CanBattery(targetCan), renderer(datalayer_ptr, extended) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    datalayer_nissan = extended;

    battery_Total_Voltage2 = 0;  //Zero out pack voltage to avoid contactor closing before we know value via CAN
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  bool supports_reset_SOH();
  void reset_SOH() { UserRequestSOHreset = true; }
  bool supports_reset_DTC() { return true; }
  void reset_DTC() { UserRequestDTCreset = true; }
  bool supports_read_DTC() { return true; }
  void read_DTC() {
    UserRequestDTCreadout = true;
    dtc_read_retries = 0;
  }
  bool supports_insulation_resistance() { return true; }

  bool soc_plausible() {
    // When pack voltage is close to max, and SOC% is still low (<65.0%), SOC is not plausible
    return !((datalayer.battery.status.voltage_dV > (datalayer.battery.info.max_design_voltage_dV - 100)) &&
             (battery_SOC < 650));
  }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }
  static constexpr const char* Name = "Nissan LEAF battery";

  uint8_t calculate_crc(CAN_frame& frame);

 private:
  bool UserRequestDTCreset = false;
  bool UserRequestDTCreadout = false;
  bool UserRequestSOHreset = false;

  // Parses a fully reassembled UDS ReadDTCInformation reply out of dtc_buffer into
  // datalayer_battery->dtc.
  void parseDTCResponse();

  // Sends pending DTC requests once the diagnostic channel is idle, and times out unanswered ones.
  void handle_DTC_requests(unsigned long currentMillis);
  static const int MAX_PACK_VOLTAGE_DV = 4055;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2400;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4224;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2500;  //Battery is put into emergency stop if one cell goes below this value

  NissanLeafHtmlRenderer renderer;

  bool is_message_corrupt(CAN_frame rx_frame);
  void clearSOH(void);

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_NISSAN_LEAF* datalayer_nissan;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis40 = 0;   // will store last time a 40ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
  unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send
  //Startup burst: the first pass through PIDgroups[] is polled at this faster rate so the battery
  //info page fills in within seconds instead of over a minute. Counted down per request actually
  //sent, so it always terminates and the steady state polling rate is left untouched.
  static const unsigned long POLL_BURST_INTERVAL_MS = 2000;
  uint8_t mprun10r = 0;     //counter 0-20 for 0x1F2 message
  uint8_t mprun10 = 0;      //counter 0-3
  uint8_t mprun100 = 0;     //counter 0-3
  uint8_t counter_3B8 = 0;  //counter 0-14
  bool flip_3B8 = false;

  static const uint8_t ZE0_BATTERY = 0;
  static const uint8_t AZE0_BATTERY = 1;
  static const uint8_t ZE1_BATTERY = 2;

  // These CAN messages need to be sent towards the battery to keep it alive
  //Byte 2 carries CHG_STA_RQ (Charge_StatusTransitionRequest) in bits 6-5, left at 00b here and
  //overwritten from the user setting on every transmission. The low nibble of byte 7 is the
  //message checksum, so the constants in this file are the ones that go with 00b.
  CAN_frame LEAF_1F2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1F2,
                        .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
  CAN_frame LEAF_50B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 7,
                        .ID = 0x50B,
                        .data = {0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_50C = {.FD = false,
                        .ext_ID = false,
                        .DLC = 6,
                        .ID = 0x50C,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_1D4 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1D4,
                        .data = {0x6E, 0x6E, 0x00, 0x04, 0x07, 0x46, 0xE0, 0x44}};
  // Extra CAN messages for ZE1 batteries
  CAN_frame LEAF_355 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x355,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00}};
  CAN_frame LEAF_3B8 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x3B8, .data = {0x7F, 0xE8, 0x01, 0x07, 0xFF}};
  CAN_frame LEAF_5C5 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x5C5,
                        .data = {0x40, 0x01, 0x2F, 0x5E, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_5EC = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x5EC, .data = {0x00}};
  CAN_frame LEAF_626 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 6,
                        .ID = 0x626,
                        .data = {0x02, 0x00, 0xff, 0x1d, 0x20, 0x00}};
  // Active polling messages
  //Ordered so the values that identify an unknown pack come out first. The three static groups
  //(0x62 charge counters, 0x84 serial number, 0x83 part number) are read once and then skipped,
  //leaving 0x04/0x01/0x02/0x06 as the recurring rotation.
  uint8_t PIDgroups[7] = {0x62, 0x84, 0x04, 0x01, 0x02, 0x06, 0x83};
  //Start on the last entry so the first rotation step wraps to index 0.
  uint8_t PIDindex = sizeof(PIDgroups) / sizeof(PIDgroups[0]) - 1;
  uint8_t poll_burst_remaining = sizeof(PIDgroups) / sizeof(PIDgroups[0]);
  //Set while a BMS reset is running. It suspends the "already answered, skip it" rule for one
  //full pass through the list, so the static groups are asked again on the LBC's new session.
  bool repoll_static_groups = false;
  CAN_frame LEAF_GROUP_REQUEST = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x79B,
                                  .data = {2, 0x21, 1, 0, 0, 0, 0, 0}};
  CAN_frame LEAF_NEXT_LINE_REQUEST = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x79B,
                                      .data = {0x30, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
  CAN_frame LEAF_CLEAR_DTC = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x79B,
                              .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};
  // UDS ReadDTCInformation (0x19) / reportDTCByStatusMask (0x02) with status mask 0x0E:
  // testFailedThisOperationCycle, pendingDTC and confirmedDTC. A code is reported when any masked
  // bit is set in its status, so this asks for codes that have actually failed.
  // Do not widen this to 0xFF. That pulls in bit 6, testNotCompletedThisOperationCycle, which the
  // LBC sets on every code whose self test has not run this cycle. On a stationary pack that is
  // nearly all of them, so the reply becomes the LBC's supported code list rather than a fault
  // list: one capture returned 149 entries of which exactly one, a U1000, had really failed, the
  // rest carrying status 0x40. It also makes an erase look ineffective, since clearing faults
  // leaves every code untested and therefore still matching bit 6.
  // The LBC answers on 0x7BB with 59 02 <availabilityMask> followed by 4 bytes per DTC
  // (3-byte code + 1 status byte), multi-frame when more than one code is stored.
  CAN_frame LEAF_READ_DTC = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x79B,
                             .data = {0x03, 0x19, 0x02, 0x0E, 0x00, 0x00, 0x00, 0x00}};

  // DTC readout reassembly state. The reply shares the 0x7BB response ID with the periodic
  // group polling, so it is intercepted separately while a readout is in flight.
  static const uint16_t DTC_BUFFER_SIZE = 3 + 4 * DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT;
  static const unsigned long DTC_TIMEOUT_MS = 2000;
  uint8_t dtc_buffer[DTC_BUFFER_SIZE] = {0};
  uint16_t dtc_rx_total = 0;   // Total payload length announced by the ISO-TP first frame
  uint16_t dtc_rx_seen = 0;    // Bytes received so far, counted even when past our storage capacity
  uint16_t dtc_rx_len = 0;     // Bytes actually stored, capped at DTC_BUFFER_SIZE
  bool dtc_rx_active = false;  // A multi-frame reply is currently being reassembled
  bool dtc_read_in_progress = false;
  unsigned long dtc_request_millis = 0;
  bool dtc_clear_in_progress = false;
  unsigned long dtc_clear_millis = 0;
  // The LBC silently discards a diagnostic request that arrives while it is still transmitting a
  // response, so both DTC operations wait for the 0x7BB channel to go quiet before transmitting.
  // A group poll transfer completes in well under this, and polls are 10 s apart, so in practice
  // the wait is either zero or a few tens of milliseconds.
  static const unsigned long DTC_BUS_IDLE_MS = 100;
  static const uint8_t DTC_MAX_RETRIES = 2;
  unsigned long last_7bb_millis = 0;
  uint8_t dtc_read_retries = 0;

  // Generic tracking of our own outstanding UDS transaction on the 0x79B/0x7BB pair. The LBC serves
  // one request at a time, so a new one must not go out until the previous answer is complete,
  // whether that answer was good or an error. This covers the gap between sending a request and the
  // first response frame arriving, which a quiet-channel check alone cannot see.
  static const unsigned long UDS_RESPONSE_TIMEOUT_MS = 1000;
  bool uds_busy = false;
  unsigned long uds_request_millis = 0;
  uint16_t uds_rx_remaining = 0;

  // The Li-ion battery controller only accepts a multi-message query. In fact, the LBC transmits many
  // groups: the first one contains lots of High Voltage battery data as SOC, currents, and voltage; the second
  // replies with all the battery’s cells voltages in millivolt, the third and the fifth one are still unknown, the
  // fourth contains the four battery packs temperatures, and the last one tells which cell has the shunt active.
  // There are also two more groups: group 61, which replies with lots of CAN messages (up to 48); here we
  // found the SOH value, and group 84 that replies with the HV battery production serial.

  //Nissan LEAF battery parameters from constantly sent CAN
  uint8_t LEAF_battery_Type = ZE0_BATTERY;
  bool battery_can_alive = false;
#define WH_PER_GID 77                          //One GID is this amount of Watt hours
  uint16_t battery_Discharge_Power_Limit = 0;  //Limit in kW
  uint16_t battery_Charge_Power_Limit = 0;     //Limit in kW
  int16_t battery_MAX_POWER_FOR_CHARGER = 0;   //Limit in kW
  int16_t battery_SOC = 500;                   //0 - 100.0 % (0-1000) The real SOC% in the battery
  uint16_t battery_TEMP = 0;                   //Temporary value used in status checks
  uint16_t battery_Wh_Remaining = 0;           //Amount of energy in battery, in Wh
  uint16_t battery_GIDS = 273;                 //Startup in 24kWh mode
  uint16_t battery_MAX = 0;
  uint16_t battery_Max_GIDS = 273;                //Startup in 24kWh mode
  uint16_t battery_StateOfHealth = 99;            //State of health %
  uint16_t battery_Total_Voltage2 = 740;          //Battery voltage (0-450V) [0.5V/bit, so actual range 0-800]
  int16_t battery_Current2 = 0;                   //Battery current (-400-200A) [0.5A/bit, so actual range -800-400]
  int16_t battery_HistData_Temperature_MAX = 86;  //-40 to 86*C
  int16_t battery_HistData_Temperature_MIN = 86;  //-40 to 86*C
  int16_t battery_AverageTemperature = 6;         //Only available on ZE0, in celcius, -40 to +55
  uint8_t battery_Relay_Cut_Request = 0;          //battery_FAIL
  uint8_t battery_Failsafe_Status = 0;            //battery_STATUS
  bool battery_Interlock =
      true;  //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
  bool battery_Full_CHARGE_flag = false;  //battery_FCHGEND , Goes to 1 if battery is fully charged
  bool battery_MainRelayOn_flag = false;  //No-Permission=0, Main Relay On Permission=1
  bool battery_Capacity_Empty = false;    //battery_EMPTY, , Goes to 1 if battery is empty
  bool battery_HeatExist = false;      //battery_HEATEXIST, Specifies if battery pack is equipped with heating elements
  bool battery_Heating_Stop = false;   //When transitioning from 0->1, signals a STOP heat request
  bool battery_Heating_Start = false;  //When transitioning from 1->0, signals a START heat request
  bool battery_Batt_Heater_Mail_Send_Request = false;  //Stores info when a heat request is happening

  // Nissan LEAF battery data from polled CAN messages
  uint8_t battery_request_idx = 0;
  uint8_t group_7bb = 0;
  //ISO-TP payload length of the group reply currently being received. Leaf group replies are all
  //shorter than 256 bytes, so the low length byte of the first frame is enough to hold it.
  uint8_t group_7bb_length = 0;
  bool stop_battery_query = true;
  //Counted down once per 10s tick, and polling only starts on the tick after it reaches zero,
  //the first group request goes out 0 seconds after startup.
  uint8_t hold_off_with_polling_10seconds = 0;
  uint16_t battery_cell_voltages[96] = {0};     //array with all the cellvoltages
  bool battery_balancing_shunts[96] = {false};  //array with all the balancing resistors
  //Balancing classification state, see update_values()
  //The classifier tracks how often a group 0x06 read comes back with the shunt set completely
  //unchanged, over a sliding window of the most recent reads.
  static const uint8_t BALANCING_WINDOW_READS = 16;
  //Unchanged reads within that window at or above which the LBC is holding a set at rest, and at or
  //below which it is bleeding and re-deciding. Measured over 232 h on a 2017 30 kWh pack: while
  //balancing, 12% of reads come back unchanged whatever the pack state; while pending, 82-100% do.
  static const uint8_t BALANCING_UNCHANGED_FOR_IDLE = 13;   //81% of the window
  static const uint8_t BALANCING_UNCHANGED_FOR_ACTIVE = 7;  //44% of the window
  //Below this many flagged shunts the pack counts as not balancing at all
  static const uint8_t BALANCING_READY_BELOW_CELLS = 4;
  //Consecutive reads below that count before READY is reported. A dropped group 0x06 response can
  //momentarily read as all-clear, so a single low read is not enough to declare balancing finished.
  static const uint8_t BALANCING_READY_DEBOUNCE_READS = 3;
  //Previous group 0x06 shunt bitmap (96 bits packed into 3 words), for change detection
  uint32_t balancing_bitmap_prev[3] = {0};
  //true once balancing_bitmap_prev holds a real reading
  bool balancing_bitmap_valid = false;
  //One bit per recent read, set if that read came back with the shunt set unchanged
  uint16_t balancing_unchanged_window = 0;
  //How many reads the window holds so far, saturating at BALANCING_WINDOW_READS. Until it is full the
  //unchanged count is not meaningful - an empty window looks identical to one full of changed reads -
  //so no classification is made and the status stays as it was, UNKNOWN after a boot or a BMS reset.
  uint8_t balancing_window_fill = 0;
  //Consecutive reads with fewer than BALANCING_READY_BELOW_CELLS shunts flagged
  uint8_t balancing_low_reads = 0;
  //Which group 0x06 frames of the current response have arrived, so partial responses are discarded
  uint8_t balancing_frames_seen = 0;
  //Set by the group 0x06 handler once a complete response has been assembled
  bool balancing_data_fresh = false;
  //Applies a new balancing status, raising the start/end events on the ACTIVE edges
  void set_balancing_status(balancing_status_enum new_status);
  uint8_t battery_cellcounter = 0;
  uint16_t battery_min_max_voltage[2] = {0};  //contains cell min[0] and max[1] values in mV
  uint16_t battery_HX_pptt = 0;               //Pack conductance estimate (Hx), in hundredths of a percent
  uint16_t battery_insulation = 0;            //Insulation resistance
  uint16_t battery_charge_count_qc = 0;       //Lifetime number of quick (CHAdeMO) charges
  uint16_t battery_charge_count_l1l2 = 0;     //Lifetime number of L1/L2 (AC) charges
  uint16_t battery_temp_raw_1 = 718;
  uint8_t battery_temp_raw_2_highnibble = 0;
  uint16_t battery_temp_raw_2 = 718;
  uint16_t battery_temp_raw_3 = 718;  //This measurement not available on 2013+
  uint16_t battery_temp_raw_4 = 718;
  uint16_t battery_temp_raw_max = 0;
  uint16_t battery_temp_raw_min = 0;
  int16_t battery_temp_polled_max = 0;
  int16_t battery_temp_polled_min = 0;
  uint8_t BatterySerialNumber[15] = {0};  // Stores raw HEX values for ASCII chars
  uint8_t BatteryPartNumber[7] = {0};     // Stores raw HEX values for ASCII chars
  uint8_t stateMachineClearSOH = 0xFF;

#ifndef SMALL_FLASH_DEVICE

  // Clear SOH values

  uint32_t incomingChallenge = 0xFFFFFFFF;
  uint8_t solvedChallenge[8] = {0};
  bool challengeFailed = false;

  CAN_frame LEAF_CLEAR_SOH = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x79B,
                              .data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

#endif
};

#endif
