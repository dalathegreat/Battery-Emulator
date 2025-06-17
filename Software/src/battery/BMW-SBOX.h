#ifndef BMW_SBOX_CONTROL_H
#define BMW_SBOX_CONTROL_H
#include "../include.h"

#ifdef BMW_SBOX
#define SELECTED_SHUNT_CLASS BmwSbox
#endif

#include "Shunt.h"

class BmwSbox : public CanShunt {
 public:
  void setup();
  void transmit_can(unsigned long currentMillis);
  void handle_incoming_can_frame(CAN_frame rx_frame);
  static constexpr char* Name = "BMW SBOX";

 private:
  /** Minimum input voltage required to enable relay control **/
  static const int MINIMUM_INPUT_VOLTAGE = 250;

  /** Minimum required percentage of input voltage at the output port to engage the positive relay. **/
  /** Prevents engagement of the positive contactor if the precharge resistor is faulty. **/
  static constexpr const double MAX_PRECHARGE_RESISTOR_VOLTAGE_PERCENT = 0.99;

  /* NOTE: modify the T2 time constant below to account for the resistance and capacitance of the target system.
 *      t=3RC at minimum, t=5RC ideally
 */

  static const int CONTACTOR_CONTROL_T1 = 5000;  // Time before negative contactor engages and precharging starts
  static const int CONTACTOR_CONTROL_T2 =
      5000;  // Precharge time before precharge resistor is bypassed by positive contactor
  static const int CONTACTOR_CONTROL_T3 = 2000;  // Precharge relay lead time after positive contactor has been engaged

  static const int MAX_ALLOWED_FAULT_TICKS = 1000;

  enum SboxState { DISCONNECTED, PRECHARGE, NEGATIVE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
  SboxState contactorStatus = DISCONNECTED;

  unsigned long prechargeStartTime = 0;
  unsigned long negativeStartTime = 0;
  unsigned long positiveStartTime = 0;
  unsigned long timeSpentInFaultedMode = 0;
  unsigned long LastMsgTime = 0;  // will store last time a 20ms CAN Message was send
  unsigned long LastAvgTime = 0;  // Last current storage time
  unsigned long ShuntLastSeen = 0;

  uint32_t avg_mA_array[10];
  uint32_t avg_sum;

  uint8_t k;  //avg array pointer

  uint8_t CAN100_cnt = 0;

  CAN_frame SBOX_100 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 4,
                        .ID = 0x100,
                        .data = {0x55, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
                                 0x00}};  // Byte 0: relay control, Byte 1: counter 0-E, Byte 4: CRC

  CAN_frame SBOX_300 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 4,
                        .ID = 0x300,
                        .data = {0xFF, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00}};  // Static frame
};

#endif
