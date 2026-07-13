#ifndef SMALL_FLASH_DEVICE

#include "CHARGEBYTE-CCS.h"
#include "../datalayer/datalayer.h"
#include "./Shunt.h"

/* Do not change code below unless you are sure what you are doing */
// will store last time a 1s CAN Message was sent

void ChargebyteCCSBattery::setPrechargeVoltage(uint16_t value, bool writeEEPROM) {
  if (!prechargeDacDev)
    return;

  uint8_t packet[3];
  packet[0] = writeEEPROM ? 0x60 : 0x40;
  packet[1] = value >> 4;
  packet[2] = (value & 0x0F) << 4;

  i2c_master_transmit(prechargeDacDev, packet, 3, 100 / portTICK_PERIOD_MS);
}

void ChargebyteCCSBattery::dump_data() {
  logging.print("[CME-CCS] [Status] ");
  logging.print("hasChargebyteError = ");
  logging.print(hasChargebyteError);
  logging.print(", hasLowLevelError = ");
  logging.print(hasLowLevelError);
  logging.print(", inPrecharge = ");
  logging.print(inPrecharge);
  logging.print(", inCharge = ");
  logging.print(inCharge);
  logging.println();

  logging.print("[CME-CCS] [Values] ");
  logging.print("precharge_dV = ");
  logging.print(precharge_dV);
  logging.print(", targetVoltage_dV = ");
  logging.print(targetVoltage_dV);
  logging.print(", targetCurrent_dA = ");
  logging.print(targetCurrent_dA);
  logging.print(", maxVoltage_dV = ");
  logging.print(maxVoltage_dV);
  logging.print(", maxCurrent_dA = ");
  logging.print(maxCurrent_dA);
  logging.print(", maxPower_W = ");
  logging.print(maxPower_W);
  logging.print(", SOC = ");
  logging.print(soc);
  logging.print(", energyCapacity_Wh = ");
  logging.print(energyCapacity_Wh);
  logging.println();
}

// this is called every 10ms. We precharge from 1200 to 4095 in 7.24s which refers to 0 to 500V.
// according to our tests e.g. a Tesla M3 allows 12 seconds of precharging before aborting
void ChargebyteCCSBattery::handle_precharge() {
  static uint16_t prechargePos = 0;
  if (inPrecharge) {
    if (prechargePos < 1200)
      prechargePos = 1200;
    if (prechargePos <= 4090)
      prechargePos += 4;

    setPrechargeVoltage(prechargePos);

    logging.print("[CME-CCS] setting precharge to ");
    logging.println(prechargePos);
  } else if (prechargePos != 0) {
    prechargePos = 0;
    setPrechargeVoltage(0);
  }

  static uint16_t contactorCloseDelay = 0;
  if (inCharge) {
    if (contactorCloseDelay == 100)  // start precharging after 1s
      digitalWrite(CCS_PRECHARGE_CONTACTOR_PIN(), HIGH);
    else if (contactorCloseDelay == 1000)  // fully connect after 10s
      digitalWrite(CCS_MAIN_CONCTACTOR_PIN(), HIGH);
    else if (contactorCloseDelay == 1200)  // turn off precharge contactor to reduce coil consumption
      digitalWrite(CCS_PRECHARGE_CONTACTOR_PIN(), LOW);

    if (contactorCloseDelay <= 1200)
      contactorCloseDelay++;
  } else if (contactorCloseDelay != 0) {
    digitalWrite(CCS_PRECHARGE_CONTACTOR_PIN(), LOW);
    digitalWrite(CCS_MAIN_CONCTACTOR_PIN(), LOW);
    contactorCloseDelay = 0;
  }

  static bool wasInCharge = false;
  static uint16_t cpPpDisconnectDelay = 0;
  if (inCharge) {
    wasInCharge = true;
    cpPpDisconnectDelay = 0;
  } else if (wasInCharge && cpPpDisconnectDelay <= 6200) {
    if (cpPpDisconnectDelay == 0) {
      logging.println("Detected charge abort, waiting for 60 seconds...");
    }
    if (cpPpDisconnectDelay == 6000) {  // disconnect CP and PP after 10s
      logging.println("Detected charge abort, disconnecting CP and PP...");
      digitalWrite(CCS_CP_PP_DISCONNECT_PIN(), HIGH);
    } else if (cpPpDisconnectDelay >= 6199) {  // reconnect after 1.9 more seconds
      logging.println("Recovering from charge abort, reconnecting CP and PP...");
      digitalWrite(CCS_CP_PP_DISCONNECT_PIN(), LOW);
      wasInCharge = false;
      cpPpDisconnectDelay = 0;
    }
    cpPpDisconnectDelay++;
  }
}

void ChargebyteCCSBattery::update_values() {
  dump_data();

  static uint16_t presentVoltage_dV = 0;
  if (inPrecharge && precharge_dV <= EVSE_MAX_VOLTAGE && precharge_dV > 0)
    presentVoltage_dV = precharge_dV;
  else if (inCharge && user_selected_shunt_type != ShuntType::None)
    presentVoltage_dV = datalayer.shunt.measured_voltage_dV;

  int32_t presentCurrent_dA = 0;
  if (inCharge && user_selected_shunt_type != ShuntType::None)
    presentCurrent_dA = datalayer.shunt.measured_amperage_mA / 100;

  CHARGEBYTE_302.data.u8[0] = (presentCurrent_dA + 32500) & 0xff;
  CHARGEBYTE_302.data.u8[1] = (presentCurrent_dA + 32500) >> 8;
  CHARGEBYTE_302.data.u8[2] = presentVoltage_dV & 0xff;
  CHARGEBYTE_302.data.u8[3] = presentVoltage_dV >> 8;

  if (inPrecharge)
    datalayer.battery.status.real_bms_status = BMS_STANDBY;
  else if (inCharge)
    datalayer.battery.status.real_bms_status = BMS_ACTIVE;
  else if (hasLowLevelError || hasChargebyteError)
    datalayer.battery.status.real_bms_status = BMS_FAULT;
  else
    datalayer.battery.status.real_bms_status = BMS_DISCONNECTED;

  datalayer.battery.status.real_soc = soc;
  datalayer.battery.status.voltage_dV = presentVoltage_dV;
  datalayer.battery.status.current_dA = presentCurrent_dA;
  datalayer.battery.status.remaining_capacity_Wh = energyCapacity_Wh * soc / 10000;
  datalayer.battery.info.total_capacity_Wh = energyCapacity_Wh;
  datalayer.battery.info.max_design_voltage_dV = maxVoltage_dV;

  datalayer.battery.status.max_charge_power_W = maxPower_W;
  datalayer.battery.status.max_discharge_power_W = maxPower_W;
}

void ChargebyteCCSBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  bool isChargebyteFrame = true;
  switch (rx_frame.ID) {
    case 0x100:
      if ((rx_frame.data.u8[2] >> 4) >= 3)
        hasLowLevelError = true;
      else
        hasLowLevelError = false;

      inPrecharge = false;
      inCharge = false;
      if ((rx_frame.data.u8[1] & 0xf) == 5)
        inPrecharge = true;
      else if ((rx_frame.data.u8[1] & 0xf) == 6)
        inCharge = true;
      break;
    case 0x102:
      if (rx_frame.data.u8[1] == 0)
        hasChargebyteError = false;
      else
        hasChargebyteError = true;
      break;
    case 0x202:
      soc = rx_frame.data.u8[0] * 50;
      break;
    case 0x201:
      targetCurrent_dA = ((uint16_t)rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];
      targetVoltage_dV = ((uint16_t)rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
      precharge_dV = ((uint16_t)rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
      break;
    case 0x200:
      maxCurrent_dA = ((uint16_t)rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];
      maxPower_W = (((uint32_t)rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 100;
      maxVoltage_dV = ((uint16_t)rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
      break;
    case 0x203:
      energyCapacity_Wh = ((uint16_t)rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];
      break;
    default:
      isChargebyteFrame = false;
      break;
  }

  if (isChargebyteFrame)
    datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void ChargebyteCCSBattery::transmit_can(unsigned long now) {
  static unsigned long previousTransmit = 0;
  // Send 100ms CAN Message
  if (now - previousTransmit >= INTERVAL_100_MS) {
    previousTransmit = now;

    transmit_can_frame(&CHARGEBYTE_300);
    transmit_can_frame(&CHARGEBYTE_301);
    transmit_can_frame(&CHARGEBYTE_302);
    transmit_can_frame(&CHARGEBYTE_303);
    transmit_can_frame(&CHARGEBYTE_304);
    transmit_can_frame(&CHARGEBYTE_305);
    transmit_can_frame(&CHARGEBYTE_306);
    transmit_can_frame(&CHARGEBYTE_307);
  }
}

void ChargebyteCCSBattery::setup() {
  pinMode(CCS_PRECHARGE_CONTACTOR_PIN(), OUTPUT);
  pinMode(CCS_MAIN_CONCTACTOR_PIN(), OUTPUT);
  pinMode(CCS_CP_PP_DISCONNECT_PIN(), OUTPUT);
  digitalWrite(CCS_PRECHARGE_CONTACTOR_PIN(), LOW);
  digitalWrite(CCS_MAIN_CONCTACTOR_PIN(), LOW);
  digitalWrite(CCS_CP_PP_DISCONNECT_PIN(), LOW);

  strncpy(datalayer.system.info.battery_protocol, "CCS using chargebyte CME/CCF", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.min_design_voltage_dV = 1000;
  datalayer.battery.info.max_design_voltage_dV = 5000;
  datalayer.battery.info.max_cell_voltage_mV = 4200;
  datalayer.battery.info.min_cell_voltage_mV = 3200;

  // fake data
  datalayer.battery.info.number_of_cells = 4;
  datalayer.battery.status.cell_voltages_mV[0] = 4000;
  datalayer.battery.status.cell_voltages_mV[1] = 4000;
  datalayer.battery.status.cell_voltages_mV[2] = 4000;
  datalayer.battery.status.cell_voltages_mV[3] = 4000;
  datalayer.battery.status.cell_min_voltage_mV = 4000;
  datalayer.battery.status.cell_max_voltage_mV = 4000;
  datalayer.battery.status.temperature_min_dC = 200;
  datalayer.battery.status.temperature_max_dC = 200;

  i2c_master_bus_config_t bus_config = {
      .i2c_port = -1,
      .sda_io_num = (gpio_num_t)PRECHARGE_DAC_SDA,
      .scl_io_num = (gpio_num_t)PRECHARGE_DAC_SCL,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
      .flags = {.enable_internal_pullup = true},
  };
  if (i2c_new_master_bus(&bus_config, &prechargeI2CBus) != ESP_OK) {
    logging.println("[CME-CCS] Failed initializing I2C bus for DAC");
    return;
  }

  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = PRECHARGE_DAC_ADDRESS,
      .scl_speed_hz = 400000,
      .scl_wait_us = 0,
      .flags = {},
  };
  if (i2c_master_bus_add_device(prechargeI2CBus, &dev_config, &prechargeDacDev) != ESP_OK) {
    logging.println("[CME-CCS] Failed adding DAC device");
    return;
  }

  logging.println("[CME-CCS] successfully initialized I2C DAC");
  setPrechargeVoltage(0, true);
}

#endif
