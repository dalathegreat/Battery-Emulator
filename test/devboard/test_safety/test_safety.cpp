#include <Arduino.h>
#include <src/datalayer/datalayer.h>
#include <src/devboard/safety/safety.h>
#include <src/devboard/utils/events.h>
#include <unity.h>

void setUp(void) {
  reset_all_events();
  inverter = nullptr;
  charger = nullptr;
  battery = nullptr;
  battery2 = nullptr;
  setup_battery();
}

void tearDown(void) {}

// Test CPU temperature protection - normal temperature
void test_update_machineryprotection_cpu_normal_temperature(void) {
  datalayer.system.info.CPU_temperature = 25.0f;
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_CPU_OVERHEATING));
  TEST_ASSERT_FALSE(is_event_set(EVENT_CPU_OVERHEATED));
}

// Test CPU temperature protection - overheating threshold
void test_update_machineryprotection_cpu_overheating_threshold(void) {
  datalayer.system.info.CPU_temperature = 85.0f;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CPU_OVERHEATING));
  TEST_ASSERT_FALSE(is_event_set(EVENT_CPU_OVERHEATED));
}

// Test CPU temperature protection - overheated threshold
void test_update_machineryprotection_cpu_overheated_threshold(void) {
  datalayer.system.info.CPU_temperature = 115.0f;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CPU_OVERHEATING));
  TEST_ASSERT_TRUE(is_event_set(EVENT_CPU_OVERHEATED));
}

// Test CPU temperature protection - hysteresis clearing
void test_update_machineryprotection_cpu_hysteresis_clearing(void) {
  // First trigger overheated
  datalayer.system.info.CPU_temperature = 115.0f;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CPU_OVERHEATED));

  // Then test hysteresis clearing at 103°C
  datalayer.system.info.CPU_temperature = 103.0f;
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_CPU_OVERHEATED));
}

// Test CAN native interface failure
void test_update_machineryprotection_can_native_failure(void) {
  datalayer.system.info.can_native_send_fail = true;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CAN_NATIVE_TX_FAILURE));
  TEST_ASSERT_FALSE(datalayer.system.info.can_native_send_fail);
}

// Test CAN 2515 interface failure
void test_update_machineryprotection_can_2515_failure(void) {
  datalayer.system.info.can_2515_send_fail = true;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CAN_BUFFER_FULL));
  TEST_ASSERT_FALSE(datalayer.system.info.can_2515_send_fail);
}

// Test CAN 2518 (CAN-FD) interface failure
void test_update_machineryprotection_can_2518_failure(void) {
  datalayer.system.info.can_2518_send_fail = true;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CANFD_BUFFER_FULL));
  TEST_ASSERT_FALSE(datalayer.system.info.can_2518_send_fail);
}

// Test battery temperature - normal range
void test_update_machineryprotection_battery_temp_normal(void) {
  datalayer.battery.status.temperature_max_dC = 250;  // 25.0°C
  datalayer.battery.status.temperature_min_dC = 200;  // 20.0°C
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_OVERHEAT));
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_FROZEN));
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_TEMP_DEVIATION_HIGH));
}

// Test battery overheating
void test_update_machineryprotection_battery_overheating(void) {
  datalayer.battery.status.temperature_max_dC = 650;  // 65.0°C (over BATTERY_MAXTEMPERATURE=600)
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_OVERHEAT));
}

// Test battery frozen
void test_update_machineryprotection_battery_frozen(void) {
  datalayer.battery.status.temperature_min_dC = -251;  // -25.1°C (under BATTERY_MINTEMPERATURE)
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_FROZEN));
}

// Test battery temperature deviation
void test_update_machineryprotection_battery_temp_deviation(void) {
  datalayer.battery.status.temperature_max_dC = 400;  // 40.0°C
  datalayer.battery.status.temperature_min_dC = 150;  // 15.0°C (25°C difference > 20°C limit)
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_TEMP_DEVIATION_HIGH));
}

// Test battery voltage - normal range
void test_update_machineryprotection_battery_voltage_normal(void) {
  datalayer.battery.status.voltage_dV = 3800;  // 380.0V (within range)
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_OVERVOLTAGE));
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_UNDERVOLTAGE));
}

// Test battery overvoltage
void test_update_machineryprotection_battery_overvoltage(void) {
  datalayer.battery.status.voltage_dV = 4300;  // 430.0V (over max_design_voltage_dV=4200)
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_OVERVOLTAGE));
  TEST_ASSERT_EQUAL(0, datalayer.battery.status.max_charge_power_W);
}

