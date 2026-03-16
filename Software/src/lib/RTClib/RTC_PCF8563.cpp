#include "RTClib.h"

#define PCF8563_ADDRESS 0x51       ///< I2C address for PCF8563
#define PCF8563_CLKOUTCONTROL 0x0D ///< CLKOUT control register
#define PCF8563_CONTROL_1 0x00     ///< Control and status register 1
#define PCF8563_CONTROL_2 0x01     ///< Control and status register 2
#define PCF8563_VL_SECONDS 0x02    ///< register address for VL_SECONDS
#define PCF8563_CLKOUT_MASK 0x83   ///< bitmask for SqwPinMode on CLKOUT pin

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8563 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find PCF8563 or false otherwise.
*/
/**************************************************************************/
bool RTC_PCF8563::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(PCF8563_ADDRESS, wireInstance);
  if (!i2c_dev->begin())
    return false;
  return true;
}

/**************************************************************************/
/*!
    @brief  Check the status of the VL bit in the VL_SECONDS register.
    @details The PCF8563 has an on-chip voltage-low detector. When VDD drops
     below Vlow, bit VL in the VL_seconds register is set to indicate that
     the integrity of the clock information is no longer guaranteed.
    @return True if the bit is set (VDD droped below Vlow) indicating that
    the clock integrity is not guaranteed and false only after the bit is
    cleared using adjust()
*/
/**************************************************************************/
bool RTC_PCF8563::lostPower(void) {
  return read_register(PCF8563_VL_SECONDS) >> 7;
}

/**************************************************************************/
/*!
    @brief  Set the date and time
    @param dt DateTime to set
*/
/**************************************************************************/
void RTC_PCF8563::adjust(const DateTime &dt) {
  uint8_t buffer[8] = {PCF8563_VL_SECONDS, // start at location 2, VL_SECONDS
                       bin2bcd(dt.second()), bin2bcd(dt.minute()),
                       bin2bcd(dt.hour()),   bin2bcd(dt.day()),
                       bin2bcd(0), // skip weekdays
                       bin2bcd(dt.month()),  bin2bcd(dt.year() - 2000U)};
  i2c_dev->write(buffer, 8);
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object containing the current date/time
*/
/**************************************************************************/
DateTime RTC_PCF8563::now() {
  uint8_t buffer[7];
  buffer[0] = PCF8563_VL_SECONDS; // start at location 2, VL_SECONDS
  i2c_dev->write_then_read(buffer, 1, buffer, 7);

  return DateTime(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5] & 0x1F),
                  bcd2bin(buffer[3] & 0x3F), bcd2bin(buffer[2] & 0x3F),
                  bcd2bin(buffer[1] & 0x7F), bcd2bin(buffer[0] & 0x7F));
}

/**************************************************************************/
/*!
    @brief  Resets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8563::start(void) {
  uint8_t ctlreg = read_register(PCF8563_CONTROL_1);
  if (ctlreg & (1 << 5))
    write_register(PCF8563_CONTROL_1, ctlreg & ~(1 << 5));
}

/**************************************************************************/
/*!
    @brief  Sets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8563::stop(void) {
  uint8_t ctlreg = read_register(PCF8563_CONTROL_1);
  if (!(ctlreg & (1 << 5)))
    write_register(PCF8563_CONTROL_1, ctlreg | (1 << 5));
}

/**************************************************************************/
/*!
    @brief  Is the PCF8563 running? Check the STOP bit in register Control_1
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
uint8_t RTC_PCF8563::isrunning() {
  return !((read_register(PCF8563_CONTROL_1) >> 5) & 1);
}

/**************************************************************************/
/*!
    @brief  Read the mode of the CLKOUT pin on the PCF8563
    @return CLKOUT pin mode as a #Pcf8563SqwPinMode enum
*/
/**************************************************************************/
Pcf8563SqwPinMode RTC_PCF8563::readSqwPinMode() {
  int mode = read_register(PCF8563_CLKOUTCONTROL);
  return static_cast<Pcf8563SqwPinMode>(mode & PCF8563_CLKOUT_MASK);
}

/**************************************************************************/
/*!
    @brief  Set the CLKOUT pin mode on the PCF8563
    @param mode The mode to set, see the #Pcf8563SqwPinMode enum for options
*/
/**************************************************************************/
void RTC_PCF8563::writeSqwPinMode(Pcf8563SqwPinMode mode) {
  write_register(PCF8563_CLKOUTCONTROL, mode);
}
