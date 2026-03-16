#include "RTClib.h"

#define PCF8523_ADDRESS 0x68       ///< I2C address for PCF8523
#define PCF8523_CLKOUTCONTROL 0x0F ///< Timer and CLKOUT control register
#define PCF8523_CONTROL_1 0x00     ///< Control and status register 1
#define PCF8523_CONTROL_2 0x01     ///< Control and status register 2
#define PCF8523_CONTROL_3 0x02     ///< Control and status register 3
#define PCF8523_TIMER_B_FRCTL 0x12 ///< Timer B source clock frequency control
#define PCF8523_TIMER_B_VALUE 0x13 ///< Timer B value (number clock periods)
#define PCF8523_OFFSET 0x0E        ///< Offset register
#define PCF8523_STATUSREG 0x03     ///< Status register

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8523 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find PCF8523 or false otherwise.
*/
/**************************************************************************/
bool RTC_PCF8523::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(PCF8523_ADDRESS, wireInstance);
  if (!i2c_dev->begin())
    return false;
  return true;
}

/**************************************************************************/
/*!
    @brief  Check the status register Oscillator Stop flag to see if the PCF8523
   stopped due to power loss
    @details When battery or external power is first applied, the PCF8523's
   crystal oscillator takes up to 2s to stabilize. During this time adjust()
   cannot clear the 'OS' flag. See datasheet OS flag section for details.
    @return True if the bit is set (oscillator is or has stopped) and false only
   after the bit is cleared, for instance with adjust()
*/
/**************************************************************************/
bool RTC_PCF8523::lostPower(void) {
  return read_register(PCF8523_STATUSREG) >> 7;
}

/**************************************************************************/
/*!
    @brief  Check control register 3 to see if we've run adjust() yet (setting
   the date/time and battery switchover mode)
    @return True if the PCF8523 has been set up, false if not
*/
/**************************************************************************/
bool RTC_PCF8523::initialized(void) {
  return (read_register(PCF8523_CONTROL_3) & 0xE0) != 0xE0;
}

/**************************************************************************/
/*!
    @brief  Set the date and time, set battery switchover mode
    @param dt DateTime to set
*/
/**************************************************************************/
void RTC_PCF8523::adjust(const DateTime &dt) {
  uint8_t buffer[8] = {3, // start at location 3
                       bin2bcd(dt.second()),
                       bin2bcd(dt.minute()),
                       bin2bcd(dt.hour()),
                       bin2bcd(dt.day()),
                       bin2bcd(0), // skip weekdays
                       bin2bcd(dt.month()),
                       bin2bcd(dt.year() - 2000U)};
  i2c_dev->write(buffer, 8);

  // set to battery switchover mode
  write_register(PCF8523_CONTROL_3, 0x00);
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object containing the current date/time
*/
/**************************************************************************/
DateTime RTC_PCF8523::now() {
  uint8_t buffer[7];
  buffer[0] = 3;
  i2c_dev->write_then_read(buffer, 1, buffer, 7);

  return DateTime(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5]),
                  bcd2bin(buffer[3]), bcd2bin(buffer[2]), bcd2bin(buffer[1]),
                  bcd2bin(buffer[0] & 0x7F));
}

/**************************************************************************/
/*!
    @brief  Resets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8523::start(void) {
  uint8_t ctlreg = read_register(PCF8523_CONTROL_1);
  if (ctlreg & (1 << 5))
    write_register(PCF8523_CONTROL_1, ctlreg & ~(1 << 5));
}

/**************************************************************************/
/*!
    @brief  Sets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8523::stop(void) {
  write_register(PCF8523_CONTROL_1,
                 read_register(PCF8523_CONTROL_1) | (1 << 5));
}

/**************************************************************************/
/*!
    @brief  Is the PCF8523 running? Check the STOP bit in register Control_1
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
uint8_t RTC_PCF8523::isrunning() {
  return !((read_register(PCF8523_CONTROL_1) >> 5) & 1);
}

/**************************************************************************/
/*!
    @brief  Read the mode of the INT/SQW pin on the PCF8523
    @return SQW pin mode as a #Pcf8523SqwPinMode enum
*/
/**************************************************************************/
Pcf8523SqwPinMode RTC_PCF8523::readSqwPinMode() {
  int mode = read_register(PCF8523_CLKOUTCONTROL);
  mode >>= 3;
  mode &= 0x7;
  return static_cast<Pcf8523SqwPinMode>(mode);
}

