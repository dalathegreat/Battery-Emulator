#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "src/devboard/webserver/firmware_info_json.h"
#include "src/lib/bblanchon-ArduinoJson/ArduinoJson.h"

namespace {

std::string arduinojson_firmware_info(const char* hardware, const char* firmware) {
  JsonDocument doc;
  doc["hardware"] = hardware;
  doc["firmware"] = firmware;
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string formatted(const char* hardware, const char* firmware) {
  char buf[512];
  const int n = format_firmware_info_json(buf, sizeof(buf), hardware, firmware);
  EXPECT_GE(n, 0);
  EXPECT_LT(static_cast<size_t>(n), sizeof(buf));
  EXPECT_EQ(static_cast<size_t>(n), std::char_traits<char>::length(buf));
  return std::string(buf, static_cast<size_t>(n));
}

}  // namespace

TEST(FirmwareInfoJson, MatchesArduinoJsonForRepresentativeValues) {
  const char* samples[][2] = {
      {"LilyGo T-CAN485", "9.0.0dev-f8cac6b (main)"},
      {"LilyGo T_2CAN", "unknown"},
      {"ESP32 DevKit V1", "8.1.0"},
      {"DFRobot Edge101 IOT Controller", "v1.2.3dev-abc1234 (cursor/gzip-settings-js-ed41)"},
      {"Waveshare ESP32-S3-RS485-CAN", "10.0.0dev-deadbee (feature/quoted\"branch)"},
      {"Stark CMR Module", ""},
      {"BECom", "9.0.0dev-f8cac6b"},
      {"", ""},
      {"a\"b\\c", "quote"},
      {"\b\f\n\r\t", "controls"},
      {"\x01\x7f", "raw-controls"},
      {"\x80\xff", "high-bytes"},
  };

  for (const auto& sample : samples) {
    const std::string expected = arduinojson_firmware_info(sample[0], sample[1]);
    const std::string actual = formatted(sample[0], sample[1]);
    EXPECT_EQ(actual, expected) << sample[0];
  }
}

TEST(FirmwareInfoJson, TruncationMatchesSnprintf) {
  const char* hardware = "LilyGo T-CAN485";
  const char* firmware = "9.0.0dev-f8cac6b (main)";
  const std::string full = arduinojson_firmware_info(hardware, firmware);
  char tiny[8];
  memset(tiny, 0x5A, sizeof(tiny));
  const int n = format_firmware_info_json(tiny, sizeof(tiny), hardware, firmware);
  EXPECT_EQ(static_cast<size_t>(n), full.size());
  EXPECT_GE(n, static_cast<int>(sizeof(tiny)));
  EXPECT_EQ(tiny[sizeof(tiny) - 1], '\0');
  EXPECT_EQ(std::string(tiny, sizeof(tiny) - 1), full.substr(0, sizeof(tiny) - 1));
}
