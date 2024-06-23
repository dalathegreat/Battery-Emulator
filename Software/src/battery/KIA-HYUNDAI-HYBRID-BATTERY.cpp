#include "../include.h"
#ifdef KIA_HYUNDAI_HYBRID_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "KIA-HYUNDAI-HYBRID-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was send
static unsigned long previousMillis1000 = 0;  // will store last time a 100ms CAN Message was send

static uint16_t SOC = 0;
static bool interlock_missing = false;
static uint16_t battery_current = 0;
static uint16_t voltage = 0;
static int8_t temperature = 0;
static uint8_t poll_data_pid = 0;

CAN_frame_t KIA_7E4_id1 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x7E4,
                           .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t KIA_7E4_id2 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x7E4,
                           .data = {0x02, 0x21, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t KIA_7E4_id3 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x7E4,
                           .data = {0x02, 0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t KIA_7E4_id5 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x7E4,
                           .data = {0x02, 0x21, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t KIA_7E4_ack = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x7E4,  //Ack frame, correct PID is returned. Flow control message
                           .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = SOC * 100;

  datalayer.battery.status.voltage_dV = voltage * 10;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.active_power_W;

  datalayer.battery.status.temperature_min_dC = temperature * 10;

  datalayer.battery.status.temperature_max_dC = temperature * 10;

  if (interlock_missing) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.MsgID) {
    case 0x5F1:
      SOC = rx_frame.data.u8[2];  //Best guess so far, could be very wrong
      break;
    case 0x51E:
      break;
    case 0x588:
      break;
    case 0x5AE:
      if (rx_frame.data.u8[0] > 100) {
        voltage = rx_frame.data.u8[0];
      }
      interlock_missing = (bool)(rx_frame.data.u8[1] & 0x02) >> 1;
      temperature = rx_frame.data.u8[3];  // Is this correct byte? Or is it 5AD 4/5? The one there is 1deg higher
      break;
    case 0x5AF:
      break;
    case 0x5AD:
      battery_current = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[2];
      break;
    case 0x670:
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[3] == poll_data_pid) {
            ESP32Can.CANWriteFrame(&KIA_7E4_ack);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {

          } else if (poll_data_pid == 2) {

          } else if (poll_data_pid == 3) {

          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 1) {

          } else if (poll_data_pid == 2) {

          } else if (poll_data_pid == 3) {

          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {

          } else if (poll_data_pid == 2) {

          } else if (poll_data_pid == 3) {

          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {

          } else if (poll_data_pid == 2) {

          } else if (poll_data_pid == 3) {

          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 1) {

          } else if (poll_data_pid == 2) {

          } else if (poll_data_pid == 3) {

          } else if (poll_data_pid == 5) {
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          break;
        case 0x27:  //Seventh datarow in PID group
          break;
        case 0x28:  //Eighth datarow in PID group
          break;
      }
      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();
  //Send 10ms message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    }
    previousMillis10 = currentMillis;
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //PID data is polled after last message sent from battery:
    if (poll_data_pid >= 5) {  //polling one of 5 PIDs at 100ms, resolution = 500ms
      poll_data_pid = 0;
    }
    poll_data_pid++;
    if (poll_data_pid == 1) {
      ESP32Can.CANWriteFrame(&KIA_7E4_id1);
    } else if (poll_data_pid == 2) {
      ESP32Can.CANWriteFrame(&KIA_7E4_id2);
    } else if (poll_data_pid == 3) {
      ESP32Can.CANWriteFrame(&KIA_7E4_id3);
    } else if (poll_data_pid == 4) {

    } else if (poll_data_pid == 5) {
      ESP32Can.CANWriteFrame(&KIA_7E4_id5);
    }
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Kia/Hyundai Hybrid battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;  //TODO: Values?
  datalayer.battery.info.min_design_voltage_dV = 1000;
}

#endif
