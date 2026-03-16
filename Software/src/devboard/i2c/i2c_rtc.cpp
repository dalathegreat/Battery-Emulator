#include "i2c_rtc.h"
#include "i2c_devices.h"        
// #include "../debug/logging.h"
#include <src/lib/RTClib/RTClib.h> 
#include <sys/time.h>

RTC_DS3231 rtc;
bool is_rtc_online = false;

void init_i2c_rtc(BatteryEmulatorSettingsStore& settings) {
  if (settings.getBool("I2C_RTC", false)) {
    if (checkI2CDevicePresence(0x68)) {
      logging.println(" ✅ DS3231 RTC found at 0x68. Initializing...");
      
      if (rtc.begin(&Wire)) {
        is_rtc_online = true;
        if (rtc.lostPower()) {
          logging.println(" ⚠️ RTC lost power! Setting to compile time temporarily.");
          rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
        }
        syncSystemTimeFromRTC(); 
      } else {
        logging.println(" ❌ Failed to initialize DS3231 module!");
        is_rtc_online = false;
      }
    } else {
      logging.println(" ❌ DS3231 RTC enabled in settings, but NOT FOUND on bus! Bypassing...");
      is_rtc_online = false;
    }
  }
}

void syncSystemTimeFromRTC() {
  if (!is_rtc_online) return;
  DateTime now = rtc.now();
  struct timeval tv;
  tv.tv_sec = now.unixtime();
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);    
  logging.printf("🕒 System Time Synced from DS3231: %04d-%02d-%02d %02d:%02d:%02d\n",
                 now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
}

void updateRTCFromSystemTime() {
  if (!is_rtc_online) return;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (tv.tv_sec > 1577836800) { 
    rtc.adjust(DateTime(tv.tv_sec)); 
    logging.println("🔄 DS3231 RTC accurately updated from NTP/System Time.");
  }
}