// Test battery undervoltage
void test_update_machineryprotection_battery_undervoltage(void) {
  datalayer.battery.status.voltage_dV = 3100;  // 310.0V
  datalayer.battery.info.min_design_voltage_dV = 3200;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_UNDERVOLTAGE));
  TEST_ASSERT_EQUAL(0, datalayer.battery.status.max_discharge_power_W);
}

// Test cell voltage - normal range
void test_update_machineryprotection_cell_voltage_normal(void) {
  datalayer.battery.status.cell_max_voltage_mV = 4000;
  datalayer.battery.status.cell_min_voltage_mV = 3800;
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_CELL_OVER_VOLTAGE));
  TEST_ASSERT_FALSE(is_event_set(EVENT_CELL_UNDER_VOLTAGE));
}

// Test cell overvoltage
void test_update_machineryprotection_cell_overvoltage(void) {
  datalayer.battery.status.cell_max_voltage_mV = 4200;
  datalayer.battery.info.max_cell_voltage_mV = datalayer.battery.status.cell_max_voltage_mV - 1;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CELL_OVER_VOLTAGE));
  TEST_ASSERT_EQUAL(0, datalayer.battery.status.max_charge_power_W);
}

// Test critical cell overvoltage
void test_update_machineryprotection_cell_critical_overvoltage(void) {
  datalayer.battery.status.cell_max_voltage_mV = 4350;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CELL_CRITICAL_OVER_VOLTAGE));
}

// Test cell undervoltage
void test_update_machineryprotection_cell_undervoltage(void) {
  datalayer.battery.status.cell_min_voltage_mV = 2700;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CELL_UNDER_VOLTAGE));
  TEST_ASSERT_EQUAL(0, datalayer.battery.status.max_discharge_power_W);
}

// Test critical cell undervoltage
void test_update_machineryprotection_cell_critical_undervoltage(void) {
  datalayer.battery.status.cell_min_voltage_mV = 2550;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CELL_CRITICAL_UNDER_VOLTAGE));
}

// Test cell voltage deviation
void test_update_machineryprotection_cell_voltage_deviation(void) {
  datalayer.battery.status.cell_max_voltage_mV = 4000;
  datalayer.battery.status.cell_min_voltage_mV = 3400;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CELL_DEVIATION_HIGH));
}

// Test SOC - normal range
void test_update_machineryprotection_soc_normal(void) {
  datalayer.battery.status.reported_soc = 5000;  // 50.00%
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_FULL));
  TEST_ASSERT_FALSE(is_event_set(EVENT_BATTERY_EMPTY));
}

// Test battery full
void test_update_machineryprotection_battery_full(void) {
  datalayer.battery.status.reported_soc = 10000;  // 100.00%
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_FULL));
  TEST_ASSERT_EQUAL(0, datalayer.battery.status.max_charge_power_W);
}

// Test battery empty
void test_update_machineryprotection_battery_empty(void) {
  datalayer.battery.status.reported_soc = 0;  // 0.00%
  datalayer.battery.status.bms_status = ACTIVE;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_BATTERY_EMPTY));
  TEST_ASSERT_EQUAL(0, datalayer.battery.status.max_discharge_power_W);
}

// Test SOH too low
void test_update_machineryprotection_soh_low(void) {
  datalayer.battery.status.soh_pptt = 2000;  // 20.00% (under 25.00% limit)
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_SOH_LOW));
}

// Test power levels - normal range
void test_update_machineryprotection_power_normal(void) {
  datalayer.battery.status.active_power_W = 5000;       // 5kW charging
  datalayer.battery.status.max_charge_power_W = 10000;  // 10kW max
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_CHARGE_LIMIT_EXCEEDED));
}

// Test charge limit exceeded
void test_update_machineryprotection_charge_limit_exceeded(void) {
  datalayer.battery.status.active_power_W = 13000;      // 13kW (over 10kW + 2kW tolerance)
  datalayer.battery.status.max_charge_power_W = 10000;  // 10kW max
  for (int i = 0; i < MAX_CHARGE_DISCHARGE_LIMIT_FAILURES + 2; i++) {
    update_machineryprotection();
  }
  TEST_ASSERT_TRUE(is_event_set(EVENT_CHARGE_LIMIT_EXCEEDED));
}

// Test discharge limit exceeded
void test_update_machineryprotection_discharge_limit_exceeded(void) {
  datalayer.battery.status.active_power_W = -13000;        // -13kW discharging (over 10kW + 2kW tolerance)
  datalayer.battery.status.max_discharge_power_W = 10000;  // 10kW max
  for (int i = 0; i < MAX_CHARGE_DISCHARGE_LIMIT_FAILURES + 2; i++) {
    update_machineryprotection();
  }
  TEST_ASSERT_TRUE(is_event_set(EVENT_DISCHARGE_LIMIT_EXCEEDED));
}

