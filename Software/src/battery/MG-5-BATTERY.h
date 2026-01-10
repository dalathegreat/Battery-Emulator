#ifndef MG_5_BATTERY_H
#define MG_5_BATTERY_H

#include "UdsCanBattery.h"

//#ifndef SMALL_FLASH_DEVICE

class Mg5Battery : public UdsCanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual uint16_t handle_pid(uint16_t pid, uint32_t value);
  virtual void update_values();
  virtual void update_soc(uint16_t soc_times_ten);
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "MG 5 battery";
  // void startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID);
  // bool storeUDSPayload(const uint8_t* payload, uint8_t length);
  //void buildMG5_8AFrame();
  //bool isUDSMessageComplete();
  //virtual void print_formatted_dtc(uint32_t dtc24, uint8_t status);
  bool supports_contactor_close() { return true; }
  void request_open_contactors() { userRequestContactorClose = false; }
  void request_close_contactors() { userRequestContactorClose = true; }
  // virtual void read_DTC() { userRequestReadDTC = true; }
  // virtual void reset_DTC() { userRequestClearDTC = true; }

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3100;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int TOTAL_BATTERY_CAPACITY_WH = 52500;  // 52.5 kWh

  static const uint16_t MAX_CHARGE_POWER_W = 11000;
  static const uint16_t CHARGE_TRICKLE_POWER_W = 20;
  static const uint16_t DERATE_CHARGE_ABOVE_SOC = 9000;  // in 0.01% units

  static const uint16_t MAX_DISCHARGE_POWER_W = 11000;
  static const uint16_t DERATE_DISCHARGE_BELOW_SOC = 1500;  // in 0.01% units
  static const uint16_t DISCHARGE_MIN_SOC = 1000;

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send

  uint16_t cellVoltageValidTime = 0;
  static const uint8_t CELL_VOLTAGE_TIMEOUT = 10;  // in seconds

  uint8_t previousState = 0;

  // poll counters
  //int uds_slow_req_id_counter = -1;

  // rolling counter for 0x8A (0x10..0x1F pattern)
  uint8_t mg5_8a_counter = 0x10;

  // simple toggle to alternate 0x80/0x00 and 0x7F/0xFF (alive / redundancy style)
  bool mg5_8a_flip = false;

  //bool uds_tx_in_flight = false;
  // bool userRequestReadDTC = false;
  // bool userRequestClearDTC = false;
  bool userRequestContactorClose = true;
  bool contactorClosed = false;
  // unsigned long uds_req_started_ms = 0;
  // unsigned long uds_timeout_ms = 0;
  // const unsigned long UDS_PID_REFRESH_MS = 500;         // inter-request gap
  // const unsigned long UDS_TIMEOUT_BEFORE_FF_MS = 1100;  // no reply yet
  // const unsigned long UDS_TIMEOUT_AFTER_FF_MS = 1100;   // multi-frame in progress
  // const unsigned long UDS_TIMEOUT_AFTER_BOOT = 2000;    // DELAY TO START UDS AFTER BOOT-UP
  // const unsigned long TESTER_PRESENT_PERIOD_MS = 1000;  // ~1 s

  // A structure to keep track of the ongoing multi-frame UDS response
  // typedef struct {
  //   bool UDS_inProgress;                // Are we currently receiving a multi-frame message?
  //   uint16_t UDS_expectedLength;        // Expected total payload length
  //   uint16_t UDS_bytesReceived;         // How many bytes have been stored so far
  //   uint8_t UDS_moduleID;               // The "module" indicated by the first frame
  //   uint8_t receivedInBatch;            // Number of CFs received in the current batch
  //   uint8_t UDS_buffer[1024];           // Buffer for the reassembled data
  //   unsigned long UDS_lastFrameMillis;  // Timestamp of last frame (for timeouts, if desired)
  // } UDS_RxContext;

  // A single global UDS context, since only one module can respond at a time
  //  UDS_RxContext gUDSContext;

  //0x781 UDS diagnostic requests - Extended Session Control
  // CAN_frame MG5_781_ses_ctrl = {.FD = false,
  //                               .ext_ID = false,
  //                               .DLC = 8,
  //                               .ID = 0x781,
  //                               .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x781 UDS diagnostic requests - Session Response
  // CAN_frame MG5_781_ses_resp = {.FD = false,
  //                               .ext_ID = false,
  //                               .DLC = 8,
  //                               .ID = 0x789,
  //                               .data = {0x02, 0x50, 0x03, 0x00, 0x32, 0x01, 0xF4, 0x00}};

  // //0x781 UDS diagnostic requests - keep alive
  // CAN_frame MG5_781_keep_alive = {.FD = false,
  //                                 .ext_ID = false,
  //                                 .DLC = 8,
  //                                 .ID = 0x781,
  //                                 .data = {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  //0x781 UDS diagnostic requests - keep alive
  // CAN_frame MG5_781_RQ_CONTINUE_MULTIFRAME = {.FD = false,
  //                                             .ext_ID = false,
  //                                             .DLC = 8,
  //                                             .ID = 0x781,
  //                                             .data = {0x30, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x781 UDS diagnostic requests - request all DTC's
  // CAN_frame MG5_781_RQ_DTCs = {.FD = false,
  //                              .ext_ID = false,
  //                              .DLC = 8,
  //                              .ID = 0x781,
  //                              .data = {0x03, 0x19, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00}};

  // //0x781 UDS diagnostic requests - clear all DTC's
  // CAN_frame MG5_781_CLEAR_DTCs = {.FD = false,
  //                                 .ext_ID = false,
  //                                 .DLC = 8,
  //                                 .ID = 0x781,
  //                                 .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};

  static const int POLL_BUS_VOLTAGE = 0xB041;
  static const int POLL_BAT_VOLTAGE = 0xB042;
  static const int POLL_BAT_CURRENT = 0xB043;
  static const int POLL_BAT_RESISTANCE = 0xB045;
  static const int POLL_BAT_SOC = 0xB046;
  static const int POLL_BMS_ERR = 0xB047;
  static const int POLL_BMS_STATE = 0xB048;
  static const int POLL_BAT_RELAY_B = 0xB049;
  static const int POLL_BAT_RELAY_G = 0xB04A;
  static const int POLL_BAT_RELAY_P = 0xB052;
  static const int POLL_BAT_TEMP = 0xB056;
  static const int POLL_MAX_CELL = 0xB058;
  static const int POLL_MIN_CELL = 0xB059;
  static const int POLL_COOLANT_TEMP = 0xB05C;
  static const int POLL_BAT_SOH = 0xB061;
  static const int POLL_BMS_TIME = 0xB06D;

  // CAN_frame MG5_781_RQ_BUS_VOLTAGE = {.FD = false,
  //                                     .ext_ID = false,
  //                                     .DLC = 8,
  //                                     .ID = 0x781,
  //                                     .data = {0x03, 0x22, 0xB0, 0x41, 0x00, 0x00, 0x00, 0x00}};  //battery bus voltage

  // CAN_frame MG5_781_RQ_BAT_VOLTAGE = {.FD = false,
  //                                     .ext_ID = false,
  //                                     .DLC = 8,
  //                                     .ID = 0x781,
  //                                     .data = {0x03, 0x22, 0xB0, 0x42, 0x00, 0x00, 0x00, 0x00}};  //  Battery Voltage

  // CAN_frame MG5_781_RQ_BAT_CURRENT = {.FD = false,
  //                                     .ext_ID = false,
  //                                     .DLC = 8,
  //                                     .ID = 0x781,
  //                                     .data = {0x03, 0x22, 0xB0, 0x43, 0x00, 0x00, 0x00, 0x00}};  //  Current

  // CAN_frame MG5_781_RQ_BAT_RESISTANCE = {.FD = false,
  //                                        .ext_ID = false,
  //                                        .DLC = 8,
  //                                        .ID = 0x781,
  //                                        .data = {0x03, 0x22, 0xB0, 0x45, 0x00, 0x00, 0x00, 0x00}};  //  Resistance

  // CAN_frame MG5_781_RQ_BAT_SOC = {.FD = false,
  //                                 .ext_ID = false,
  //                                 .DLC = 8,
  //                                 .ID = 0x781,
  //                                 .data = {0x03, 0x22, 0xB0, 0x46, 0x00, 0x00, 0x00, 0x00}};  //  State of Charge

  // CAN_frame MG5_781_RQ_BMS_ERR = {.FD = false,
  //                                 .ext_ID = false,
  //                                 .DLC = 8,
  //                                 .ID = 0x781,
  //                                 .data = {0x03, 0x22, 0xB0, 0x47, 0x00, 0x00, 0x00, 0x00}};  //  BMS Error

  // CAN_frame MG5_781_RQ_BMS_STATE = {.FD = false,
  //                                   .ext_ID = false,
  //                                   .DLC = 8,
  //                                   .ID = 0x781,
  //                                   .data = {0x03, 0x22, 0xB0, 0x48, 0x00, 0x00, 0x00, 0x00}};  //  BMS Status

  // CAN_frame MG5_781_RQ_BAT_RELAY_B = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x49, 0x00, 0x00, 0x00, 0x00}};  //  Battery Relay Status

  // CAN_frame MG5_781_RQ_BAT_RELAY_G = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x4A, 0x00, 0x00, 0x00, 0x00}};  //  Battery Relay Status

  // CAN_frame MG5_781_RQ_BAT_RELAY_P = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x52, 0x00, 0x00, 0x00, 0x00}};  //  Battery Relay Status

  // CAN_frame MG5_781_RQ_BAT_TEMP = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x56, 0x00, 0x00, 0x00, 0x00}};  //  Battery Temperature Status

  // CAN_frame MG5_781_RQ_MAX_CELL = {.FD = false,
  //                                  .ext_ID = false,
  //                                  .DLC = 8,
  //                                  .ID = 0x781,
  //                                  .data = {0x03, 0x22, 0xB0, 0x58, 0x00, 0x00, 0x00, 0x00}};  //  MAX Cell Voltage

  // CAN_frame MG5_781_RQ_MIN_CELL = {.FD = false,
  //                                  .ext_ID = false,
  //                                  .DLC = 8,
  //                                  .ID = 0x781,
  //                                  .data = {0x03, 0x22, 0xB0, 0x59, 0x00, 0x00, 0x00, 0x00}};  //  MIN Cell Voltage

  // CAN_frame MG5_781_RQ_COOLANT_TEMP = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x5C, 0x00, 0x00, 0x00, 0x00}};  //  Coolant Temperature Status

  // CAN_frame MG5_781_RQ_BAT_SOH = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x61, 0x00, 0x00, 0x00, 0x00}};  //  Battery State of Health

  // CAN_frame MG5_781_RQ_BMS_TIME = {
  //     .FD = false,
  //     .ext_ID = false,
  //     .DLC = 8,
  //     .ID = 0x781,
  //     .data = {0x03, 0x22, 0xB0, 0x6D, 0x00, 0x00, 0x00, 0x00}};  //  Battery Management System Time

  CAN_frame MG5_8A = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x08A,
                      .data = {0x80, 0x00, 0x00, 0x04, 0x00, 0x02, 0xBB, 0x3F}};

  static constexpr CAN_frame MG5_1F1 = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 8,
                                        .ID = 0x1F1,
                                        .data = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //Setup Fast UDS values to poll for
  //CAN_frame* UDS_REQUESTS_FAST[0] = {};
  //int numFastUDSreqs =
  //    sizeof(UDS_REQUESTS_FAST) / sizeof(UDS_REQUESTS_FAST[0]);  //Store Number of elements in the array

  //Setup Slow UDS values to poll for
  // CAN_frame* UDS_REQUESTS_SLOW[15] = {
  //     &MG5_781_RQ_BUS_VOLTAGE, &MG5_781_RQ_BAT_VOLTAGE,  &MG5_781_RQ_BAT_CURRENT, &MG5_781_RQ_BAT_RESISTANCE,
  //     &MG5_781_RQ_BAT_SOC,     &MG5_781_RQ_BMS_ERR,      &MG5_781_RQ_BMS_STATE,   &MG5_781_RQ_BAT_RELAY_B,
  //     &MG5_781_RQ_BAT_RELAY_G, &MG5_781_RQ_BAT_RELAY_P,  &MG5_781_RQ_BAT_TEMP,    &MG5_781_RQ_MAX_CELL,
  //     &MG5_781_RQ_MIN_CELL,    &MG5_781_RQ_COOLANT_TEMP, &MG5_781_RQ_BAT_SOH
  //     //&MG5_781_RQ_DTCs
  // };
  //int numSlowUDSreqs =
  //    sizeof(UDS_REQUESTS_SLOW) / sizeof(UDS_REQUESTS_SLOW[0]);  // Store Number of elements in the array

  // tiny helper for cycling request indices
  // static int increment_uds_req_id_counter(int cur, int n) {
  //   if (n <= 0)
  //     return -1;
  //   cur++;
  //   if (cur >= n)
  //     cur = 0;
  //   return cur;
  // }

  //compute checksum for MG5 0x8A message
  // uint8_t computeMG5_8AChecksum(const uint8_t* bytes7) const {
  //   uint8_t crc = 0;
  //   for (int i = 0; i < 7; ++i) {
  //     crc += bytes7[i];
  //   }
  //   return crc;
  // }
};

//#endif

#endif
