#include "RTClib.h"

#define DS1307_ADDRESS 0x68 ///< I2C address for DS1307
#define DS1307_CONTROL 0x07 ///< Control register
#define DS1307_NVRAM 0x08   ///< Start of RAM registers - 56 bytes, 0x08 to 0x3f

/**************************************************************************/
/*!
    @brief  Start I2C for the DS1307 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find DS1307 or false otherwise.
*/
/**************************************************************************/
bool RTC_DS1307::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(DS1307_ADDRESS, wireInstance);
  if (!i2c_dev->begin())
    return false;
  return true;
}

/**************************************************************************/
/*!
    @brief  Is the DS1307 running? Check the Clock Halt bit in register 0
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
uint8_t RTC_DS1307::isrunning(void) { return !(read_register(0) >> 7); }

/**************************************************************************/
/*!
    @brief  Set the date and time in the DS1307
    @param dt DateTime object containing the desired date/time
*/
/**************************************************************************/
void RTC_DS1307::adjust(const DateTime &dt) {
  uint8_t buffer[8] = {0,
                       bin2bcd(dt.second()),
                       bin2bcd(dt.minute()),
                       bin2bcd(dt.hour()),
                       0,
                       bin2bcd(dt.day()),
                       bin2bcd(dt.month()),
                       bin2bcd(dt.year() - 2000U)};
  i2c_dev->write(buffer, 8);
}

/**************************************************************************/
/*!
    @brief  Get the current date and time from the DS1307
    @return DateTime object containing the current date and time
*/
/**************************************************************************/
DateTime RTC_DS1307::now() {
  uint8_t buffer[7];
  buffer[0] = 0;
  i2c_dev->write_then_read(buffer, 1, buffer, 7);

  return DateTime(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5]),
                  bcd2bin(buffer[4]), bcd2bin(buffer[2]), bcd2bin(buffer[1]),
                  bcd2bin(buffer[0] & 0x7F));
}

/**************************************************************************/
/*!
    @brief  Read the current mode of the SQW pin
    @return Mode as Ds1307SqwPinMode enum
*/
/**************************************************************************/
Ds1307SqwPinMode RTC_DS1307::readSqwPinMode() {
  return static_cast<Ds1307SqwPinMode>(read_register(DS1307_CONTROL) & 0x93);
}

/**************************************************************************/
/*!
    @brief  Change the SQW pin mode
    @param mode The mode to use
*/
/**************************************************************************/
void RTC_DS1307::writeSqwPinMode(Ds1307SqwPinMode mode) {
  write_register(DS1307_CONTROL, mode);
}

/**************************************************************************/
/*!
    @brief  Read data from the DS1307's NVRAM
    @param buf Pointer to a buffer to store the data - make sure it's large
   enough to hold size bytes
    @param size Number of bytes to read
    @param address Starting NVRAM address, from 0 to 55
*/
/**************************************************************************/
void RTC_DS1307::readnvram(uint8_t *buf, uint8_t size, uint8_t address) {
  uint8_t addrByte = DS1307_NVRAM + address;
  i2c_dev->write_then_read(&addrByte, 1, buf, size);
}

/**************************************************************************/
/*!
    @brief  Write data to the DS1307 NVRAM
    @param address Starting NVRAM address, from 0 to 55
    @param buf Pointer to buffer containing the data to write
    @param size Number of bytes in buf to write to NVRAM
*/
/**************************************************************************/
void RTC_DS1307::writenvram(uint8_t address, const uint8_t *buf, uint8_t size) {
  uint8_t addrByte = DS1307_NVRAM + address;
  i2c_dev->write(buf, size, true, &addrByte, 1);
}

/**************************************************************************/
/*!
    @brief  Shortcut to read one byte from NVRAM
    @param address NVRAM address, 0 to 55
    @return The byte read from NVRAM
*/
/**************************************************************************/
uint8_t RTC_DS1307::readnvram(uint8_t address) {
  uint8_t data;
  readnvram(&data, 1, address);
  return data;
}

/**************************************************************************/
/*!
    @brief  Shortcut to write one byte to NVRAM
    @param address NVRAM address, 0 to 55
    @param data One byte to write
*/
/**************************************************************************/
void RTC_DS1307::writenvram(uint8_t address, uint8_t data) {
  writenvram(address, &data, 1);
}