// Test CAN battery missing
void test_update_machineryprotection_can_battery_missing(void) {
  datalayer.battery.status.CAN_battery_still_alive = 0;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CAN_BATTERY_MISSING));
}

// Test CAN error counter exceeded
void test_update_machineryprotection_can_error_counter_exceeded(void) {
  datalayer.battery.status.CAN_error_counter = MAX_CAN_FAILURES + 1;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CAN_CORRUPTED_WARNING));
}

// Test CAN inverter missing
void test_update_machineryprotection_can_inverter_missing(void) {
  user_selected_inverter_protocol = InverterProtocolType::Solax;
  setup_inverter();
  datalayer.system.status.CAN_inverter_still_alive = 0;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CAN_INVERTER_MISSING));
}

// Test CAN inverter present
void test_update_machineryprotection_can_inverter_present(void) {
  user_selected_inverter_protocol = InverterProtocolType::Solax;
  setup_inverter();
  datalayer.system.status.CAN_inverter_still_alive = 1;
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_CAN_INVERTER_MISSING));
}

// Test CAN charger missing
void test_update_machineryprotection_can_charger_missing(void) {
  user_selected_charger_type = ChargerType::NissanLeaf;
  setup_charger();
  datalayer.charger.CAN_charger_still_alive = false;
  update_machineryprotection();
  TEST_ASSERT_TRUE(is_event_set(EVENT_CAN_CHARGER_MISSING));
}

// Test CAN charger present
void test_update_machineryprotection_can_charger_present(void) {
  user_selected_charger_type = ChargerType::NissanLeaf;
  setup_charger();
  datalayer.charger.CAN_charger_still_alive = true;
  update_machineryprotection();
  TEST_ASSERT_FALSE(is_event_set(EVENT_CAN_CHARGER_MISSING));
}

void setup() {
  UNITY_BEGIN();
  // CPU Temperature Tests
  RUN_TEST(test_update_machineryprotection_cpu_normal_temperature);
  RUN_TEST(test_update_machineryprotection_cpu_overheating_threshold);
  RUN_TEST(test_update_machineryprotection_cpu_overheated_threshold);
  RUN_TEST(test_update_machineryprotection_cpu_hysteresis_clearing);

  // CAN Interface Tests
  RUN_TEST(test_update_machineryprotection_can_native_failure);
  RUN_TEST(test_update_machineryprotection_can_2515_failure);
  RUN_TEST(test_update_machineryprotection_can_2518_failure);

  // Battery Temperature Tests
  RUN_TEST(test_update_machineryprotection_battery_temp_normal);
  RUN_TEST(test_update_machineryprotection_battery_overheating);
  RUN_TEST(test_update_machineryprotection_battery_frozen);
  RUN_TEST(test_update_machineryprotection_battery_temp_deviation);

  // Battery Voltage Tests
  RUN_TEST(test_update_machineryprotection_battery_voltage_normal);
  RUN_TEST(test_update_machineryprotection_battery_overvoltage);
  RUN_TEST(test_update_machineryprotection_battery_undervoltage);

  // Cell Voltage Tests
  RUN_TEST(test_update_machineryprotection_cell_voltage_normal);
  RUN_TEST(test_update_machineryprotection_cell_overvoltage);
  RUN_TEST(test_update_machineryprotection_cell_critical_overvoltage);
  RUN_TEST(test_update_machineryprotection_cell_undervoltage);
  RUN_TEST(test_update_machineryprotection_cell_critical_undervoltage);
  RUN_TEST(test_update_machineryprotection_cell_voltage_deviation);

  // SOC and SOH Tests
  RUN_TEST(test_update_machineryprotection_soc_normal);
  RUN_TEST(test_update_machineryprotection_battery_full);
  RUN_TEST(test_update_machineryprotection_battery_empty);
  RUN_TEST(test_update_machineryprotection_soh_low);

  // Power Limit Tests
  RUN_TEST(test_update_machineryprotection_power_normal);
  RUN_TEST(test_update_machineryprotection_charge_limit_exceeded);
  RUN_TEST(test_update_machineryprotection_discharge_limit_exceeded);

  // Communication Tests
  RUN_TEST(test_update_machineryprotection_can_battery_missing);
  RUN_TEST(test_update_machineryprotection_can_error_counter_exceeded);
  RUN_TEST(test_update_machineryprotection_can_inverter_missing);
  RUN_TEST(test_update_machineryprotection_can_charger_missing);
  RUN_TEST(test_update_machineryprotection_can_charger_present);

  UNITY_END();
}

void loop() {}
