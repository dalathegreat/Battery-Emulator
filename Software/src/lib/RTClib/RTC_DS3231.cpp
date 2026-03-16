#include "RTClib.h"

#define DS3231_ADDRESS 0x68   ///< I2C address for DS3231
#define DS3231_TIME 0x00      ///< Time register
#define DS3231_ALARM1 0x07    ///< Alarm 1 register
#define DS3231_ALARM2 0x0B    ///< Alarm 2 register
#define DS3231_CONTROL 0x0E   ///< Control register
#define DS3231_STATUSREG 0x0F ///< Status register
#define DS3231_TEMPERATUREREG                                                  \
  0x11 ///< Temperature register (high byte - low byte is at 0x12), 10-bit
       ///< temperature value

/**************************************************************************/
/*!
    @brief  Start I2C for the DS3231 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find DS3231 or false otherwise.
*/
/**************************************************************************/
bool RTC_DS3231::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(DS3231_ADDRESS, wireInstance);
  if (!i2c_dev->begin())
    return false;
  return true;
}

/**************************************************************************/
/*!
    @brief  Check the status register Oscillator Stop Flag to see if the DS3231
   stopped due to power loss
    @return True if the bit is set (oscillator stopped) or false if it is
   running
*/
/**************************************************************************/
bool RTC_DS3231::lostPower(void) {
  return read_register(DS3231_STATUSREG) >> 7;
}

/**************************************************************************/
/*!
    @brief  Set the date and flip the Oscillator Stop Flag
    @param dt DateTime object containing the date/time to set
*/
/**************************************************************************/
void RTC_DS3231::adjust(const DateTime &dt) {
  uint8_t buffer[8] = {DS3231_TIME,
                       bin2bcd(dt.second()),
                       bin2bcd(dt.minute()),
                       bin2bcd(dt.hour()),
                       bin2bcd(dowToDS3231(dt.dayOfTheWeek())),
                       bin2bcd(dt.day()),
                       bin2bcd(dt.month()),
                       bin2bcd(dt.year() - 2000U)};
  i2c_dev->write(buffer, 8);

  uint8_t statreg = read_register(DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_register(DS3231_STATUSREG, statreg);
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object with the current date/time
*/
/**************************************************************************/
DateTime RTC_DS3231::now() {
  uint8_t buffer[7];
  buffer[0] = 0;
  i2c_dev->write_then_read(buffer, 1, buffer, 7);

  return DateTime(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5] & 0x7F),
                  bcd2bin(buffer[4]), bcd2bin(buffer[2]), bcd2bin(buffer[1]),
                  bcd2bin(buffer[0] & 0x7F));
}

/**************************************************************************/
/*!
    @brief  Read the SQW pin mode
    @return Pin mode, see Ds3231SqwPinMode enum
*/
/**************************************************************************/
Ds3231SqwPinMode RTC_DS3231::readSqwPinMode() {
  int mode;
  mode = read_register(DS3231_CONTROL) & 0x1C;
  if (mode & 0x04)
    mode = DS3231_OFF;
  return static_cast<Ds3231SqwPinMode>(mode);
}

