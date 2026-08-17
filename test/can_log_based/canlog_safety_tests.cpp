#include <gtest/gtest.h>

#include "../utils/utils.h"

#include "Arduino.h"  // set_millis64() for driving the UDS poll clock

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/battery/CanBattery.h"
#include "../../Software/src/battery/UdsCanBattery.h"
#include "../../Software/src/devboard/utils/events.h"

#include <fstream>

// TX frame capture injected by the emulated CAN layer (see emul/can.cpp).
const std::vector<CAN_frame>& get_transmitted_frames();

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
    // Assume a 90s NMC pack for custom-BMS batteries
    user_selected_max_pack_voltage_dV = 378 + 10;
    user_selected_min_pack_voltage_dV = 261 - 10;
    user_selected_max_cell_voltage_mV = 4200 + 20;
    user_selected_min_cell_voltage_mV = 2900 - 20;

    // Extract battery type from log filename
    std::string filename = path_.filename().string();
    std::string batteryId = filename.substr(0, filename.find('_'));
    user_selected_battery_type = (BatteryType)std::stoi(batteryId);
    battery = nullptr;
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

  void HandleFrames() {
    // Read the CAN frames in and handle them. In the emulator this would run
    // every 1ms or so.

    std::vector<CAN_frame> parsedMessages = parse_can_log_file(path_);

    CanBattery* canBattery = dynamic_cast<CanBattery*>(battery);

    unsigned long now = 0;
    for (const auto& msg : parsedMessages) {
      // Pump the battery transmit (may be necessary)
      now = PumpBatteryTransmit(battery, msg, now);
      canBattery->handle_incoming_can_frame(msg);
    }

    // When debugging, uncomment this to see the parsed values
    // PrintValues();
  }

  unsigned long PumpBatteryTransmit(Battery* battery, const CAN_frame& msg, unsigned long now) {
    // Some log frames require that the battery is in the right state to receive
    // them. This includes UDS/KWP2000 PID scan responses, where the battery
    // cycles through a list of PIDs and will only accept responses for that
    // particular PID during the window after that particular poll transmission.

    UdsCanBattery* udsBattery = dynamic_cast<UdsCanBattery*>(battery);
    const auto& tx_frames = get_transmitted_frames();

    // Does this message looks like a UDS/KWP2000 poll response?
    if (udsBattery != nullptr && msg.ID >= 0x780 && msg.ID <= 0x7EF && (msg.data.u8[0] & 0xF0) == 0x10) {
      // Extract the SID from the poll response
      const uint8_t resp_sid = msg.data.u8[2];
      uint16_t target_pid = msg.data.u8[3];
      if (resp_sid == 0x62) {
        // Is a UDS two-byte PID response (as opposed to a one-byte KWP2000 one).
        target_pid = (target_pid << 8) | msg.data.u8[4];
      }

      // Now we keep hammering transmit_can up to 3000 times until we detect
      // that it has sent a poll to the SID we're responding for.

      const size_t tx_start = tx_frames.size();
      const unsigned long deadline = now + 30000;  // 30 s of emulated time
      bool poll_found = false;
      while (now < deadline && !poll_found) {
        now += 10;
        set_millis64(now);
        udsBattery->transmit_can(now);

        // Go through all the frames that the battery has transmitted.
        for (size_t i = tx_start; i < tx_frames.size(); i++) {
          const CAN_frame& req = tx_frames[i];
          const bool single_frame = (req.data.u8[0] == 0x02 || req.data.u8[0] == 0x03);
          const bool one_byte_match =
              (resp_sid == 0x61 && req.data.u8[1] == 0x21 && req.data.u8[2] == (uint8_t)target_pid);
          const bool two_byte_match = (resp_sid == 0x62 && req.data.u8[1] == 0x22 &&
                                       (((uint16_t)req.data.u8[2] << 8) | req.data.u8[3]) == target_pid);
          if (single_frame && (one_byte_match || two_byte_match)) {
            // We've found an outgoing poll for our SID. Now we can finish up,
            // and the battery should accept the next log message.
            poll_found = true;
            break;
          }
        }
      }
    }
    return now;
  }

  void UpdateValues() {
    // Run the battery update_values functions and apply safety protections. In
    // the emulator this happens every 1s.

    dynamic_cast<CanBattery*>(battery)->update_values();
    update_machineryprotection();
  }

  void HandleFramesAndUpdateValues() {
    HandleFrames();
    UpdateValues();
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

    // Set the charge/discharge power to a specific value
    datalayer.battery.status.max_charge_power_W = 11;
    datalayer.battery.status.max_discharge_power_W = 12;

    // Handle the CAN frames. This shouldn't touch the above limits.
    HandleFrames();
    EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 11);
    EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 12);

    // Update the values, which should overwrite the above limits
    UpdateValues();
    EXPECT_NE(datalayer.battery.status.max_charge_power_W, 11);
    EXPECT_NE(datalayer.battery.status.max_discharge_power_W, 12);

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
    HandleFramesAndUpdateValues();

    EXPECT_EQ(get_event_pointer(EVENT_BATTERY_OVERVOLTAGE)->occurences, 1);
  }
};

