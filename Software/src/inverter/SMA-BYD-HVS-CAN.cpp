#include "SMA-BYD-HVS-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/* TODO: Map error bits in 0x158 */

/* Do not change code below unless you are sure what you are doing */

void SmaBydHvsInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the inverter CAN
  // Update values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    ampere_hours_remaining =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) *
         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
  }

  //Map values to CAN messages

  //Maxvoltage (eg 400.0V = 4000 , 16bits long)
  SMA_358.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SMA_358.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  //Minvoltage (eg 300.0V = 3000 , 16bits long)
  SMA_358.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  SMA_358.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  //Discharge limited current, 500 = 50A, (0.1, A)
  SMA_358.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  SMA_358.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Charge limited current, 125 =12.5A (0.1, A)
  SMA_358.data.u8[6] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SMA_358.data.u8[7] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

  //SOC (100.00%)
  SMA_3D8.data.u8[0] = (datalayer.battery.status.reported_soc >> 8);
  SMA_3D8.data.u8[1] = (datalayer.battery.status.reported_soc & 0x00FF);
  //StateOfHealth (100.00%)
  SMA_3D8.data.u8[2] = (datalayer.battery.status.soh_pptt >> 8);
  SMA_3D8.data.u8[3] = (datalayer.battery.status.soh_pptt & 0x00FF);
  //State of charge (AH, 0.1)
  SMA_3D8.data.u8[4] = (ampere_hours_remaining >> 8);
  SMA_3D8.data.u8[5] = (ampere_hours_remaining & 0x00FF);

  //Voltage (370.0)
  SMA_4D8.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  SMA_4D8.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Current (TODO: signed OK?)
  SMA_4D8.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  SMA_4D8.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Temperature average
  SMA_4D8.data.u8[4] = (temperature_average >> 8);
  SMA_4D8.data.u8[5] = (temperature_average & 0x00FF);
  //Battery ready
  if (datalayer.battery.status.bms_status == ACTIVE) {
    SMA_4D8.data.u8[6] = READY_STATE;
  } else {
    SMA_4D8.data.u8[6] = STOP_STATE;
  }

  //Highest battery temperature
  SMA_518.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  SMA_518.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //Lowest battery temperature
  SMA_518.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  SMA_518.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  //Sum of all cellvoltages
  SMA_518.data.u8[4] = (datalayer.battery.status.voltage_dV >> 8);
  SMA_518.data.u8[5] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Cell min/max voltage (mV / 25)
  SMA_518.data.u8[6] = (datalayer.battery.status.cell_min_voltage_mV / 25);
  SMA_518.data.u8[7] = (datalayer.battery.status.cell_max_voltage_mV / 25);

  //Lifetime charged energy amount
  SMA_458.data.u8[0] = (datalayer.battery.status.total_charged_battery_Wh & 0xFF000000) >> 24;
  SMA_458.data.u8[1] = (datalayer.battery.status.total_charged_battery_Wh & 0x00FF0000) >> 16;
  SMA_458.data.u8[2] = (datalayer.battery.status.total_charged_battery_Wh & 0x0000FF00) >> 8;
  SMA_458.data.u8[3] = (datalayer.battery.status.total_charged_battery_Wh & 0x000000FF);
  //Lifetime discharged energy amount
  SMA_458.data.u8[4] = (datalayer.battery.status.total_discharged_battery_Wh & 0xFF000000) >> 24;
  SMA_458.data.u8[5] = (datalayer.battery.status.total_discharged_battery_Wh & 0x00FF0000) >> 16;
  SMA_458.data.u8[6] = (datalayer.battery.status.total_discharged_battery_Wh & 0x0000FF00) >> 8;
  SMA_458.data.u8[7] = (datalayer.battery.status.total_discharged_battery_Wh & 0x000000FF);

  //Error bits
  if (datalayer.system.status.battery_allows_contactor_closing) {
    SMA_158.data.u8[2] = 0xAA;
  } else {
    SMA_158.data.u8[2] = 0x6A;
  }

  control_contactor_led();

  // Check if Enable line is working. If we go too long without any input, raise an event
  if (!datalayer.system.status.inverter_allows_contactor_closing) {
    timeWithoutInverterAllowsContactorClosing++;

    if (timeWithoutInverterAllowsContactorClosing > THIRTY_MINUTES) {
      set_event(EVENT_NO_ENABLE_DETECTED, 0);
    }
  } else {
    timeWithoutInverterAllowsContactorClosing = 0;
  }

  /*
  //SMA_158.data.u8[0] = //bit12 Fault high temperature, bit34Battery cellundervoltage, bit56 Battery cell overvoltage, bit78 batterysystemdefect
  //TODO: add all error bits. Sending message with all 0xAA until that.

  0x158 can be used to send error messages or warnings.

  Each message is defined of two bits:  
  01=message triggered  
  10=no message triggered  
  0xA9=10101001, triggers first message  
  0xA6=10100110, triggers second message  
  0x9A=10011010, triggers third message  
  0x6A=01101010, triggers forth message  
  bX defines the byte

  b0 A9   Battery system defect
  b0 A6   Battery cell overvoltage fault
  b0 9A   Battery cell undervoltage fault
  b0 6A   Battery high temperature fault
  b1 A9   Battery low temperature fault
  b1 A6   Battery high temperature fault
  b1 9A   Battery low temperature fault
  b1 6A   Overload (reboot required)
  b2 A9   Overload (reboot required)
  b2 A6   Incorrect switch position for the battery disconnection point
  b2 9A   Battery system short circuit
  b2 6A   Internal battery hardware fault
  b3 A9   Battery imbalancing fault
  b3 A6   Battery service life expiry
  b3 9A   Battery system thermal management defective
  b3 6A   Internal battery hardware fault
  b4 A9   Battery system defect (warning)
  b4 A6   Battery cell overvoltage fault (warning)
  b4 9A   Battery cell undervoltage fault (warning)
  b4 6A   Battery high temperature fault (warning)
  b5 A9   Battery low temperature fault (warning)
  b5 A6   Battery high temperature fault (warning)
  b5 9A   Battery low temperature fault (warning)
  b5 6A   Self-diagnosis (warning)
  b6 A9   Self-diagnosis (warning)
  b6 A6   Incorrect switch position for the battery disconnection point (warning)
  b6 9A   Battery system short circuit (warning)
  b6 6A   Internal battery hardware fault (warning)
  b7 A9   Battery imbalancing fault (warning)
  b7 A6   Battery service life expiry (warning)
  b7 9A   Battery system thermal management defective (warning)
  b7 6A   Internal battery hardware fault (warning)

*/
}

void SmaBydHvsInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x360:  //Message originating from SMA inverter - Voltage and current
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_voltage = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      inverter_current = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;
    case 0x3E0:  //Message originating from SMA inverter - ?
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x420:  //Message originating from SMA inverter - Timestamp
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_time =
          (rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;
    case 0x560:  //Message originating from SMA inverter - Init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x561:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x562:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x563:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x564:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x565:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x566:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x567:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E0:  //Message originating from SMA inverter - String
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Inverter brand (frame1-3 = 0x53 0x4D 0x41) = SMA
      break;
    case 0x5E1:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E2:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E3:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E4:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E5:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E6:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E7:  //Message originating from SMA inverter - Pairing request
    case 0x660:  //Message originating from SMA inverter - Pairing request
      logging.println("Received SMA pairing request");
      pairing_events++;
      set_event(EVENT_SMA_PAIRING, pairing_events);
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can_init = true;
      break;
    case 0x62C:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void SmaBydHvsInverter::transmit_can(unsigned long currentMillis) {

  if (transmit_can_init) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisBatch >= delay_between_batches_ms) {
      previousMillisBatch = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (batch_send_index) {
        case 0:
          transmit_can_frame(&SMA_558);
          transmit_can_frame(&SMA_598);
          transmit_can_frame(&SMA_5D8);
          break;
        case 1:
          transmit_can_frame(&SMA_618_1);
          transmit_can_frame(&SMA_618_2);
          transmit_can_frame(&SMA_618_3);
          break;
        case 2:
          transmit_can_frame(&SMA_158);
          transmit_can_frame(&SMA_358);
          transmit_can_frame(&SMA_3D8);
          break;
        case 3:
          transmit_can_frame(&SMA_458);
          transmit_can_frame(&SMA_518);
          transmit_can_frame(&SMA_4D8);
          transmit_can_init = false;
          break;
        default:
          break;
      }

      // Increment message index and wrap around if needed
      batch_send_index++;

      if (transmit_can_init == false) {  //We completed sending the batches
        batch_send_index = 0;
      }
    }
  }

  // Send CAN Message every 100ms if inverter allows contactor closing
  if (datalayer.system.status.inverter_allows_contactor_closing) {
    if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
      previousMillis100ms = currentMillis;
      transmit_can_frame(&SMA_158);
      transmit_can_frame(&SMA_358);
      transmit_can_frame(&SMA_3D8);
      transmit_can_frame(&SMA_458);
      transmit_can_frame(&SMA_518);
      transmit_can_frame(&SMA_4D8);
    }
    // Send CAN Message every 60s (potentially SMA_458 is not required for stable operation)
    if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
      previousMillis60s = currentMillis;
      transmit_can_frame(&SMA_458);
    }
  }
}
