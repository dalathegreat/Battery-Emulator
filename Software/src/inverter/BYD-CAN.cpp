#include "BYD-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "INVERTERS.h"

/* Do not change code below unless you are sure what you are doing */

void BydCanInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  /* Calculate temperature */
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  /* Calculate capacity, Amp hours(Ah) = Watt hours (Wh) / Voltage (V)*/
  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    remaining_capacity_ah =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
    fully_charged_capacity_ah =
        ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
  }

  //Map values to CAN messages
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    //Target charge voltage (eg 400.0V = 4000 , 16bits long)
    BYD_110.data.u8[0] = (datalayer.battery.settings.max_user_set_charge_voltage_dV >> 8);
    BYD_110.data.u8[1] = (datalayer.battery.settings.max_user_set_charge_voltage_dV & 0x00FF);
    //Target discharge voltage (eg 300.0V = 3000 , 16bits long)
    BYD_110.data.u8[2] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV >> 8);
    BYD_110.data.u8[3] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV & 0x00FF);
  } else {  //Use the voltage based on battery reported design voltage +- offset to avoid triggering events
    //Target charge voltage (eg 400.0V = 4000 , 16bits long)
    BYD_110.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) >> 8);
    BYD_110.data.u8[1] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) & 0x00FF);
    //Target discharge voltage (eg 300.0V = 3000 , 16bits long)
    BYD_110.data.u8[2] = ((datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV) >> 8);
    BYD_110.data.u8[3] = ((datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV) & 0x00FF);
  }

  //Maximum discharge power allowed (Unit: A+1)
  BYD_110.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  BYD_110.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Maximum charge power allowed (Unit: A+1)
  BYD_110.data.u8[6] = (datalayer.battery.status.max_charge_current_dA >> 8);
  BYD_110.data.u8[7] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

  //SOC (100.00%)
  BYD_150.data.u8[0] = (datalayer.battery.status.reported_soc >> 8);
  BYD_150.data.u8[1] = (datalayer.battery.status.reported_soc & 0x00FF);
  if (user_selected_inverter_deye_workaround) {
    // Fix for avoiding offgrid Deye inverters to underdischarge batteries
    if (datalayer.battery.status.max_charge_current_dA == 0) {
      //Force to 100.00% incase battery no longer wants to charge
      BYD_150.data.u8[0] = (10000 >> 8);
      BYD_150.data.u8[1] = (10000 & 0x00FF);
    }
    if (datalayer.battery.status.max_discharge_current_dA == 0) {
      //Force to 0% incase battery no longer wants to discharge
      BYD_150.data.u8[0] = 0;
      BYD_150.data.u8[1] = 0;
    }
  }
  //StateOfHealth (100.00%)
  BYD_150.data.u8[2] = (datalayer.battery.status.soh_pptt >> 8);
  BYD_150.data.u8[3] = (datalayer.battery.status.soh_pptt & 0x00FF);
  //Remaining capacity (Ah+1)
  BYD_150.data.u8[4] = (remaining_capacity_ah >> 8);
  BYD_150.data.u8[5] = (remaining_capacity_ah & 0x00FF);
  //Fully charged capacity (Ah+1)
  BYD_150.data.u8[6] = (fully_charged_capacity_ah >> 8);
  BYD_150.data.u8[7] = (fully_charged_capacity_ah & 0x00FF);

  //Alarms
  //TODO: BYD Alarms are not implemented yet. Investigation needed on the bits in this message
  //BYD_190.data.u8[0] =

  //Voltage (ex 370.0)
  BYD_1D0.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  BYD_1D0.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Current (ex 81.0A)
  BYD_1D0.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  BYD_1D0.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Temperature average
  BYD_1D0.data.u8[4] = (temperature_average >> 8);
  BYD_1D0.data.u8[5] = (temperature_average & 0x00FF);

  //Temperature max
  BYD_210.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  BYD_210.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //Temperature min
  BYD_210.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  BYD_210.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  //Capacity
  BYD_250.data.u8[4] = (uint8_t)((datalayer.battery.info.reported_total_capacity_Wh / 100) >> 8);
  BYD_250.data.u8[5] = (uint8_t)(datalayer.battery.info.reported_total_capacity_Wh / 100);
}

void BydCanInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x151:  //Message originating from BYD HVS compatible inverter. Reply with CAN identifier!
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] & 0x01) {  //Battery requests identification
        send_initial_data();
      } else {  // We can identify what inverter type we are connected to
        for (uint8_t i = 0; i < 7; i++) {
          datalayer.system.info.inverter_brand[i] = rx_frame.data.u8[i + 1];
        }
        datalayer.system.info.inverter_brand[7] = '\0';
      }
      break;
    case 0x091:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_voltage = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) * 0.1;
      inverter_current = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) * 0.1;
      inverter_temperature = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.1;
      break;
    case 0x0D1:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_SOC = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) * 0.1;
      break;
    case 0x111:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_timestamp = ((rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) |
                            rx_frame.data.u8[3]);
      break;
    default:
      break;
  }
}

void BydCanInverter::transmit_can(unsigned long currentMillis) {

  if (!inverterStartedUp) {
    //Avoid sending messages towards inverter, unless it has woken up and sent something to us first
    return;
  }

  // Send initial CAN data once on bootup
  if (!initialDataSent) {
    send_initial_data();
    initialDataSent = true;
  }

  // Send 2s CAN Message
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;

    transmit_can_frame(&BYD_110);  //Send Limits
  }
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;

    transmit_can_frame(&BYD_150);  //Send States
    transmit_can_frame(&BYD_1D0);  //Send Battery Info
    transmit_can_frame(&BYD_210);  //Send Cell Info
  }
  //Send 60s message
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    transmit_can_frame(&BYD_190);  //Send Alarm
  }
}

void BydCanInverter::send_initial_data() {
  transmit_can_frame(&BYD_250);
  transmit_can_frame(&BYD_290);
  transmit_can_frame(&BYD_2D0);
  BYD_3D0.data = {0x00, 0x42, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79};  //Battery
  transmit_can_frame(&BYD_3D0);
  BYD_3D0.data = {0x01, 0x2D, 0x42, 0x6F, 0x78, 0x20, 0x50, 0x72};  //-Box Pr
  transmit_can_frame(&BYD_3D0);
  BYD_3D0.data = {0x02, 0x65, 0x6D, 0x69, 0x75, 0x6D, 0x20, 0x48};  //emium H
  transmit_can_frame(&BYD_3D0);
  BYD_3D0.data = {0x03, 0x56, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00};  //VS
  transmit_can_frame(&BYD_3D0);
}
