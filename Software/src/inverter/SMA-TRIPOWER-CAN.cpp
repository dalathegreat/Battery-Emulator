#include "SMA-TRIPOWER-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/* TODO:
- Figure out the manufacturer info needed in transmit_can_init() CAN messages
  - CAN logs from real system might be needed
- Figure out how cellvoltages need to be displayed
- Figure out if sending transmit_can_init() like we do now is OK
- Figure out how to send the non-cyclic messages when needed
*/

void SmaTripowerInverter::
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
  if (datalayer.battery.status.bms_status == FAULT) {
    SMA_4D8.data.u8[6] = STOP_STATE;
  } else {
    SMA_4D8.data.u8[6] = READY_STATE;
  }

  // Check if Enable line is working. If we go too long without any input, raise an event
  if (!datalayer.system.status.inverter_allows_contactor_closing) {
    timeWithoutInverterAllowsContactorClosing++;

    if (timeWithoutInverterAllowsContactorClosing > THIRTY_MINUTES) {
      set_event(EVENT_NO_ENABLE_DETECTED, 0);
    }
  } else {
    timeWithoutInverterAllowsContactorClosing = 0;
  }
}

void SmaTripowerInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
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
    case 0x5E0:  //Message originating from SMA inverter - String
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Inverter brand (frame1-3 = 0x53 0x4D 0x41) = SMA
      break;
    case 0x660:  //Message originating from SMA inverter - Pairing request
#ifdef DEBUG_LOG
      logging.println("Received 0x660: SMA pairing request");
#endif  // DEBUG_LOG
      pairing_events++;
      set_event(EVENT_SMA_PAIRING, pairing_events);
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can_init();
      break;
    default:
      break;
  }
}

void SmaTripowerInverter::pushFrame(CAN_frame* frame, std::function<void(void)> callback) {
  if (listLength >= 20) {
    return;  //TODO: scream.
  }
  framesToSend[listLength] = {
      .frame = frame,
      .callback = callback,
  };
  listLength++;
}

void SmaTripowerInverter::transmit_can(unsigned long currentMillis) {

  // Send CAN Message only if we're enabled by inverter
  if (!datalayer.system.status.inverter_allows_contactor_closing) {
    return;
  }

  if (listLength > 0 && currentMillis - previousMillis250ms >= INTERVAL_250_MS) {
    previousMillis250ms = currentMillis;
    // Send next frame.
    Frame frame = framesToSend[0];
    transmit_can_frame(frame.frame, can_config.inverter);
    frame.callback();
    for (int i = 0; i < listLength - 1; i++) {
      framesToSend[i] = framesToSend[i + 1];
    }
    listLength--;
  }

  if (!pairing_completed) {
    return;
  }

  // Send CAN Message every 2s
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;
    pushFrame(&SMA_358);
  }
  // Send CAN Message every 10s
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;
    pushFrame(&SMA_518);
    pushFrame(&SMA_4D8);
    pushFrame(&SMA_3D8);
  }
}

void SmaTripowerInverter::completePairing() {
  pairing_completed = true;
}

void SmaTripowerInverter::transmit_can_init() {
  listLength = 0;  // clear all frames

  pushFrame(&SMA_558);    //Pairing start - Vendor
  pushFrame(&SMA_598);    //Serial
  pushFrame(&SMA_5D8);    //BYD
  pushFrame(&SMA_618_0);  //BATTERY
  pushFrame(&SMA_618_1);  //-Box Pr
  pushFrame(&SMA_618_2);  //emium H
  pushFrame(&SMA_618_3);  //VS
  pushFrame(&SMA_358);
  pushFrame(&SMA_3D8);
  pushFrame(&SMA_458);
  pushFrame(&SMA_4D8);
  pushFrame(&SMA_518, [this]() { this->completePairing(); });
}

void SmaTripowerInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
  datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
  pinMode(INVERTER_CONTACTOR_ENABLE_PIN, INPUT);
#ifdef INVERTER_CONTACTOR_ENABLE_LED_PIN
  pinMode(INVERTER_CONTACTOR_ENABLE_LED_PIN, OUTPUT);
  digitalWrite(INVERTER_CONTACTOR_ENABLE_LED_PIN, LOW);  // Turn LED off, until inverter allows contactor closing
#endif                                                   // INVERTER_CONTACTOR_ENABLE_LED_PIN
}
