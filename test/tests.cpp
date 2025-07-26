#include <gtest/gtest.h>
#include <stdio.h>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/safety/safety.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/inverter/ModbusInverterProtocol.h"

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

void store_settings_equipment_stop(void) {}

ModbusInverterProtocol::ModbusInverterProtocol() {}
