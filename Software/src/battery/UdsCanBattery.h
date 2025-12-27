#ifndef UDS_CAN_BATTERY_H
#define UDS_CAN_BATTERY_H

#include "CanBattery.h"

class UdsCanBattery : public CanBattery {
 public:
  virtual void transmit_uds_can(unsigned long currentMillis);
  virtual bool handle_incoming_uds_can_frame(CAN_frame rx_frame);
  virtual bool supports_reset_DTC();
  virtual void reset_DTC();

  void startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID);
  bool storeUDSPayload(const uint8_t* payload, uint8_t length);
  bool isUDSMessageComplete();
  virtual void print_formatted_dtc(uint32_t dtc24, uint8_t status);

  uint32_t previousUdsMillis200 = 0;
  uint16_t next_request_id = 0;
  uint16_t uds_busy_timeout = 0;

  uint16_t successful_uds_request_id = 0;
  bool request_clear_dtc = false;

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

#endif
