#include "MAXUS-EV80-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/* TODO: 
- CAN log from battery needed!!!
- CAN log from battery running this code needed!!! So we can confirm the PID poll
- Get contactor closing working
- Figure out which CAN messages need to be sent towards the battery to keep it alive
- Map all values from battery CAN messages
- OBD2 integration started
*/

void MaxusEV80Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //datalayer.battery.status.real_soc;

  //datalayer.battery.status.voltage_dV;

  //datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh = 56000;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //datalayer.battery.status.max_discharge_power_W;

  //datalayer.battery.status.max_charge_power_W;

  //datalayer.battery.status.temperature_min_dC;

  //datalayer.battery.status.temperature_max_dC;
}

void MaxusEV80Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x7C8:  //Poll reply from BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if (rx_frame.data.u8[0] == 0x10) {  //Multiframe response, send ACK
        transmit_can_frame(&MAXUS_ACK);
        //Multiframe has the poll reply slightly different location (TODO, not confirmed for Maxus)
        incoming_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      }

      if (rx_frame.data.u8[0] < 0x10) {  //One line responses
        incoming_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

        switch (incoming_poll) {  //One line responses
          case POLL_BMS_STATE:
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_CELLVOLTAGE:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_CELLTEMP:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_SOH:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_SOC:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_SOC_RAW:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_CURRENT_SENSOR:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_CCS:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_SAC:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_UNKNOWN1:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          case POLL_UNKNOWN2:  //TODO: Not confirmed if multiframe or one line response
            //todo = (rx_frame.data.u8[4]);
            break;
          default:
            break;
        }
      }

      /*
        switch (incoming_poll)  //Multiframe responses
        {
          case PID_EXAMPLE_MULTIFRAME:
            switch (rx_frame.data.u8[0]) {
              case 0x10:
                pid_SOH_cell_1 = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
                break;
              case 0x21:
                break;
              case 0x22:
                break;
              case 0x23:
                break;
              default:
                break;
            }
            break;
          default:
            //Not a multiframe response, do nothing
            break;
        }
            */
      break;
    default:
      break;
  }
}
void MaxusEV80Battery::transmit_can(unsigned long currentMillis) {
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (poll_state) {
      case POLL_BMS_STATE:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_BMS_STATE & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_BMS_STATE & 0x00FF);
        poll_state = POLL_CELLVOLTAGE;
        break;
      case POLL_CELLVOLTAGE:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_CELLVOLTAGE & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_CELLVOLTAGE & 0x00FF);
        poll_state = POLL_CELLTEMP;
        break;
      case POLL_CELLTEMP:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_CELLTEMP & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_CELLTEMP & 0x00FF);
        poll_state = POLL_SOH;
        break;
      case POLL_SOH:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_SOH & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_SOH & 0x00FF);
        poll_state = POLL_SOC;
        break;
      case POLL_SOC:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_SOC & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_SOC & 0x00FF);
        poll_state = POLL_SOC_RAW;
        break;
      case POLL_SOC_RAW:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_SOC_RAW & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_SOC_RAW & 0x00FF);
        poll_state = POLL_CURRENT_SENSOR;
        break;
      case POLL_CURRENT_SENSOR:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_CURRENT_SENSOR & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_CURRENT_SENSOR & 0x00FF);
        poll_state = POLL_CCS;
        break;
      case POLL_CCS:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_CCS & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_CCS & 0x00FF);
        poll_state = POLL_SAC;
        break;
      case POLL_SAC:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_SAC & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_SAC & 0x00FF);
        poll_state = POLL_UNKNOWN1;
        break;
      case POLL_UNKNOWN1:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_UNKNOWN1 & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_UNKNOWN1 & 0x00FF);
        poll_state = POLL_UNKNOWN2;
        break;
      case POLL_UNKNOWN2:
        MAXUS_POLL.data.u8[2] = (uint8_t)((POLL_UNKNOWN2 & 0xFF00) >> 8);
        MAXUS_POLL.data.u8[3] = (uint8_t)(POLL_UNKNOWN2 & 0x00FF);
        poll_state = POLL_BMS_STATE;
        break;
      default:
        //We should never end up here. Reset poll_state to first poll
        poll_state = POLL_BMS_STATE;
        break;
    }

    transmit_can_frame(&MAXUS_POLL);
  }
}

void MaxusEV80Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