/**************************************************************************/
/*!
    @brief  Set the INT/SQW pin mode on the PCF8523
    @param mode The mode to set, see the #Pcf8523SqwPinMode enum for options
*/
/**************************************************************************/
void RTC_PCF8523::writeSqwPinMode(Pcf8523SqwPinMode mode) {
  write_register(PCF8523_CLKOUTCONTROL, mode << 3);
}

/**************************************************************************/
/*!
    @brief  Enable the Second Timer (1Hz) Interrupt on the PCF8523.
    @details The INT/SQW pin will pull low for a brief pulse once per second.
*/
/**************************************************************************/
void RTC_PCF8523::enableSecondTimer() {
  uint8_t ctlreg = read_register(PCF8523_CONTROL_1);
  uint8_t clkreg = read_register(PCF8523_CLKOUTCONTROL);
  // TAM pulse int. mode (shared with Timer A), CLKOUT (aka SQW) disabled
  write_register(PCF8523_CLKOUTCONTROL, clkreg | 0xB8);
  // SIE Second timer int. enable
  write_register(PCF8523_CONTROL_1, ctlreg | (1 << 2));
}

/**************************************************************************/
/*!
    @brief  Disable the Second Timer (1Hz) Interrupt on the PCF8523.
*/
/**************************************************************************/
void RTC_PCF8523::disableSecondTimer() {
  write_register(PCF8523_CONTROL_1,
                 read_register(PCF8523_CONTROL_1) & ~(1 << 2));
}

/**************************************************************************/
/*!
    @brief  Enable the Countdown Timer Interrupt on the PCF8523.
    @details The INT/SQW pin will be pulled low at the end of a specified
   countdown period ranging from 244 microseconds to 10.625 days.
    Uses PCF8523 Timer B. Any existing CLKOUT square wave, configured with
   writeSqwPinMode(), will halt. The interrupt low pulse width is adjustable
   from 3/64ths (default) to 14/64ths of a second.
    @param clkFreq One of the PCF8523's Timer Source Clock Frequencies.
   See the #PCF8523TimerClockFreq enum for options and associated time ranges.
    @param numPeriods The number of clkFreq periods (1-255) to count down.
    @param lowPulseWidth Optional: the length of time for the interrupt pin
   low pulse. See the #PCF8523TimerIntPulse enum for options.
*/
/**************************************************************************/
void RTC_PCF8523::enableCountdownTimer(PCF8523TimerClockFreq clkFreq,
                                       uint8_t numPeriods,
                                       uint8_t lowPulseWidth) {
  // Datasheet cautions against updating countdown value while it's running,
  // so disabling allows repeated calls with new values to set new countdowns
  disableCountdownTimer();

  // Leave compatible settings intact
  uint8_t ctlreg = read_register(PCF8523_CONTROL_2);
  uint8_t clkreg = read_register(PCF8523_CLKOUTCONTROL);

  // CTBIE Countdown Timer B Interrupt Enabled
  write_register(PCF8523_CONTROL_2, ctlreg |= 0x01);

  // Timer B source clock frequency, optionally int. low pulse width
  write_register(PCF8523_TIMER_B_FRCTL, lowPulseWidth << 4 | clkFreq);

  // Timer B value (number of source clock periods)
  write_register(PCF8523_TIMER_B_VALUE, numPeriods);

  // TBM Timer B pulse int. mode, CLKOUT (aka SQW) disabled, TBC start Timer B
  write_register(PCF8523_CLKOUTCONTROL, clkreg | 0x79);
}

