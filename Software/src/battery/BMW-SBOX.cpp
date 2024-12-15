#include "../include.h"
#ifdef BMW_SBOX
#include "../datalayer/datalayer.h"
#include "BMW-SBOX.h"


#define MAX_ALLOWED_FAULT_TICKS 1000
/* NOTE: modify the precharge time constant below to account for the resistance and capacitance of the target system.
 *      t=3RC at minimum, t=5RC ideally
 */

#define PRECHARGE_TIME_MS 160           // Time before negative contactor engages and precharging starts
#define NEGATIVE_CONTACTOR_TIME_MS 1000 // Precharge time before precharge resistor is bypassed by positive contactor
#define POSITIVE_CONTACTOR_TIME_MS 2000 // Precharge relay lead time after positive contactor has been engaged

enum State { DISCONNECTED, PRECHARGE, NEGATIVE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
State contactorStatus = DISCONNECTED;

unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;
unsigned long positiveStartTime = 0;
unsigned long timeSpentInFaultedMode = 0;
unsigned long LastMsgTime = 0;  // will store last time a 20ms CAN Message was send

uint8_t CAN100_cnt=0;

CAN_frame SBOX_100 = {.FD = false,
  .ext_ID = false,
  .DLC = 4,
  .ID = 0x100,
  .data = {0x55, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00}};   // Byte 0: relay control, Byte 1: counter 0-E, Byte 4: CRC

CAN_frame SBOX_300 = {.FD = false,
  .ext_ID = false,
  .DLC = 4,
  .ID = 0x300,
  .data = {0xFF, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00}};   // Static frame


uint8_t reverse_bits(uint8_t byte) {
  uint8_t reversed = 0;
  for (int i = 0; i < 8; i++) {
    reversed = (reversed << 1) | (byte & 1);  
    byte >>= 1; 
  }
  return reversed;
}

uint8_t calculateCRC(CAN_frame CAN) {
  uint8_t crc = 0;
  for (size_t i = 0; i < CAN.DLC; i++) {
    uint8_t reversed_byte = reverse_bits(CAN.data.u8[i]);  
    crc ^= reversed_byte;  
    for (int j = 0; j < 8; j++) {
      if (crc & 0x80) {  
        crc = (crc << 1) ^ 0x31;  
      } else {
        crc <<= 1;  
      }
      crc &= 0xFF;  
    }
  }
  crc=reverse_bits(crc);
  return crc;
}

void receive_can_shunt(CAN_frame rx_frame) {
  if(rx_frame.ID ==0x200)
  { 
    datalayer.shunt.measured_amperage_mA=((rx_frame.data.u8[2]<<24)| (rx_frame.data.u8[1]<<16)| (rx_frame.data.u8[0]<<8))/256;
  }
  else if(rx_frame.ID ==0x210) //Battery voltage
  { 
    datalayer.shunt.measured_voltage_mV==((rx_frame.data.u8[2]<<24)| (rx_frame.data.u8[1]<<16)| (rx_frame.data.u8[0]<<8))/256;
  }  
  else if(rx_frame.ID ==0x220) //SBOX output voltage
  { 
    datalayer.shunt.measured_outvoltage_mV==((rx_frame.data.u8[2]<<24)| (rx_frame.data.u8[1]<<16)| (rx_frame.data.u8[0]<<8))/256;
  }
}

void send_can_shunt() {
  // First check if we have any active errors, incase we do, turn off the battery
  if (datalayer.battery.status.bms_status == FAULT) {
    timeSpentInFaultedMode++;
  } else {
    timeSpentInFaultedMode = 0;
  }

  //handle contactor control SHUTDOWN_REQUESTED
  if (timeSpentInFaultedMode > MAX_ALLOWED_FAULT_TICKS) {
    contactorStatus = SHUTDOWN_REQUESTED;
    SBOX_100.data.u8[0]=0x55;  // All open
  }

  if (contactorStatus == SHUTDOWN_REQUESTED) {
    datalayer.shunt.contactors_engaged = false;
    return;  // A fault scenario latches the contactor control. It is not possible to recover without a powercycle (and investigation why fault occured)
  }

  // After that, check if we are OK to start turning on the battery
  if (contactorStatus == DISCONNECTED) {
    datalayer.shunt.contactors_engaged = false;
    SBOX_100.data.u8[0]=0x55;  // All open
    if (datalayer.system.status.battery_allows_contactor_closing &&
        datalayer.system.status.inverter_allows_contactor_closing && !datalayer.system.settings.equipment_stop_active && ( measured_voltage_mV => MINIMUM_INPUT_VOLTAGE*1000)) {
      contactorStatus = PRECHARGE;
    }
  }

  // In case the inverter requests contactors to open, set the state accordingly
  if (contactorStatus == COMPLETED) {
    //Incase inverter (or estop) requests contactors to open, make state machine jump to Disconnected state (recoverable)
    if (!datalayer.system.status.inverter_allows_contactor_closing || datalayer.system.settings.equipment_stop_active) {
      contactorStatus = DISCONNECTED;
    }
  }

  unsigned long currentTime = millis();
  // Handle actual state machine. This first turns on Precharge, then Negative, then Positive, and finally turns OFF precharge
  switch (contactorStatus) {
    case PRECHARGE:
      SBOX_100.data.u8[0]=0x86;          // Precharge relay only
      prechargeStartTime = currentTime;
      contactorStatus = NEGATIVE;
      break;

    case NEGATIVE:
      if (currentTime - prechargeStartTime >= PRECHARGE_TIME_MS) {
        SBOX_100.data.u8[0]=0xA6;          // Precharge + Negative        
        negativeStartTime = currentTime;
        contactorStatus = POSITIVE;
        datalayer.shunt.precharging = true;
      }
      break;

    case POSITIVE:
      if (currentTime - negativeStartTime >= NEGATIVE_CONTACTOR_TIME_MS && (measured_voltage_mV * MAX_PRECHARGE_RESISTOR_VOLTAGE_RATIO < measured_outvoltage_mV)) {
        SBOX_100.data.u8[0]=0xAA;          // Precharge + Negative + Positive       
        positiveStartTime = currentTime;
        contactorStatus = PRECHARGE_OFF;
        datalayer.shunt.precharging = false;
      }
      break;

    case PRECHARGE_OFF:
      if (currentTime - positiveStartTime >= POSITIVE_CONTACTOR_TIME_MS) {
        SBOX_100.data.u8[0]=0x6A;          // Negative + Positive
        contactorStatus = COMPLETED;
        datalayer.shunt.contactors_engaged = true;
      }
      break;
    case COMPLETED:
      SBOX_100.data.u8[0]=0x6A;          // Negative + Positive
    default:
      break;
  }

  // Send 20ms CAN Message
  if (currentTime - LastMsgTime >= INTERVAL_20_MS) {
    LastMsgTime = currentTime;
    CAN100_cnt++;
    if (CAN100_cnt>0x0E) {
      CAN100_cnt=0;
    }    
    SBOX_100.data.u8[1]=CAN100_cnt<<4|0x01;
    SBOX_100.data.u8[3]=0x00;
    SBOX_100.data.u8[3]=calculateCRC(SBOX_100);
    transmit_can(&SBOX_100, can_config.shunt);
    transmit_can(&SBOX_300, can_config.shunt);
  }
}

void setup_can_shunt() {
  datalayer.system.info.shunt_protocol[63] = 'BMW SBOX\0';
}
#endif
