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

#include "uptime_formatter.h"
#include "uptime.h"

uptime_formatter::uptime_formatter()
{
}

//returns the actual time passed since device boot
//in the format: x days, y hours, z minutes, s seconds
String uptime_formatter::getUptime()
{
  uptime::calculateUptime();
  
  return  (String)(uptime::getDays()   ) + " days, "    +
          (String)(uptime::getHours()  ) + " hours, "   +
          (String)(uptime::getMinutes()) + " minutes, " +
          (String)(uptime::getSeconds()) + " seconds";
}
