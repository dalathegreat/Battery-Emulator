#ifndef MG_5_BATTERY_H
#define MG_5_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef MG_5_BATTERY
#define SELECTED_BATTERY_CLASS Mg5Battery
#endif





class Mg5Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void update_soc(uint16_t soc_times_ten);
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "MG 5 battery";
  void startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID);
  bool storeUDSPayload(const uint8_t* payload, uint8_t length);
  bool isUDSMessageComplete();
  virtual void print_formatted_dtc(uint32_t dtc24, uint8_t status);

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3100;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200  = 0;
  unsigned long previousMillis1000 = 0;
  unsigned long previousMillis6000 = 0;
  unsigned long previousMillisDTC = 0;
  unsigned long previousMillisPID = 0;

  #define INTERVAL_1000_MS 1000UL
  #define INTERVAL_6000_MS 6000UL

  uint8_t previousState = 0;

    // For calculating charge and discharge power
  float RealVoltage;
  float RealSoC;
  float tempfloat;

  int BMS_SOC = 0;

  static const uint16_t CELL_VOLTAGE_TIMEOUT = 10;  // in seconds
  uint16_t cellVoltageValidTime = 0;
  uint16_t soc1 = 0;
  uint16_t soc2 = 0;
  uint16_t cell_id = 0;
  uint16_t v = 0;
  uint8_t transmitIndex = 0;  //For polling switchcase

  const int MaxChargePower = 3000;  // Maximum allowable charge power, excluding the taper
  const int StartChargeTaper = 90;  // Battery percentage above which the charge power will taper to zero
  const float ChargeTaperExponent =
      1;  // Shape of charge power taper to zero. 1 is linear. >1 reduces quickly and is small at nearly full.
  const int TricklePower = 20;  // Minimimum trickle charge or discharge power (W)

  const int MaxDischargePower = 4000;  // Maximum allowable discharge power, excluding the taper
  const int MinSoC = 20;               // Minimum SoC allowed
  const int StartDischargeTaper = 30;  // Battery percentage below which the discharge power will taper to zero
  const float DischargeTaperExponent =
      1;  // Shape of discharge power taper to zero. 1 is linear. >1 red
    // A structure to keep track of the ongoing multi-frame UDS response
  typedef struct {
    bool UDS_inProgress;                // Are we currently receiving a multi-frame message?
    uint16_t UDS_expectedLength;        // Expected total payload length
    uint16_t UDS_bytesReceived;         // How many bytes have been stored so far
    uint8_t UDS_moduleID;               // The "module" indicated by the first frame
    uint8_t receivedInBatch;            // Number of CFs received in the current batch
    uint8_t UDS_buffer[1024];            // Buffer for the reassembled data
    unsigned long UDS_lastFrameMillis;  // Timestamp of last frame (for timeouts, if desired)
  } UDS_RxContext;

  // A single global UDS context, since only one module can respond at a time
  UDS_RxContext gUDSContext;

  //0x781 UDS diagnostic requests - Extended Session Control                   
  CAN_frame MG5_781_ses_ctrl = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x781,
                         .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //0x781 UDS diagnostic requests - Session Response                  
  CAN_frame MG5_781_ses_resp = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x789,
                         .data = {0x02, 0x50, 0x03, 0x00, 0x32, 0x01, 0xF4, 0x00}};

 //0x781 UDS diagnostic requests - keep alive                   
  CAN_frame MG5_781_keep_alive = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x781,
                         .data = {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
 //0x781 UDS diagnostic requests - keep alive                   
  CAN_frame MG5_781_RQ_CONTINUE_MULTIFRAME = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x781,
                         .data = {0x30, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}};

 //0x781 UDS diagnostic requests - request all DTC's                   
  CAN_frame MG5_781_RQ_DTCs = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x781,
                         .data = {0x03, 0x19, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00}};
                                      
  CAN_frame MG5_781_RQ_BUS_VOLTAGE = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 8,
                                       .ID = 0x781,
                                       .data = {0x03, 0x22, 0xB0, 0x41, 0x00, 0x00, 0x00, 0x00}};  //battery bus voltage

  CAN_frame MG5_781_RQ_BAT_VOLTAGE = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 8,
                                       .ID = 0x781,
                                       .data = {0x03, 0x22, 0xB0, 0x42, 0x00, 0x00, 0x00, 0x00}};  //  Battery Voltage

  CAN_frame MG5_781_RQ_BAT_CURRENT = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 8,
                                       .ID = 0x781,
                                       .data = {0x03, 0x22, 0xB0, 0x43, 0x00, 0x00, 0x00, 0x00}};  //  Current

  CAN_frame MG5_781_RQ_BAT_RESISTANCE = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 8,
                                       .ID = 0x781,
                                       .data = {0x03, 0x22, 0xB0, 0x45, 0x00, 0x00, 0x00, 0x00}};  //  Resistance

  CAN_frame MG5_781_RQ_BAT_SOC = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 8,
                                       .ID = 0x781,
                                       .data = {0x03, 0x22, 0xB0, 0x46, 0x00, 0x00, 0x00, 0x00}};  //  State of Charge

  CAN_frame MG5_781_RQ_BMS_ERR = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 8,
                                       .ID = 0x781,
                                       .data = {0x03, 0x22, 0xB0, 0x47, 0x00, 0x00, 0x00, 0x00}};  //  BMS Error

  CAN_frame MG5_781_RQ_BMS_STATE = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x781,
                                      .data = {0x03, 0x22, 0xB0, 0x48, 0x00, 0x00, 0x00, 0x00}};  //  BMS Status

  CAN_frame MG5_781_RQ_BAT_RELAY_B = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x781,
                                      .data = {0x03, 0x22, 0xB0, 0x49, 0x00, 0x00, 0x00, 0x00}};  //  Battery Relay Status

  CAN_frame MG5_781_RQ_BAT_RELAY_G = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x4A, 0x00, 0x00, 0x00, 0x00}};  //  Battery Relay Status

  CAN_frame MG5_781_RQ_BAT_RELAY_P = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x52, 0x00, 0x00, 0x00, 0x00}};  //  Battery Relay Status

  CAN_frame MG5_781_RQ_BAT_TEMP = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x56, 0x00, 0x00, 0x00, 0x00}};  //  Battery Temperature Status

  CAN_frame MG5_781_RQ_MAX_CELL = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x58, 0x00, 0x00, 0x00, 0x00}};  //  MAX Cell Voltage

  CAN_frame MG5_781_RQ_MIN_CELL = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x781,
                                      .data = {0x03, 0x22, 0xB0, 0x59, 0x00, 0x00, 0x00, 0x00}};  //  MIN Cell Voltage

  CAN_frame MG5_781_RQ_COOLANT_TEMP = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x5C, 0x00, 0x00, 0x00, 0x00}};  //  Coolant Temperature Status

  CAN_frame MG5_781_RQ_BAT_SOH = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x61, 0x00, 0x00, 0x00, 0x00}};  //  Battery State of Health

  CAN_frame MG5_781_RQ_BMS_TIME = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x781,
                                    .data = {0x03, 0x22, 0xB0, 0x6D, 0x00, 0x00, 0x00, 0x00}};  //  Battery Management System Time


  CAN_frame MG_HS_8A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x08A,
                        .data = {0x80, 0x00, 0x00, 0x04, 0x00, 0x02, 0x36, 0xB0}};

  CAN_frame MG_HS_1F1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1F1,
                         .data = {0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


                                    //Setup Fast UDS values to poll for
  //CAN_frame* UDS_REQUESTS_FAST[0] = {};
  //int numFastUDSreqs =
  //    sizeof(UDS_REQUESTS_FAST) / sizeof(UDS_REQUESTS_FAST[0]);  //Store Number of elements in the array

  //Setup Slow UDS values to poll for
  CAN_frame* UDS_REQUESTS_SLOW[15] = {
                                     &MG5_781_RQ_BUS_VOLTAGE,
                                     &MG5_781_RQ_BAT_VOLTAGE,
                                     &MG5_781_RQ_BAT_CURRENT,
                                     &MG5_781_RQ_BAT_RESISTANCE,
                                     &MG5_781_RQ_BAT_SOC,
                                     &MG5_781_RQ_BMS_ERR,
                                     &MG5_781_RQ_BMS_STATE,
                                     &MG5_781_RQ_BAT_RELAY_B,
                                     &MG5_781_RQ_BAT_RELAY_G,
                                     &MG5_781_RQ_BAT_RELAY_P,
                                     &MG5_781_RQ_BAT_TEMP,
                                     &MG5_781_RQ_MAX_CELL,
                                     &MG5_781_RQ_MIN_CELL,
                                     &MG5_781_RQ_COOLANT_TEMP,
                                     &MG5_781_RQ_BAT_SOH
                                     //&MG5_781_RQ_DTCs
                                    };
  int numSlowUDSreqs =
      sizeof(UDS_REQUESTS_SLOW) / sizeof(UDS_REQUESTS_SLOW[0]);  // Store Number of elements in the array


  //0x3ac: Battery summary
  uint16_t battery_soc = 0;
  uint16_t battery_current = 0;
  uint16_t battery_voltage = 0;  
  uint16_t battery_BMS_state = 0;

  // poll counters
  int uds_fast_req_id_counter = -1;
  int uds_slow_req_id_counter = -1;

  bool uds_tx_in_flight = false;
  unsigned long uds_req_started_ms = 0;
  unsigned long uds_timeout_ms  = 0;
  const unsigned long UDS_PID_REFRESH_MS = 800;     // inter-request gap
  const unsigned long UDS_TIMEOUT_BEFORE_FF_MS = 1000;   // no reply yet
  const unsigned long UDS_TIMEOUT_AFTER_FF_MS  = 3000;  // multi-frame in progress
  const unsigned long UDS_TIMEOUT_AFTER_BOOT = 2000;   // DELAY TO START UDS AFTER BOOT-UP
  const unsigned long TESTER_PRESENT_PERIOD_MS = 1000;  // ~1 s
  const uint32_t UDS_DTC_REFRESH_MS = 10000; // or 60000

  // tiny helper for cycling request indices
  static int increment_uds_req_id_counter(int cur, int n) {
    if (n <= 0) return -1;
    cur++;
    if (cur >= n) cur = 0;
    return cur;
  }
};


#endif
