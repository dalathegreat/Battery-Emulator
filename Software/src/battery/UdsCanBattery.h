#ifndef UDS_CAN_BATTERY_H
#define UDS_CAN_BATTERY_H

#include "CanBattery.h"

// Extend this class to add UDS features to a battery integration.

// 1. Call setup_uds(uint16_t obd_address, uint16_t first_pid) in your battery's
//    setup() function to initialize UDS handling.
//     - obd_address (the CAN ID of the ECU to query, e.g. 0x7DF for generic
//       requests)
//     - first_pid (the first PID to request, e.g. 0xB042 for MG HS PHEV battery
//       voltage)
//
// 2. Call transmit_uds_can(unsigned long currentMillis) in your battery's
//    transmit_can() function to send UDS requests periodically.
//
// 3. Call handle_incoming_uds_can_frame(CAN_frame rx_frame) in your battery's
//    handle_incoming_can_frame(CAN_frame rx_frame) function to process incoming
//    UDS responses. If it returns true, the frame was handled as a UDS
//    response.
//
// 4. Override handle_pid(uint16_t pid, uint32_t value) to be passed PID query
//    responses. You should return the next PID to query, or 0 to reset the cycle.

class UdsCanBattery : public CanBattery {
 public:
  enum class UdsStatus : uint8_t {
    OK = 0,
    OK_SHORT,
    TIMEOUT,
    NEGATIVE_RESPONSE,  // The ECU returned an NRC (e.g., 0x7F)
  };

  void setup_uds(uint16_t obd_address, uint32_t first_pid = 0);
  void transmit_uds_can(unsigned long currentMillis);
  bool handle_incoming_uds_can_frame(CAN_frame rx_frame);
  // Temporarily pause UDS requests for the specified number of 200ms ticks.
  void pause_uds(uint16_t ticks_200ms) { uds_busy_timeout = ticks_200ms; }
  // If you let UdsCanBattery handle UDS responses, you can override this be
  // passed the PID query responses. The value returned is used as the next PID
  // to query. Return 0 to let the PID cycle continue as normal.
  virtual uint32_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length, UdsStatus status) {
    return 0;
  }
  //virtual uint32_t handle_long_pid(uint16_t pid, const uint8_t* data, uint16_t length) { return 0; }
  virtual bool supports_read_DTC();
  virtual bool supports_reset_DTC();
  virtual void read_DTC();
  virtual void reset_DTC();

  void startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID);
  bool storeUDSPayload(const uint8_t* payload, uint8_t length);
  bool isUDSMessageComplete();
  virtual void print_formatted_dtc(uint32_t dtc24, uint8_t status);

  // The range of response IDs (addresses) we'll accept UDS responses from.
  static const uint16_t MIN_UDS_RESPONSE_ID = 0x780;
  static const uint16_t MAX_UDS_RESPONSE_ID = 0x7EF;

  static const uint32_t SHORT_PID = 0x10000;

  uint32_t previousUdsMillis200 = 0;
  uint32_t first_pid = 0;
  uint32_t next_pid = 0;
  uint16_t uds_busy_timeout = 0;
  uint16_t uds_address = 0x7DF;

  bool user_request_read_dtc = false;
  bool user_request_clear_dtc = false;

  // A structure to keep track of the ongoing multi-frame UDS response
  typedef struct {
    bool UDS_inProgress;                // Are we currently receiving a multi-frame message?
    uint16_t UDS_expectedLength;        // Expected total payload length
    uint16_t UDS_bytesReceived;         // How many bytes have been stored so far
    uint8_t UDS_moduleID;               // The "module" indicated by the first frame
    uint8_t receivedInBatch;            // Number of CFs received in the current batch
    uint8_t UDS_buffer[1024];           // Buffer for the reassembled data
    unsigned long UDS_lastFrameMillis;  // Timestamp of last frame (for timeouts, if desired)
  } UDS_RxContext;

  // A single global UDS context, since only one module can respond at a time
  UDS_RxContext gUDSContext;

  CAN_frame UDS_PID_REQUEST = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E5,
                               .data = {0x03, 0x22, 0xB0, 0x42, 0x00, 0x00, 0x00, 0x00}};

  //0x781 UDS diagnostic requests - request all DTC's
  CAN_frame UDS_RQ_DTCs = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x781,
                           .data = {0x03, 0x19, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00}};

  //0x781 UDS diagnostic requests - clear all DTC's
  CAN_frame UDS_CLEAR_DTCs = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x781,
                              .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};

  CAN_frame UDS_RQ_CONTINUE_MULTIFRAME = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x781,
                                          .data = {0x30, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif  // UDS_CAN_BATTERY_H
