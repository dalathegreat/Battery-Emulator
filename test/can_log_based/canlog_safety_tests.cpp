#include <gtest/gtest.h>

#include "../utils/utils.h"

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/devboard/utils/events.h"

#include <fstream>

namespace fs = std::filesystem;

// These tests replay CAN logs against individual batteries to check that they
// are correctly parsed, and that safety mechanisms work.

// The base class for our tests
class CanLogTestFixture : public testing::Test {
 public:
  CanLogTestFixture(fs::path path) : path_(std::move(path)) {}
  // Optional:
  //   static void SetUpTestSuite() { ... }
  //   static void TearDownTestSuite() { ... }

  void SetUp() override {
    // Reset the datalayer and events before each test
    datalayer = DataLayer();
    reset_all_events();
    if (battery) {
      delete battery;
      battery = nullptr;
    }

    // Assume a 90s NMC pack for custom-BMS batteries
    user_selected_max_pack_voltage_dV = 378 + 10;
    user_selected_min_pack_voltage_dV = 261 - 10;
    user_selected_max_cell_voltage_mV = 4200 + 20;
    user_selected_min_cell_voltage_mV = 2900 - 20;

    // Extract battery type from log filename
    std::string filename = path_.filename().string();
    std::string batteryId = filename.substr(0, filename.find('_'));
    user_selected_battery_type = (BatteryType)std::stoi(batteryId);
    setup_battery();

    // Initialize datalayer to invalid values
    datalayer.battery.status.voltage_dV = 0;
    datalayer.battery.status.current_dA = INT16_MIN;
    datalayer.battery.status.cell_min_voltage_mV = 0;
    datalayer.battery.status.cell_max_voltage_mV = 0;
    datalayer.battery.status.real_soc = UINT16_MAX;
    datalayer.battery.status.temperature_max_dC = INT16_MIN;
    datalayer.battery.status.temperature_min_dC = INT16_MIN;
  }

  void TearDown() override {
    if (battery) {
      delete battery;
      battery = nullptr;
    }
  }

  void ProcessLog() {
    std::vector<CAN_frame> parsedMessages = parse_can_log_file(path_);

    for (const auto& msg : parsedMessages) {
      dynamic_cast<CanBattery*>(battery)->handle_incoming_can_frame(msg);
      dynamic_cast<CanBattery*>(battery)->update_values();
    }

    update_machineryprotection();

    // When debugging, uncomment this to see the parsed values
    // PrintValues();
  }

  void PrintValues() {
    std::cout << "Battery voltage: " << (datalayer.battery.status.voltage_dV / 10.0) << " V" << std::endl;
    std::cout << "Battery current: " << (datalayer.battery.status.current_dA / 10.0) << " A" << std::endl;
    std::cout << "Battery cell min voltage: " << datalayer.battery.status.cell_min_voltage_mV << " mV" << std::endl;
    std::cout << "Battery cell max voltage: " << datalayer.battery.status.cell_max_voltage_mV << " mV" << std::endl;
    std::cout << "Battery real SoC: " << (datalayer.battery.status.real_soc / 100.0) << " %" << std::endl;
    std::cout << "Battery temperature max: " << (datalayer.battery.status.temperature_max_dC / 10.0) << " C"
              << std::endl;
    std::cout << "Battery temperature min: " << (datalayer.battery.status.temperature_min_dC / 10.0) << " C"
              << std::endl;
  }

 private:
  fs::path path_;
};

// Check that the parsed logs populate the minimum required datalayer values for
// Battery Emulator to function.
class BaseValuesPresentTest : public CanLogTestFixture {
 public:
  explicit BaseValuesPresentTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    datalayer.battery.status.CAN_battery_still_alive = 10;

    ProcessLog();

    EXPECT_GT(datalayer.battery.status.CAN_battery_still_alive, 10);
    EXPECT_NE(datalayer.battery.status.voltage_dV, 0);
    // TODO: Current isn't actually a requirement? check power instead?
    //EXPECT_NE(datalayer.battery.status.current_dA, INT16_MIN);
    EXPECT_NE(datalayer.battery.status.cell_min_voltage_mV, 0);
    EXPECT_NE(datalayer.battery.status.cell_max_voltage_mV, 0);
    EXPECT_NE(datalayer.battery.status.real_soc, UINT16_MAX);
    EXPECT_NE(datalayer.battery.status.temperature_max_dC, INT16_MIN);
    EXPECT_NE(datalayer.battery.status.temperature_min_dC, INT16_MIN);

