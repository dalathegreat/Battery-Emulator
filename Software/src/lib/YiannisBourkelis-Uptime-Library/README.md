# Uptime Library

With the uptime library for Arduino boards and compatible systems you can read the time passed since device startup, without the 49 days overflow limitation of the millis() function.

# Usage

#### Example 1: [Device Uptime](https://github.com/YiannisBourkelis/Uptime-Library/tree/master/examples/DeviceUptime "Device Uptime")
```cpp
#include "uptime_formatter.h"

void setup() {
  // connect at 115200 so we can read the uptime fast enough
  Serial.begin(115200);
}

void loop() {
  //uptime_formatter::get_uptime() returns a string 
  //containing the total device uptime since startup in days, hours, minutes and seconds
  Serial.println("up " + uptime_formatter::getUptime());

  //wait 1 second
  delay(1000);
}
```

#### Output:
```
up 0 days, 0 hours, 0 minutes, 56 seconds
up 0 days, 0 hours, 0 minutes, 57 seconds
up 0 days, 0 hours, 0 minutes, 58 seconds
up 0 days, 0 hours, 0 minutes, 59 seconds
up 0 days, 0 hours, 1 minutes, 0 seconds
up 0 days, 0 hours, 1 minutes, 1 seconds
up 0 days, 0 hours, 1 minutes, 2 seconds
up 0 days, 0 hours, 1 minutes, 3 seconds
up 0 days, 0 hours, 1 minutes, 4 seconds
up 0 days, 0 hours, 1 minutes, 5 seconds
up 0 days, 0 hours, 1 minutes, 6 seconds
up 0 days, 0 hours, 1 minutes, 7 seconds
up 0 days, 0 hours, 1 minutes, 8 seconds
up 0 days, 0 hours, 1 minutes, 9 seconds
```

#### Example 2: [Device Uptime Custom Formatting](https://github.com/YiannisBourkelis/Uptime-Library/tree/master/examples/DeviceUptimeCustomFormatting "Device Uptime Custom Formatting")
```cpp
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
```

#### Output:
```
days: 0
hours: 0
minutes: 23
seconds: 16
milliseconds: 23

days: 0
hours: 0
minutes: 23
seconds: 17
milliseconds: 23

days: 0
hours: 0
minutes: 23
seconds: 18
milliseconds: 23
```
