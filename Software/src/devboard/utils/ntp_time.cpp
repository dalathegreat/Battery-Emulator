#include "ntp_time.h"
#include "../../include.h"
#include "time.h"

const unsigned long millisInADay = 24 * 60 * 60 * 1000;  // 24 hours in milliseconds

unsigned long long getNtpTimeInMillis() {
  configTzTime(time_zone, ntpServer1, ntpServer2);
  struct tm timeinfo;

  // Wait for time to be set
  for (int i = 0; i < 5; i++) {
    if (getLocalTime(&timeinfo)) {
      logging.println("Time retrieved from NTP Server");
      break;
    }
    logging.println("Waiting for NTP time...");
  }

  if (!getLocalTime(&timeinfo)) {
    logging.println("Failed to obtain time");
    return 0;
  }

  // Convert to milliseconds
  time_t epochTime = mktime(&timeinfo);
  return static_cast<unsigned long long>(epochTime) * 1000;
}

// Function to calculate the difference in milliseconds to the next target time
unsigned long long millisToNextTargetTime(unsigned long long currentMillis, int targetTime) {
  int hour = targetTime / 100;
  int minute = targetTime % 100;

  time_t currentTime = currentMillis / 1000;  // Convert milliseconds to seconds
  struct tm* timeinfo = localtime(&currentTime);

  // Set timeinfo to the target time on the next day
  timeinfo->tm_hour = hour;
  timeinfo->tm_min = minute;
  timeinfo->tm_sec = 0;

  // Increment day if the current time is past the target time
  if (mktime(timeinfo) <= currentTime) {
    timeinfo->tm_mday += 1;
  }
  time_t nextTargetTime = mktime(timeinfo);
  unsigned long long nextTargetMillis = static_cast<unsigned long long>(nextTargetTime) * 1000;
  return nextTargetMillis - currentMillis;
}

unsigned long long getTimeOffsetfromNowUntil(int targetTime) {
  logging.println("Getting time offset from now until " + String(targetTime));
  unsigned long long timeinMillis = getNtpTimeInMillis();
  if (timeinMillis != 0) {
    logging.println("Time in millis: " + String(timeinMillis));
    unsigned long long timeOffsetUntilTargetTime = millisInADay - (millisToNextTargetTime(timeinMillis, targetTime));
    logging.println("Time offset until target time: " + String(timeOffsetUntilTargetTime));
    return timeOffsetUntilTargetTime;
  } else

    return 0;
}
