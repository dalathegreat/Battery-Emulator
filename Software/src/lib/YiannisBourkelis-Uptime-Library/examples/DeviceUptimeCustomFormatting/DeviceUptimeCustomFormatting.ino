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
 
#include "uptime.h"

void setup() {
  // connect at 115200 so we can read the uptime fast enough
  Serial.begin(115200);
}

void loop() {
  //If you do not want to use the string library
  //you can get the total uptime variables
  //and format the output the way you want.
  
  //First call calculate_uptime() to calculate the uptime
  //and then read the uptime variables.
  uptime::calculateUptime();
  
  Serial.print("days: ");
  Serial.println(uptime::getDays());
  
  Serial.print("hours: ");
  Serial.println(uptime::getHours());
  
  Serial.print("minutes: ");
  Serial.println(uptime::getMinutes());
  
  Serial.print("seconds: ");
  Serial.println(uptime::getSeconds());
  
  Serial.print("milliseconds: ");
  Serial.println(uptime::getMilliseconds());

  Serial.print("\n");
  
  //wait 1 second
  delay(1000);
}
