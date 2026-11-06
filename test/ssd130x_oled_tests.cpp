#include <gtest/gtest.h>

#include "../Software/src/devboard/display/ssd130x_init.h"

/* Host-side checks for the shared SSD1306/SSD1309 I2C OLED init used on the
   Configurable port (e.g. LilyGo T-2CAN + HiLetgo 2.42" SSD1309). */

TEST(Ssd130xOledTests, PrefersPrimaryAddressThenAlternate) {
  EXPECT_EQ(ssd130x_select_i2c_address(true, false), SSD130X_I2C_ADDR_PRIMARY);
  EXPECT_EQ(ssd130x_select_i2c_address(true, true), SSD130X_I2C_ADDR_PRIMARY);
  EXPECT_EQ(ssd130x_select_i2c_address(false, true), SSD130X_I2C_ADDR_ALTERNATE);
  EXPECT_EQ(ssd130x_select_i2c_address(false, false), 0);
}

TEST(Ssd130xOledTests, InitSequenceHasSsd1309Essentials) {
  EXPECT_TRUE(ssd130x_init_has_cmd_arg(0x8d, 0x14));  // charge pump on
  EXPECT_TRUE(ssd130x_init_has_cmd_arg(0xda, 0x12));  // COM pins
  EXPECT_TRUE(ssd130x_init_has_cmd_arg(0x81, 0xcf));  // contrast
  EXPECT_TRUE(ssd130x_init_has_cmd(0xd9));            // pre-charge
  EXPECT_TRUE(ssd130x_init_has_cmd(0xdb));            // VCOMH
  EXPECT_TRUE(ssd130x_init_has_cmd(0xaf));            // display on
  EXPECT_FALSE(ssd130x_init_has_cmd_arg(0x81, 0x0f)); // old too-dark contrast
}