// Check that the parsed logs correctly trigger a cell overvoltage event.
class CellOverVoltageTest : public CanLogTestFixture {
 public:
  explicit CellOverVoltageTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    HandleFramesAndUpdateValues();

    EXPECT_EQ(get_event_pointer(EVENT_CELL_OVER_VOLTAGE)->occurences, 1);
    EXPECT_EQ(get_event_pointer(EVENT_CELL_CRITICAL_OVER_VOLTAGE)->occurences, 1);
  }
};

// Check that the parsed logs correctly trigger a cell undervoltage event.
class CellUnderVoltageTest : public CanLogTestFixture {
 public:
  explicit CellUnderVoltageTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    HandleFramesAndUpdateValues();

    EXPECT_EQ(get_event_pointer(EVENT_CELL_UNDER_VOLTAGE)->occurences, 1);
    EXPECT_EQ(get_event_pointer(EVENT_CELL_CRITICAL_UNDER_VOLTAGE)->occurences, 1);
  }
};

// Check that the parsed logs set a particular cell voltage value (3123mV)
class CellVoltageTest : public CanLogTestFixture {
 public:
  explicit CellVoltageTest(fs::path path, int cellnum) : CanLogTestFixture(path), cellnum_(cellnum) {}
  void TestBody() override {
    HandleFramesAndUpdateValues();

    // Target is 3123mV, but some integrations round strangely so need a margin
    EXPECT_GT(datalayer.battery.status.cell_voltages_mV[cellnum_], 3121);
    EXPECT_LT(datalayer.battery.status.cell_voltages_mV[cellnum_], 3125);
  }

 private:
  int cellnum_;
};

// Check that PID responses were correctly demuxed and interpreted.
// Currently very Zoe1 specific, could be generalized (or removed).
class PidDemuxTest : public CanLogTestFixture {
 public:
  explicit PidDemuxTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    HandleFramesAndUpdateValues();