/**************************************************************************/
/*!
    @brief  Set the SQW pin mode
    @param mode Desired mode, see Ds3231SqwPinMode enum
*/
/**************************************************************************/
void RTC_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode) {
  uint8_t ctrl = read_register(DS3231_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  write_register(DS3231_CONTROL, ctrl | mode);
}

/**************************************************************************/
/*!
    @brief  Get the current temperature from the DS3231's temperature sensor
    @return Current temperature (float)
*/
/**************************************************************************/
float RTC_DS3231::getTemperature() {
  uint8_t buffer[2] = {DS3231_TEMPERATUREREG, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 2);
  return (float)buffer[0] + (buffer[1] >> 6) * 0.25f;
}

/**************************************************************************/
/*!
    @brief  Set alarm 1 for DS3231
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see Ds3231Alarm1Mode enum
    @return False if control register is not set, otherwise true
*/
/**************************************************************************/
bool RTC_DS3231::setAlarm1(const DateTime &dt, Ds3231Alarm1Mode alarm_mode) {
  uint8_t ctrl = read_register(DS3231_CONTROL);
  if (!(ctrl & 0x04)) {
    return false;
  }

  uint8_t A1M1 = (alarm_mode & 0x01) << 7; // Seconds bit 7.
  uint8_t A1M2 = (alarm_mode & 0x02) << 6; // Minutes bit 7.
  uint8_t A1M3 = (alarm_mode & 0x04) << 5; // Hour bit 7.
  uint8_t A1M4 = (alarm_mode & 0x08) << 4; // Day/Date bit 7.
  uint8_t DY_DT = (alarm_mode & 0x10)
                  << 2; // Day/Date bit 6. Date when 0, day of week when 1.
  uint8_t day = (DY_DT) ? dowToDS3231(dt.dayOfTheWeek()) : dt.day();

  uint8_t buffer[5] = {DS3231_ALARM1, uint8_t(bin2bcd(dt.second()) | A1M1),
                       uint8_t(bin2bcd(dt.minute()) | A1M2),
                       uint8_t(bin2bcd(dt.hour()) | A1M3),
                       uint8_t(bin2bcd(day) | A1M4 | DY_DT)};
  i2c_dev->write(buffer, 5);

  write_register(DS3231_CONTROL, ctrl | 0x01); // AI1E

  return true;
}

/**************************************************************************/
/*!
    @brief  Set alarm 2 for DS3231
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see Ds3231Alarm2Mode enum
    @return False if control register is not set, otherwise true
*/
/**************************************************************************/
bool RTC_DS3231::setAlarm2(const DateTime &dt, Ds3231Alarm2Mode alarm_mode) {
  uint8_t ctrl = read_register(DS3231_CONTROL);
  if (!(ctrl & 0x04)) {
    return false;
  }

  uint8_t A2M2 = (alarm_mode & 0x01) << 7; // Minutes bit 7.
  uint8_t A2M3 = (alarm_mode & 0x02) << 6; // Hour bit 7.
  uint8_t A2M4 = (alarm_mode & 0x04) << 5; // Day/Date bit 7.
  uint8_t DY_DT = (alarm_mode & 0x08)
                  << 3; // Day/Date bit 6. Date when 0, day of week when 1.
  uint8_t day = (DY_DT) ? dowToDS3231(dt.dayOfTheWeek()) : dt.day();

  uint8_t buffer[4] = {DS3231_ALARM2, uint8_t(bin2bcd(dt.minute()) | A2M2),
                       uint8_t(bin2bcd(dt.hour()) | A2M3),
                       uint8_t(bin2bcd(day) | A2M4 | DY_DT)};
  i2c_dev->write(buffer, 4);

  write_register(DS3231_CONTROL, ctrl | 0x02); // AI2E

  return true;
}

/**************************************************************************/
/*!
    @brief  Get the date/time value of Alarm1
    @return DateTime object with the Alarm1 data set in the
            day, hour, minutes, and seconds fields
*/
/**************************************************************************/
DateTime RTC_DS3231::getAlarm1() {
  uint8_t buffer[5] = {DS3231_ALARM1, 0, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 5);

  uint8_t seconds = bcd2bin(buffer[0] & 0x7F);
  uint8_t minutes = bcd2bin(buffer[1] & 0x7F);
  // Fetching the hour assumes 24 hour time (never 12)
  // because this library exclusively stores the time
  // in 24 hour format. Note that the DS3231 supports
  // 12 hour storage, and sets bits to indicate the type
  // that is stored.
  uint8_t hour = bcd2bin(buffer[2] & 0x3F);

  // Determine if the alarm is set to fire based on the
  // day of the week, or an explicit date match.
  bool isDayOfWeek = (buffer[3] & 0x40) >> 6;
  uint8_t day;
  if (isDayOfWeek) {
    // Alarm set to match on day of the week
    day = bcd2bin(buffer[3] & 0x0F);
  } else {
    // Alarm set to match on day of the month
    day = bcd2bin(buffer[3] & 0x3F);
  }

  // On the first week of May 2000, the day-of-the-week number
  // matches the date number.
  return DateTime(2000, 5, day, hour, minutes, seconds);
}

/**************************************************************************/
/*!
    @brief  Get the date/time value of Alarm2
    @return DateTime object with the Alarm2 data set in the
            day, hour, and minutes fields
*/
/**************************************************************************/
DateTime RTC_DS3231::getAlarm2() {
  uint8_t buffer[4] = {DS3231_ALARM2, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 4);

  uint8_t minutes = bcd2bin(buffer[0] & 0x7F);
  // Fetching the hour assumes 24 hour time (never 12)
  // because this library exclusively stores the time
  // in 24 hour format. Note that the DS3231 supports
  // 12 hour storage, and sets bits to indicate the type
  // that is stored.
  uint8_t hour = bcd2bin(buffer[1] & 0x3F);

  // Determine if the alarm is set to fire based on the
  // day of the week, or an explicit date match.
  bool isDayOfWeek = (buffer[2] & 0x40) >> 6;
  uint8_t day;
  if (isDayOfWeek) {
    // Alarm set to match on day of the week
    day = bcd2bin(buffer[2] & 0x0F);
  } else {
    // Alarm set to match on day of the month
    day = bcd2bin(buffer[2] & 0x3F);
  }

  // On the first week of May 2000, the day-of-the-week number
  // matches the date number.
  return DateTime(2000, 5, day, hour, minutes, 0);
}

/**************************************************************************/
/*!
    @brief  Get the mode for Alarm1
    @return Ds3231Alarm1Mode enum value for the current Alarm1 mode
*/
/**************************************************************************/
Ds3231Alarm1Mode RTC_DS3231::getAlarm1Mode() {
  uint8_t buffer[5] = {DS3231_ALARM1, 0, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 5);

  uint8_t alarm_mode = (buffer[0] & 0x80) >> 7    // A1M1 - Seconds bit
                       | (buffer[1] & 0x80) >> 6  // A1M2 - Minutes bit
                       | (buffer[2] & 0x80) >> 5  // A1M3 - Hour bit
                       | (buffer[3] & 0x80) >> 4  // A1M4 - Day/Date bit
                       | (buffer[3] & 0x40) >> 2; // DY_DT

  // Determine which mode the fetched alarm bits map to
  switch (alarm_mode) {
  case DS3231_A1_PerSecond:
  case DS3231_A1_Second:
  case DS3231_A1_Minute:
  case DS3231_A1_Hour:
  case DS3231_A1_Date:
  case DS3231_A1_Day:
    return (Ds3231Alarm1Mode)alarm_mode;
  default:
    // Default if the alarm mode cannot be read
    return DS3231_A1_Date;
  }
}

/**************************************************************************/
/*!
    @brief  Get the mode for Alarm2
    @return Ds3231Alarm2Mode enum value for the current Alarm2 mode
*/
/**************************************************************************/
Ds3231Alarm2Mode RTC_DS3231::getAlarm2Mode() {
  uint8_t buffer[4] = {DS3231_ALARM2, 0, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 4);

  uint8_t alarm_mode = (buffer[0] & 0x80) >> 7    // A2M2 - Minutes bit
                       | (buffer[1] & 0x80) >> 6  // A2M3 - Hour bit
                       | (buffer[2] & 0x80) >> 5  // A2M4 - Day/Date bit
                       | (buffer[2] & 0x40) >> 3; // DY_DT

  // Determine which mode the fetched alarm bits map to
  switch (alarm_mode) {
  case DS3231_A2_PerMinute:
  case DS3231_A2_Minute:
  case DS3231_A2_Hour:
  case DS3231_A2_Date:
  case DS3231_A2_Day:
    return (Ds3231Alarm2Mode)alarm_mode;
  default:
    // Default if the alarm mode cannot be read
    return DS3231_A2_Date;
  }
}

/**************************************************************************/
/*!
    @brief  Disable alarm
        @param 	alarm_num Alarm number to disable
*/
/**************************************************************************/
void RTC_DS3231::disableAlarm(uint8_t alarm_num) {
  uint8_t ctrl = read_register(DS3231_CONTROL);
  ctrl &= ~(1 << (alarm_num - 1));
  write_register(DS3231_CONTROL, ctrl);
}

/**************************************************************************/
/*!
    @brief  Clear status of alarm
        @param 	alarm_num Alarm number to clear
*/
/**************************************************************************/
void RTC_DS3231::clearAlarm(uint8_t alarm_num) {
  uint8_t status = read_register(DS3231_STATUSREG);
  status &= ~(0x1 << (alarm_num - 1));
  write_register(DS3231_STATUSREG, status);
}

/**************************************************************************/
/*!
    @brief  Get status of alarm
        @param 	alarm_num Alarm number to check status of
        @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_DS3231::alarmFired(uint8_t alarm_num) {
  return (read_register(DS3231_STATUSREG) >> (alarm_num - 1)) & 0x1;
}

/**************************************************************************/
/*!
    @brief  Enable 32KHz Output
    @details The 32kHz output is enabled by default. It requires an external
    pull-up resistor to function correctly
*/
/**************************************************************************/
void RTC_DS3231::enable32K(void) {
  uint8_t status = read_register(DS3231_STATUSREG);
  status |= (0x1 << 0x03);
  write_register(DS3231_STATUSREG, status);
}

/**************************************************************************/
/*!
    @brief  Disable 32KHz Output
*/
/**************************************************************************/
void RTC_DS3231::disable32K(void) {
  uint8_t status = read_register(DS3231_STATUSREG);
  status &= ~(0x1 << 0x03);
  write_register(DS3231_STATUSREG, status);
}

/**************************************************************************/
/*!
    @brief  Get status of 32KHz Output
    @return True if enabled otherwise false
*/
/**************************************************************************/
bool RTC_DS3231::isEnabled32K(void) {
  return (read_register(DS3231_STATUSREG) >> 0x03) & 0x01;
}
