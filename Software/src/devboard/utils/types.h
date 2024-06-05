#ifndef _TYPES_H_
#define _TYPES_H_

enum bms_status_enum { STANDBY = 0, INACTIVE = 1, DARKSTART = 2, ACTIVE = 3, FAULT = 4, UPDATING = 5 };
enum battery_chemistry_enum { NCA, NMC, LFP };
enum led_color { GREEN, YELLOW, RED, BLUE, RGB };

#define DISCHARGING 1
#define CHARGING 2

#define INTERVAL_10_MS 10
#define INTERVAL_20_MS 20
#define INTERVAL_30_MS 30
#define INTERVAL_50_MS 50
#define INTERVAL_100_MS 100
#define INTERVAL_200_MS 200
#define INTERVAL_500_MS 500
#define INTERVAL_640_MS 640
#define INTERVAL_1_S 1000
#define INTERVAL_2_S 2000
#define INTERVAL_5_S 5000
#define INTERVAL_10_S 10000
#define INTERVAL_60_S 60000

#define INTERVAL_10_MS_DELAYED 15
#define INTERVAL_20_MS_DELAYED 30
#define INTERVAL_30_MS_DELAYED 40
#define INTERVAL_100_MS_DELAYED 120
#define INTERVAL_500_MS_DELAYED 550

#define CAN_STILL_ALIVE \
  12  // Set by battery each time we get a CAN message. Decrements every 5seconds. Incase we reach 0 (after 60 seconds of inactivity)

#endif