    EXPECT_EQ(get_event_pointer(EVENT_BATTERY_OVERVOLTAGE)->occurences, 0);
    EXPECT_EQ(get_event_pointer(EVENT_BATTERY_UNDERVOLTAGE)->occurences, 0);
  }
};

// Check that the parsed logs correctly trigger an overvoltage event.
class OverVoltageTest : public CanLogTestFixture {
 public:
  explicit OverVoltageTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    ProcessLog();

    EXPECT_EQ(get_event_pointer(EVENT_BATTERY_OVERVOLTAGE)->occurences, 1);
  }
};

// Check that the parsed logs correctly trigger a cell overvoltage event.
class CellOverVoltageTest : public CanLogTestFixture {
 public:
  explicit CellOverVoltageTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    ProcessLog();

    EXPECT_EQ(get_event_pointer(EVENT_CELL_OVER_VOLTAGE)->occurences, 1);
    EXPECT_EQ(get_event_pointer(EVENT_CELL_CRITICAL_OVER_VOLTAGE)->occurences, 1);
  }
};

// Check that the parsed logs correctly trigger a cell undervoltage event.
class CellUnderVoltageTest : public CanLogTestFixture {
 public:
  explicit CellUnderVoltageTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    ProcessLog();

    EXPECT_EQ(get_event_pointer(EVENT_CELL_UNDER_VOLTAGE)->occurences, 1);
    EXPECT_EQ(get_event_pointer(EVENT_CELL_CRITICAL_UNDER_VOLTAGE)->occurences, 1);
  }
};

void RegisterCanLogTests() {
  // The logs should be named as follows:
  //
  // <battery_type>_<battery class name>_<flag1>_<flag2...>.txt
  //
  // where:
  //   battery_type is the integer in the BatteryType enum
  //   flag1/flag2... are flags that indicate which tests to run:
  //     base: test that the minimmum required values are populated (and no events triggered)
  //     ov:   test that an overvoltage event is triggered
  //     cov:  test that normal and critical cell overvoltage events are triggered
  //     cuv:  test that normal and critical cell undervoltage events are triggered

  std::string directoryPath = "../can_log_based/can_logs";

  for (const auto& entry : fs::directory_iterator(directoryPath)) {
    if (!entry.is_regular_file() || entry.path().extension().string() != ".txt") {
      continue;
    }

    auto bits = split(entry.path().stem(), '_');
    auto has_flag = [&bits](const std::string& flag) -> bool {
      return std::find(bits.begin() + 2, bits.end(), flag) != bits.end();
    };

    if (has_flag("base")) {
      testing::RegisterTest("CanLogSafetyTests",
                            ("TestBaseValuesPresent" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
                            nullptr, nullptr, __FILE__, __LINE__,
                            [=]() -> CanLogTestFixture* { return new BaseValuesPresentTest(entry.path()); });
    }
    if (has_flag("ov")) {
      testing::RegisterTest("CanLogSafetyTests",
                            ("TestOverVoltage" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
                            nullptr, nullptr, __FILE__, __LINE__,
                            [=]() -> CanLogTestFixture* { return new OverVoltageTest(entry.path()); });
    }
    if (has_flag("cov")) {
      testing::RegisterTest("CanLogSafetyTests",
                            ("TestCellOverVoltage" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
                            nullptr, nullptr, __FILE__, __LINE__,
                            [=]() -> CanLogTestFixture* { return new CellOverVoltageTest(entry.path()); });
    }
    if (has_flag("cuv")) {
      testing::RegisterTest("CanLogSafetyTests",
                            ("TestCellUnderVoltage" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
                            nullptr, nullptr, __FILE__, __LINE__,
                            [=]() -> CanLogTestFixture* { return new CellUnderVoltageTest(entry.path()); });
    }
  }
}