    // 96 cells polled in two ISO-TP blocks (0x41: cells 0-61, 0x42: 62-95).
    // Every cell is 3700 mV in this log; the summed pack works out to 3552 dV.
    for (int i = 0; i < 96; i++) {
      EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[i], 3700) << "cell " << i;
    }
    EXPECT_EQ(datalayer.battery.status.voltage_dV, 3552);

    // 0x07 balancing group: one bit per cell, MSB first within each byte.
    // The log uses a pattern that spans every ISO-TP frame of the reply and
    // both edges of the first/last byte, so re-decode it here to check the
    // demuxed bits land on the right cells.
    const uint8_t balancing_bytes[] = {0xA5, 0x3C, 0x00, 0xFF, 0x81, 0x10, 0x42, 0x80, 0x01, 0x55, 0xAA, 0x0F};
    for (int i = 0; i < 96; i++) {
      bool expected = (balancing_bytes[i / 8] >> (7 - (i % 8))) & 1;
      EXPECT_EQ(datalayer.battery.status.cell_balancing_status[i], expected) << "cell " << i;
    }

    // 0x155 broadcast: SOC value bytes 4-5 are 0x2132 (8498 = 84.98%).
    EXPECT_EQ(datalayer.battery.status.real_soc, 8498);

    // 0x424 broadcast: min/max temperatures (18 C / 21 C -> 180/210 dC).
    EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 180);
    EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 210);

    // 0x61 metrics group: mileage 0xBB7C (47996 km), lifetime energy 0x23E4
    // (9188 kWh), surfaced on the advanced page via get_uds_info_html().
    UdsCanBattery* udsBattery = dynamic_cast<UdsCanBattery*>(battery);
    ASSERT_NE(udsBattery, nullptr);
    String uds_html = udsBattery->get_uds_info_html();
    EXPECT_NE(std::string(uds_html.c_str()).find("47996 km"), std::string::npos);
    EXPECT_NE(std::string(uds_html.c_str()).find("9188 kWh"), std::string::npos);

    // 0x424/0x445 heartbeats were 0x55, so no frame was rejected.
    EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);
  }
};

// Check that the driver accepted every frame in the log. Drivers that verify a
// checksum count rejected frames in CAN_error_counter.
class NoCanErrorsTest : public CanLogTestFixture {
 public:
  explicit NoCanErrorsTest(fs::path path) : CanLogTestFixture(path) {}
  void TestBody() override {
    datalayer.battery.status.CAN_battery_still_alive = 0;

    HandleFramesAndUpdateValues();

    EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);
    EXPECT_GT(datalayer.battery.status.CAN_battery_still_alive, 0);
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
  //     cv88: test that cell 88 (or another) voltage is correctly set (to 3123mV)
  //     crc:  test that no frame is rejected, i.e. the driver's checksum calculation
  //     pid:  test that KWP2000 poll responses were demuxed into the datalayer
  //           exactly (cells, balancing, SOC, temperatures, metrics)

  std::string directoryPath = TEST_CAN_LOG_DIR;

  for (const auto& entry : fs::directory_iterator(directoryPath)) {
    if (!entry.is_regular_file() || entry.path().extension().string() != ".txt") {
      continue;
    }

    auto bits = split(entry.path().stem(), '_');
    auto has_flag = [&bits](const std::string& flag) -> bool {
      // return true if any of the bits start with the supplied flag string
      return std::any_of(bits.begin() + 2, bits.end(),
                         [&flag](const std::string& bit) { return bit.rfind(flag, 0) == 0; });
    };
    auto get_int_flag = [&bits](const std::string& prefix) -> int {
      // parse anything after a matching flag prefix as an integer, or -1 if not found
      for (const auto& bit : bits) {
        if (bit.rfind(prefix, 0) == 0) {
          return std::stoi(bit.substr(prefix.size()));
        }
      }
      return -1;
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

    if (has_flag("crc")) {
      testing::RegisterTest("CanLogSafetyTests",
                            ("TestNoCanErrors" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
                            nullptr, nullptr, __FILE__, __LINE__,
                            [=]() -> CanLogTestFixture* { return new NoCanErrorsTest(entry.path()); });
    }

    if (has_flag("cv")) {
      int cellnum = get_int_flag("cv");
      testing::RegisterTest("CanLogSafetyTests",
                            ("TestCellVoltage" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
                            nullptr, nullptr, __FILE__, __LINE__,
                            [=]() -> CanLogTestFixture* { return new CellVoltageTest(entry.path(), cellnum); });
    }

    if (has_flag("pid")) {
      testing::RegisterTest(
          "CanLogSafetyTests", ("TestPidDemux" + snake_case_to_camel_case(entry.path().stem().string())).c_str(),
          nullptr, nullptr, __FILE__, __LINE__, [=]() -> CanLogTestFixture* { return new PidDemuxTest(entry.path()); });
    }
  }
}
