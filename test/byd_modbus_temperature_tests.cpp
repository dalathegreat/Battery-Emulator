#include <gtest/gtest.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/inverter/BYD-MODBUS.h"

/* The Fronius Gen24 stops charge and discharge below -10 degrees C, because the pack it was
   designed for is LFP. The cap that works around that belongs to this inverter's registers: the
   datalayer is read by the web page, MQTT, ESP-NOW and every other inverter, and clamping it in
   place would hand all of them a temperature the packs never reported. */
namespace {

TEST(BydModbusTemperatureTest, ColdCapAppliesBetweenMinus9AndMinus20) {
  EXPECT_EQ(BydModbusInverter::clamp_cold_temperature(-150), -90);
  EXPECT_EQ(BydModbusInverter::clamp_cold_temperature(-91), -90);
}

TEST(BydModbusTemperatureTest, WarmerAndFarColderValuesPassThrough) {
  EXPECT_EQ(BydModbusInverter::clamp_cold_temperature(-90), -90);
  EXPECT_EQ(BydModbusInverter::clamp_cold_temperature(-50), -50);
  EXPECT_EQ(BydModbusInverter::clamp_cold_temperature(250), 250);
  // Below -20 C is left alone: a reading that extreme is a fault, not weather
  EXPECT_EQ(BydModbusInverter::clamp_cold_temperature(-250), -250);
}

}  // namespace
