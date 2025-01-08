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
#ifndef UPTIMELIB_H
#define UPTIMELIB_H
/*
 * Uptime library for Arduino devices
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
 * 
 *Complete documentation for each function and variable name exist
 *inside the implementation uptime.cpp file
 */

class uptime
{
  public:
    uptime();

    static void          calculateUptime();

    static unsigned long getSeconds();
    static unsigned long getMinutes();
    static unsigned long getHours();
    static unsigned long getDays();
      
  private:
    static unsigned long m_milliseconds;
    static unsigned long m_seconds;
    static unsigned long m_minutes;
    static unsigned long m_hours;
    static unsigned long m_days;

    static unsigned long m_mod_milliseconds;
    static uint8_t m_mod_seconds;
    static uint8_t m_mod_minutes;
    static uint8_t m_mod_hours;
    
    static unsigned long m_last_milliseconds;  
    static unsigned long m_remaining_seconds;
    static unsigned long m_remaining_minutes;
    static unsigned long m_remaining_hours;
    static unsigned long m_remaining_days;
};
#endif
