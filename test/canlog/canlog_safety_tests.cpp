#include <gtest/gtest.h>

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../../Software/src/devboard/utils/types.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

static bool endsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void printFrame(const CAN_frame& frame) {
  std::cout << "ID: " << std::hex << frame.ID << ", DLC: " << (int)frame.DLC << ", Data: ";
  for (int i = 0; i < frame.DLC; ++i) {
    std::cout << std::hex << (int)frame.data.u8[i] << " ";
  }
  std::cout << std::dec << "\n";
}

CAN_frame parseCanLogLine(const std::string& logLine) {
  std::stringstream ss(logLine);
  CAN_frame frame = {};  // Zero-initialize the struct
  char dummy;            // To consume characters like '(', ')', '[', ']'

  // The log format doesn't contain timestamp or interface name, so we skip them
  double timestamp;
  std::string interfaceName;

  // 1. Parse and discard Timestamp: (12768.986)
  ss >> dummy >> timestamp >> dummy;

  // 2. Parse and discard Interface Name: RX0
  ss >> interfaceName;

  // 3. Parse CAN ID: 172 (hexadecimal)
  ss >> std::hex >> frame.ID;
  if (ss.fail()) {
    throw std::runtime_error("Invalid format: Failed to parse CAN ID.");
  }
  // NOTE: The log format does not specify if the ID is extended.
  // We default to false. A more complex parser could infer this
  // based on the ID's value (e.g., if ID > 0x7FF).
  frame.ext_ID = false;

  // 4. Parse Data Length: [8]
  int dlc_val;
  ss >> dummy;  // Consume '['
  if (ss.fail() || dummy != '[') {
    throw std::runtime_error("Invalid format: Missing opening bracket for data length.");
  }
  ss >> dlc_val;
  frame.DLC = static_cast<uint8_t>(dlc_val);
  ss >> dummy;  // Consume ']'
  if (ss.fail() || dummy != ']') {
    throw std::runtime_error("Invalid format: Missing closing bracket for data length.");
  }
  // NOTE: The log format does not specify if this is a CAN FD frame.
  // We default to false.
  frame.FD = false;

  // 5. Parse Data Bytes: 3f a0 df 43 00 eb 00 00
  unsigned int byte;
  for (int i = 0; i < frame.DLC; ++i) {
    ss >> std::hex >> byte;
    if (ss.fail()) {
      throw std::runtime_error("Fewer data bytes than specified by data length.");
    }
    frame.data.u8[i] = static_cast<uint8_t>(byte);
  }

  return frame;
}

std::vector<CAN_frame> parseLogFile(const fs::path& filePath) {
  std::ifstream logFile(filePath);
  if (!logFile.is_open()) {
    std::cerr << "Error: Could not open file " << filePath << std::endl;
    return {};  // Return an empty vector
  }

  std::vector<CAN_frame> frames;
  std::string line;
  int lineNumber = 0;

  // Read the file line by line
  while (std::getline(logFile, line)) {
    lineNumber++;
    // Skip empty lines
    if (line.empty()) {
      continue;
    }

    if (line[0] == '#' || line[0] == ';') {
      continue;  // Skip comment lines
    }

    try {
      frames.push_back(parseCanLogLine(line));
    } catch (const std::runtime_error& e) {
      std::cerr << "Warning: Skipping malformed line " << lineNumber << " in " << filePath.filename()
                << ". Reason: " << e.what() << std::endl;
    }
  }

  return frames;
}

class CanLogTestFixture : public testing::Test {
 public:
  CanLogTestFixture(fs::path path) : path_(std::move(path)) {}
  // All of these optional, just like in regular macro usage.
  //   static void SetUpTestSuite() { ... }
  //   static void TearDownTestSuite() { ... }
  //   void SetUp() override { ... }
  //   void TearDown() override { ... }

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

    std::string filename = path_.filename().string();
    std::string batteryId = filename.substr(0, filename.find('_'));
    user_selected_battery_type = (BatteryType)std::stoi(batteryId);
    setup_battery();

    datalayer.battery.status.voltage_dV = 0;
    datalayer.battery.status.current_dA = INT16_MIN;
    datalayer.battery.status.cell_min_voltage_mV = 0;
    datalayer.battery.status.cell_max_voltage_mV = 0;
    datalayer.battery.status.real_soc = UINT16_MAX;
    datalayer.battery.status.temperature_max_dC = INT16_MIN;
    datalayer.battery.status.temperature_min_dC = INT16_MIN;
  }

  void ProcessLog() {
    std::vector<CAN_frame> parsedMessages = parseLogFile(path_);

    for (const auto& msg : parsedMessages) {
      //printFrame(msg);

      dynamic_cast<CanBattery*>(battery)->handle_incoming_can_frame(msg);
      dynamic_cast<CanBattery*>(battery)->update_values();
    }
    // Wait till the end so we don't raise spurious events due to missing values
    update_machineryprotection();
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

class AllValuesPresentTest : public CanLogTestFixture {
 public:
  explicit AllValuesPresentTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    ProcessLog();
    //PrintValues();

    EXPECT_NE(datalayer.battery.status.voltage_dV, 0);
    // Current isn't actually a requirement? power instead?
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

class OverVoltageTest : public CanLogTestFixture {
 public:
  explicit OverVoltageTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    ProcessLog();
    //PrintValues();

    EXPECT_EQ(get_event_pointer(EVENT_BATTERY_OVERVOLTAGE)->occurences, 1);
  }
};

void RegisterCanLogTests() {
  std::string directoryPath = "../canlog/canlogs";

  for (const auto& entry : fs::directory_iterator(directoryPath)) {
    if (entry.is_regular_file() && endsWith(entry.path(), "_good.txt")) {

      testing::RegisterTest("CanLogTestFixture", ("TestAllValuesPresent_" + entry.path().filename().string()).c_str(),
                            nullptr, entry.path().filename().string().c_str(), __FILE__, __LINE__,
                            // Important to use the fixture type as the return type here.
                            [=]() -> CanLogTestFixture* { return new AllValuesPresentTest(entry.path()); });
    }
    if (entry.is_regular_file() && endsWith(entry.path(), "_overvoltage.txt")) {

      testing::RegisterTest("CanLogTestFixture", ("TestOverVoltage_" + entry.path().filename().string()).c_str(),
                            nullptr, entry.path().filename().string().c_str(), __FILE__, __LINE__,
                            // Important to use the fixture type as the return type here.
                            [=]() -> CanLogTestFixture* { return new OverVoltageTest(entry.path()); });
    }
  }
}
