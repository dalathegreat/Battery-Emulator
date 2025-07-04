#ifndef BMW_PHEV_BATTERY_H
#define BMW_PHEV_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "BMW-PHEV-HTML.h"
#include "CanBattery.h"

#ifdef BMW_PHEV_BATTERY
#define SELECTED_BATTERY_CLASS BmwPhevBattery
#endif

class BmwPhevBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  BmwPhevHtmlRenderer renderer;

  static const int MAX_PACK_VOLTAGE_DV = 4650;  //4650 = 465.0V
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4300;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2800;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_DISCHARGE_POWER_ALLOWED_W = 10000;
  static const int MAX_CHARGE_POWER_ALLOWED_W = 10000;
  static const int MAX_CHARGE_POWER_WHEN_TOPBALANCING_W = 500;
  static const int RAMPDOWN_SOC =
      9000;  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%
  static const int STALE_PERIOD_CONFIG =
      3600000;  //Number of milliseconds before critical values are classed as stale/stuck 1800000 = 3600 seconds / 60mins

  bool isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime);
  void startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID);
  bool storeUDSPayload(const uint8_t* payload, uint8_t length);
  bool isUDSMessageComplete();
  void processCellVoltages();
  void wake_battery_via_canbus();
  uint8_t increment_alive_counter(uint8_t counter);

  unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
  unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
  unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
  unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
  unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
  unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
  unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

  static const int ALIVE_MAX_VALUE = 14;  // BMW CAN messages contain alive counter, goes from 0...14

  enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

  CmdState cmdState = SOC;

  // A structure to keep track of the ongoing multi-frame UDS response
  typedef struct {
    bool UDS_inProgress;                // Are we currently receiving a multi-frame message?
    uint16_t UDS_expectedLength;        // Expected total payload length
    uint16_t UDS_bytesReceived;         // How many bytes have been stored so far
    uint8_t UDS_moduleID;               // The "module" indicated by the first frame
    uint8_t receivedInBatch;            // Number of CFs received in the current batch
    uint8_t UDS_buffer[256];            // Buffer for the reassembled data
    unsigned long UDS_lastFrameMillis;  // Timestamp of last frame (for timeouts, if desired)
  } UDS_RxContext;

  // A single global UDS context, since only one module can respond at a time
  UDS_RxContext gUDSContext;

  //Vehicle CAN START

  CAN_frame BMWiX_0C0 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 2,
      .ID = 0x0C0,
      .data = {
          0xF0,
          0x08}};  // Keep Alive 2 BDC>SME  200ms First byte cycles F0 > FE  second byte 08 - MINIMUM ID TO KEEP SME AWAKE

  CAN_frame BMW_13E = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x13E,
                       .data = {0xFF, 0x31, 0xFA, 0xFA, 0xFA, 0xFA, 0x0C, 0x00}};

  //Vehicle CAN END

  //Request Data CAN START

  CAN_frame BMW_PHEV_BUS_WAKEUP_REQUEST = {
      .FD = false,
      .ext_ID = false,
      .DLC = 4,
      .ID = 0x554,
      .data = {
          0x5A, 0xA5, 0x5A,
          0xA5}};  // Won't work at 500kbps! Ideally sent at 50kbps - but can also achieve wakeup at 100kbps (helps with library support but might not be as reliable). Might need to be sent twice + clear buffer

  CAN_frame BMWPHEV_6F1_REQUEST_SOC = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 5,
                                       .ID = 0x6F1,
                                       .data = {0x07, 0x03, 0x22, 0xDD, 0xC4}};  //  SOC%

  CAN_frame BMWPHEV_6F1_REQUEST_SOH = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 5,
                                       .ID = 0x6F1,
                                       .data = {0x07, 0x03, 0x22, 0xDD, 0x7B}};  //  SOH%

  CAN_frame BMWPHEV_6F1_REQUEST_CURRENT = {.FD = false,
                                           .ext_ID = false,
                                           .DLC = 5,
                                           .ID = 0x6F1,
                                           .data = {0x07, 0x03, 0x22, 0xDD, 0x69}};  //  SOH%

  CAN_frame BMWPHEV_6F1_REQUEST_VOLTAGE_LIMITS = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDD, 0x7E}};  //  Pack Voltage Limits  Multi Frame

  CAN_frame BMWPHEV_6F1_REQUEST_PAIRED_VIN = {.FD = false,
                                              .ext_ID = false,
                                              .DLC = 5,
                                              .ID = 0x6F1,
                                              .data = {0x07, 0x03, 0x22, 0xF1, 0x90}};  //  SME Paired VIN

  CAN_frame BMWPHEV_6F1_REQUEST_ISO_READING1 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {
          0x07, 0x03, 0x22, 0xDD,
          0x6A}};  // MULTI FRAME ISOLATIONSWIDERSTAND 62 DD 6A [07 D0] [07 D0] [07 D0] [01] [01] [01] 00 00 00 00 00   [EXT Reading] [INT reading] [ EXT - 0 not plausible, 1 plausible]

  CAN_frame BMWPHEV_6F1_REQUEST_ISO_READING2 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xD6,
               0xD9}};  //  R_ISO_ROH 62 D6 D9 [07 FF] [13] (2047kohm) quality of reading 0-21 (19)

  CAN_frame BMWPHEV_6F1_REQUEST_PACK_INFO = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDF, 0x71}};  //   62 DF 71 00 60 1C 25 1C? Cell Count, Module Count

  CAN_frame BMWPHEV_6F1_REQUEST_CURRENT_LIMITS = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDD, 0x7D}};  //  Pack Current Limits  Multi Frame

  CAN_frame BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDD, 0xB4}};  //Main Battery Voltage (Pre Contactor)

  CAN_frame BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDD, 0x66}};  //Main Battery Voltage (After Contactor)

  CAN_frame BMWPHEV_6F1_REQUEST_CELLSUMMARY = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDF, 0xA0}};  //Min and max cell voltage + temps   6.55V = Qualifier Invalid?

  CAN_frame BMWPHEV_6F1_REQUEST_CELLS_INDIVIDUAL_VOLTS = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {0x07, 0x03, 0x22, 0xDF, 0xA5}};  //All individual cell voltages

  CAN_frame BMWPHEV_6F1_REQUEST_CELL_TEMP = {
      .FD = false,
      .ext_ID = false,
      .DLC = 5,
      .ID = 0x6F1,
      .data = {
          0x07, 0x03, 0x22, 0xDD,
          0xC0}};  // UDS Request Cell Temperatures min max avg. Has continue frame min in first, then max + avg in second frame

  CAN_frame BMW_6F1_REQUEST_CONTINUE_MULTIFRAME = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x6F1,
      .data = {
          0x07, 0x30, 0x03, 0x00, 0x00, 0x00, 0x00,
          0x00}};  //Request continued frames from UDS Multiframe request  byte[2] is the request messages to return per continue. default 0x03, all is 0x00

  CAN_frame BMW_6F1_REQUEST_HARD_RESET = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 4,
                                          .ID = 0x6F1,
                                          .data = {0x07, 0x03, 0x11, 0x01}};  // Reset BMS - TBC

  CAN_frame BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x6F1,
      .data = {0x07, 0x04, 0x2E, 0xDD, 0x61, 0x01, 0x00, 0x00}};  // Request Contactors Close - Unconfirmed
  CAN_frame BMWPHEV_6F1_REQUEST_CONTACTORS_OPEN = {
      .FD = false,
      .ext_ID = false,
      .DLC = 6,
      .ID = 0x6F1,
      .data = {0x07, 0x04, 0x2E, 0xDD, 0x61, 0x00, 0x00, 0x00}};  // Request Contactors Open - Unconfirmed

  CAN_frame BMWPHEV_6F1_REQUEST_BALANCING_STATUS = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x6F1,
      .data = {0x07, 0x04, 0x31, 0x03, 0xAD, 0x6B, 0x00,
               0x00}};  // Balancing status.  Response 7DLC F1 05 71 03 AD 6B 01   (01 = active)  (03 not active)

  CAN_frame BMWPHEV_6F1_REQUEST_ISOLATION_TEST = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x6F1,
      .data = {0x07, 0x04, 0x31, 0x01, 0xAD, 0x61, 0x00, 0x00}};  // Start Isolation Test

  CAN_frame BMWPHEV_6F1_REQUEST_BALANCING_START = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x6F1,
      .data = {0x07, 0x04, 0x31, 0x01, 0xAD, 0x6B, 0x00, 0x00}};  // Balancing start request

  CAN_frame BMWPHEV_6F1_REQUEST_BALANCING_STOP = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x6F1,
      .data = {0x07, 0x04, 0x31, 0x02, 0xAD, 0x6B, 0x00, 0x00}};  // Balancing stop request

  //Action Requests:
  CAN_frame BMW_10B = {.FD = false,
                       .ext_ID = false,
                       .DLC = 3,
                       .ID = 0x10B,
                       .data = {0xCD, 0x00, 0xFC}};  // Contactor closing command?

  CAN_frame BMWPHEV_6F1_CELL_SOC = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 5,
                                    .ID = 0x6F1,
                                    .data = {0x07, 0x03, 0x22, 0xE5, 0x9A}};
  CAN_frame BMWPHEV_6F1_CELL_TEMP = {.FD = false,
                                     .ext_ID = false,
                                     .DLC = 5,
                                     .ID = 0x6F1,
                                     .data = {0x07, 0x03, 0x22, 0xE5, 0xCA}};
  //Request Data CAN End

  bool battery_awake = false;

  //Setup Fast UDS values to poll for
  CAN_frame* UDS_REQUESTS_FAST[6] = {&BMWPHEV_6F1_REQUEST_CELLSUMMARY,
                                     &BMWPHEV_6F1_REQUEST_SOC,
                                     &BMWPHEV_6F1_REQUEST_CURRENT,
                                     &BMWPHEV_6F1_REQUEST_VOLTAGE_LIMITS,
                                     &BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR,
                                     &BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR};
  int numFastUDSreqs =
      sizeof(UDS_REQUESTS_FAST) / sizeof(UDS_REQUESTS_FAST[0]);  //Store Number of elements in the array

  //Setup Slow UDS values to poll for
  CAN_frame* UDS_REQUESTS_SLOW[8] = {&BMWPHEV_6F1_REQUEST_ISO_READING1,           &BMWPHEV_6F1_REQUEST_ISO_READING2,
                                     &BMWPHEV_6F1_REQUEST_CURRENT_LIMITS,         &BMWPHEV_6F1_REQUEST_SOH,
                                     &BMWPHEV_6F1_REQUEST_CELLS_INDIVIDUAL_VOLTS, &BMWPHEV_6F1_REQUEST_CELL_TEMP,
                                     &BMWPHEV_6F1_REQUEST_BALANCING_STATUS,       &BMWPHEV_6F1_REQUEST_PAIRED_VIN};
  int numSlowUDSreqs =
      sizeof(UDS_REQUESTS_SLOW) / sizeof(UDS_REQUESTS_SLOW[0]);  // Store Number of elements in the array

  //PHEV intermediate vars
  //#define UDS_LOG //Useful for logging multiframe handling
  uint16_t battery_max_charge_voltage = 0;
  int16_t battery_max_charge_amperage = 0;
  uint16_t battery_min_discharge_voltage = 0;
  int16_t battery_max_discharge_amperage = 0;

  uint8_t startup_counter_contactor = 0;
  uint8_t alive_counter_20ms = 0;
  uint8_t BMW_13E_counter = 0;

  uint32_t battery_BEV_available_power_shortterm_charge = 0;
  uint32_t battery_BEV_available_power_shortterm_discharge = 0;
  uint32_t battery_BEV_available_power_longterm_charge = 0;
  uint32_t battery_BEV_available_power_longterm_discharge = 0;

  uint16_t battery_predicted_energy_charge_condition = 0;
  uint16_t battery_predicted_energy_charging_target = 0;

  uint16_t battery_prediction_voltage_shortterm_charge = 0;
  uint16_t battery_prediction_voltage_shortterm_discharge = 0;
  uint16_t battery_prediction_voltage_longterm_charge = 0;
  uint16_t battery_prediction_voltage_longterm_discharge = 0;

  uint8_t battery_status_service_disconnection_plug = 0;
  uint8_t battery_status_measurement_isolation = 0;
  uint8_t battery_request_abort_charging = 0;
  uint16_t battery_prediction_duration_charging_minutes = 0;
  uint8_t battery_prediction_time_end_of_charging_minutes = 0;
  uint16_t battery_energy_content_maximum_kWh = 0;

  uint8_t battery_request_operating_mode = 0;
  uint16_t battery_target_voltage_in_CV_mode = 0;
  uint8_t battery_request_charging_condition_minimum = 0;
  uint8_t battery_request_charging_condition_maximum = 0;
  uint16_t battery_display_SOC = 0;

  uint8_t battery_status_error_isolation_external_Bordnetz = 0;
  uint8_t battery_status_error_isolation_internal_Bordnetz = 0;
  uint8_t battery_request_cooling = 0;
  uint8_t battery_status_valve_cooling = 0;
  uint8_t battery_status_error_locking = 0;
  uint8_t battery_status_precharge_locked = 0;
  uint8_t battery_status_disconnecting_switch = 0;
  uint8_t battery_status_emergency_mode = 0;
  uint8_t battery_request_service = 0;
  uint8_t battery_error_emergency_mode = 0;
  uint8_t battery_status_error_disconnecting_switch = 0;
  uint8_t battery_status_warning_isolation = 0;
  uint8_t battery_status_cold_shutoff_valve = 0;
  int16_t battery_temperature_HV = 0;
  int16_t battery_temperature_heat_exchanger = 0;
  int16_t battery_temperature_max = 0;
  int16_t battery_temperature_min = 0;
  bool pack_limit_info_available = false;
  bool cell_limit_info_available = false;

  //iX Intermediate vars

  uint32_t battery_serial_number = 0;
  int32_t battery_current = 0;
  int16_t battery_voltage = 3700;  //Initialize as valid - should be fixed in future
  int16_t terminal30_12v_voltage = 0;
  int16_t battery_voltage_after_contactor = 0;
  int16_t min_soc_state = 5000;
  int16_t avg_soc_state = 5000;
  int16_t max_soc_state = 5000;
  int16_t min_soh_state = 9999;  // Uses E5 45, also available in 78 73
  int16_t avg_soh_state = 9999;  // Uses E5 45, also available in 78 73
  int16_t max_soh_state = 9999;  // Uses E5 45, also available in 78 73
  uint16_t max_design_voltage = 0;
  uint16_t min_design_voltage = 0;
  int32_t remaining_capacity = 0;
  int32_t max_capacity = 0;

  int16_t main_contactor_temperature = 0;
  int16_t min_cell_voltage = 3700;  //Initialize as valid - should be fixed in future
  int16_t max_cell_voltage = 3700;  //Initialize as valid - should be fixed in future
  unsigned long min_cell_voltage_lastchanged = 0;
  unsigned long max_cell_voltage_lastchanged = 0;
  unsigned min_cell_voltage_lastreceived = 0;
  unsigned max_cell_voltage_lastreceived = 0;
  int16_t allowable_charge_amps = 0;     //E5 62
  int16_t allowable_discharge_amps = 0;  //E5 62

  int32_t iso_safety_int_kohm = 0;  //STAT_ISOWIDERSTAND_INT_WERT
  int32_t iso_safety_ext_kohm = 0;  //STAT_ISOWIDERSTAND_EXT_STD_WERT
  int32_t iso_safety_trg_kohm = 0;
  int32_t iso_safety_ext_plausible = 0;  //STAT_ISOWIDERSTAND_EXT_TRG_PLAUS
  int32_t iso_safety_int_plausible = 0;  //STAT_ISOWIDERSTAND_EXT_TRG_WERT
  int32_t iso_safety_trg_plausible = 0;
  int32_t iso_safety_kohm = 0;          //STAT_R_ISO_ROH_01_WERT
  int32_t iso_safety_kohm_quality = 0;  //STAT_R_ISO_ROH_QAL_01_INFO Quality of measurement 0-21 (higher better)

  uint8_t paired_vin[17] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //17 Byte array for paired VIN

  int16_t count_full_charges = 0;  //TODO  42
  int16_t count_charges = 0;       //TODO  42
  int16_t hvil_status = 0;
  int16_t voltage_qualifier_status = 0;    //0 = Valid, 1 = Invalid
  int16_t balancing_status = 0;            //4 = not active
  uint8_t contactors_closed = 0;           //TODO  E5 BF  or E5 51
  uint8_t contactor_status_precharge = 0;  //TODO E5 BF
  uint8_t contactor_status_negative = 0;   //TODO E5 BF
  uint8_t contactor_status_positive = 0;   //TODO E5 BF
  uint8_t uds_fast_req_id_counter = 0;
  uint8_t uds_slow_req_id_counter = 0;
  uint8_t detected_number_of_cells = 96;
  const unsigned long STALE_PERIOD =
      STALE_PERIOD_CONFIG;  // Time in milliseconds to check for staleness (e.g., 5000 ms = 5 seconds)

  byte iX_0C0_counter = 0xF0;  // Initialize to 0xF0

  //End iX Intermediate vars

  uint8_t current_cell_polled = 0;
};

#endif