/**************************************************************************/
/*!
    @overload
    @brief  Enable Countdown Timer using default interrupt low pulse width.
    @param clkFreq One of the PCF8523's Timer Source Clock Frequencies.
   See the #PCF8523TimerClockFreq enum for options and associated time ranges.
    @param numPeriods The number of clkFreq periods (1-255) to count down.
*/
/**************************************************************************/
void RTC_PCF8523::enableCountdownTimer(PCF8523TimerClockFreq clkFreq,
                                       uint8_t numPeriods) {
  enableCountdownTimer(clkFreq, numPeriods, 0);
}

/**************************************************************************/
/*!
    @brief  Disable the Countdown Timer Interrupt on the PCF8523.
    @details For simplicity, this function strictly disables Timer B by setting
   TBC to 0. The datasheet describes TBC as the Timer B on/off switch.
   Timer B is the only countdown timer implemented at this time.
   The following flags have no effect while TBC is off, they are *not* cleared:
      - TBM: Timer B will still be set to pulsed mode.
      - CTBIE: Timer B interrupt would be triggered if TBC were on.
      - CTBF: Timer B flag indicates that interrupt was triggered. Though
        typically used for non-pulsed mode, user may wish to query this later.
*/
/**************************************************************************/
void RTC_PCF8523::disableCountdownTimer() {
  // TBC disable to stop Timer B clock
  write_register(PCF8523_CLKOUTCONTROL,
                 ~1 & read_register(PCF8523_CLKOUTCONTROL));
}

/**************************************************************************/
/*!
    @brief  Stop all timers, clear their flags and settings on the PCF8523.
    @details This includes the Countdown Timer, Second Timer, and any CLKOUT
   square wave configured with writeSqwPinMode().
*/
/**************************************************************************/
void RTC_PCF8523::deconfigureAllTimers() {
  disableSecondTimer(); // Surgically clears CONTROL_1
  write_register(PCF8523_CONTROL_2, 0);
  write_register(PCF8523_CLKOUTCONTROL, 0);
  write_register(PCF8523_TIMER_B_FRCTL, 0);
  write_register(PCF8523_TIMER_B_VALUE, 0);
}

/**************************************************************************/
/*!
    @brief Compensate the drift of the RTC.
    @details This method sets the "offset" register of the PCF8523,
      which can be used to correct a previously measured drift rate.
      Two correction modes are available:

      - **PCF8523\_TwoHours**: Clock adjustments are performed on
        `offset` consecutive minutes every two hours. This is the most
        energy-efficient mode.

      - **PCF8523\_OneMinute**: Clock adjustments are performed on
        `offset` consecutive seconds every minute. Extra adjustments are
        performed on the last second of the minute is `abs(offset)>60`.

      The `offset` parameter sets the correction amount in units of
      roughly 4&nbsp;ppm. The exact unit depends on the selected mode:

      |  mode               | offset unit                            |
      |---------------------|----------------------------------------|
      | `PCF8523_TwoHours`  | 4.340 ppm = 0.375 s/day = 2.625 s/week |
      | `PCF8523_OneMinute` | 4.069 ppm = 0.352 s/day = 2.461 s/week |

      See the accompanying sketch pcf8523.ino for an example on how to
      use this method.

    @param mode Correction mode, either `PCF8523_TwoHours` or
      `PCF8523_OneMinute`.
    @param offset Correction amount, from -64 to +63. A positive offset
      makes the clock slower.
*/
/**************************************************************************/
void RTC_PCF8523::calibrate(Pcf8523OffsetMode mode, int8_t offset) {
  write_register(PCF8523_OFFSET, ((uint8_t)offset & 0x7F) | mode);
}
