/* ***********************************************************************
 * Uptime library for Arduino boards and compatible systems
 * (C) 2019 by Yiannis Bourkelis (https://github.com/YiannisBourkelis/)
 *
 * This file is part of Uptime library for Arduino boards and compatible systems
 *
 * Uptime library for Arduino boards and compatible systems is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Uptime library for Arduino boards and compatible systems is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Uptime library for Arduino boards and compatible systems.  If not, see <http://www.gnu.org/licenses/>.
 * ***********************************************************************/

/*
 * Uptime library for Arduino boards and compatible systems
 *
 * Caclulates the time passed since the device boot time, even after the millis() overflow, after 49 days
 * 
 * Usage:
 * include "uptime_formatter.h"
 * Inside your loop() function: 
 * Serial.println("Uptime: " + uptime_formatter::get_uptime());
 * 
 * Examples here:
 *
 * Created 08 May 2019
 * By Yiannis Bourkelis
 *
 * https://github.com/YiannisBourkelis/
 */

#include "../../../devboard/utils/millis64.h"
#include "uptime.h"

//private variabes for converting milliseconds to total seconds,minutes,hours and days
//after each call to millis()
uint64_t uptime::m_milliseconds;
uint64_t uptime::m_seconds;
uint64_t uptime::m_minutes;
uint64_t uptime::m_hours;
uint64_t uptime::m_days;

//private variables that in combination hold the actual time passed
//Use the coresponding uptime::get_.... to read these private variables
unsigned long uptime::m_mod_milliseconds;
uint8_t uptime::m_mod_seconds;
uint8_t uptime::m_mod_minutes;
uint8_t uptime::m_mod_hours;
    
uptime::uptime()
{
}

/**** get the actual time passed from device boot time ****/
unsigned long uptime::getSeconds()
{
  return uptime::m_mod_seconds;
}
unsigned long uptime::getMinutes()
{
  return uptime::m_mod_minutes;
}
unsigned long uptime::getHours()
{
  return uptime::m_mod_hours;
}
unsigned long uptime::getDays()
{
  return uptime::m_days;
}
/***********************************************************/

//calculate milliseconds, seconds, hours and days
//and store them in their static variables
void uptime::calculateUptime()
{
  uptime::m_milliseconds = millis64();

  //convert passed millis to total seconds, minutes, hours and days.
  uptime::m_seconds      = (uptime::m_milliseconds / 1000);
  uptime::m_minutes      = (uptime::m_seconds      / 60);
  uptime::m_hours        = (uptime::m_minutes      / 60);
  uptime::m_days         = (uptime::m_hours        / 24);

  //calculate the actual time passed, using modulus, in milliseconds, seconds and hours.
  //The days are calculated allready in the previous step. 
  uptime::m_mod_milliseconds = uptime::m_milliseconds % 1000;
  uptime::m_mod_seconds      = uptime::m_seconds      % 60;
  uptime::m_mod_minutes      = uptime::m_minutes      % 60;
  uptime::m_mod_hours        = uptime::m_hours        % 24;
}
