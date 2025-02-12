#ifndef __NTPTIME_H__
#define __NTPTIME_H__

extern const char* ntpServer1;
extern const char* ntpServer2;
extern const char* time_zone;

unsigned long long getNtpTimeInMillis();
unsigned long long millisToNextTargetTime(unsigned long long currentMillis, int targetTime);
unsigned long long getTimeOffsetfromNowUntil(int targetTime);

#endif
