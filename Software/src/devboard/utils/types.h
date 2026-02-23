#ifndef _TYPES_H_
#define _TYPES_H_

#include <chrono>
#include <string>

using milliseconds = std::chrono::milliseconds;
using duration = std::chrono::duration<unsigned long, std::ratio<1, 1000>>;

enum bms_status_enum { STANDBY = 0, INACTIVE = 1, DARKSTART = 2, ACTIVE = 3, FAULT = 4, UPDATING = 5 };
enum real_bms_status_enum { BMS_DISCONNECTED = 0, BMS_STANDBY = 1, BMS_ACTIVE = 2, BMS_FAULT = 3 };
enum balancing_status_enum {
  BALANCING_STATUS_UNKNOWN = 0,
  BALANCING_STATUS_ERROR = 1,
  BALANCING_STATUS_READY = 2,  //No balancing active, system supports balancing
  BALANCING_STATUS_ACTIVE = 3  //Balancing active!
};
enum battery_chemistry_enum { Autodetect = 0, NCA = 1, NMC = 2, LFP = 3, ZEBRA = 4, Highest };

enum class comm_interface {
  Modbus = 1,
  RS485 = 2,
  CanNative = 3,
  CanFdNative = 4,
  CanAddonMcp2515 = 5,
  CanFdAddonMcp2518 = 6,
  Highest
};

enum led_mode_enum { CLASSIC, FLOW, HEARTBEAT };
enum PrechargeState {
  AUTO_PRECHARGE_IDLE,
  AUTO_PRECHARGE_START,
  AUTO_PRECHARGE_PRECHARGING,
  AUTO_PRECHARGE_OFF,
  AUTO_PRECHARGE_COMPLETED,
  AUTO_PRECHARGE_FAILURE
};
enum BMSResetState {
  BMS_RESET_IDLE = 0,
  BMS_RESET_WAITING_FOR_PAUSE,
  BMS_RESET_POWERED_OFF,
  BMS_RESET_POWERING_ON,
};

#define DISCHARGING 1
#define CHARGING 2

#define INTERVAL_10_MS 10
#define INTERVAL_20_MS 20
#define INTERVAL_30_MS 30
#define INTERVAL_40_MS 40
#define INTERVAL_50_MS 50
#define INTERVAL_60_MS 60
#define INTERVAL_70_MS 70
#define INTERVAL_100_MS 100
#define INTERVAL_200_MS 200
#define INTERVAL_250_MS 250
#define INTERVAL_500_MS 500
#define INTERVAL_640_MS 640
#define INTERVAL_1_S 1000
#define INTERVAL_2_S 2000
#define INTERVAL_5_S 5000
#define INTERVAL_10_S 10000
#define INTERVAL_30_S 30000
#define INTERVAL_60_S 60000

#define INTERVAL_10_MS_DELAYED 15

#define CAN_STILL_ALIVE 60
// Set by battery each time we get a CAN message. Decrements every second. When reaching 0, sets event

enum CAN_Interface {
  // Native CAN port on the LilyGo & Stark hardware
  CAN_NATIVE = 0,

  // Native CANFD port on the Stark CMR hardware
  CANFD_NATIVE = 1,

  // Add-on CAN MCP2515 connected to GPIO pins
  CAN_ADDON_MCP2515 = 2,

  // Add-on CAN-FD MCP2518 connected to GPIO pins
  CANFD_ADDON_MCP2518 = 3,

  // No CAN interface
  NO_CAN_INTERFACE = 4
};

extern const char* getCANInterfaceName(CAN_Interface interface);

/* CAN Frame structure */
typedef struct {
  bool FD;
  bool ext_ID;
  uint8_t DLC;
  uint32_t ID;
  union {
    uint8_t u8[64];
    uint32_t u32[2];
    uint64_t u64;
  } data;
} CAN_frame;

enum frameDirection { MSG_RX, MSG_TX };  //RX = 0, TX = 1

typedef struct {
  CAN_frame frame;
  frameDirection direction;
} CAN_log_frame;

std::string getBMSStatus(bms_status_enum status);

#ifdef HW_LILYGO2CAN
/* Configurable GPIO options (device specific) */
enum class GPIOOPT1 {
  // T-2CAN: WUP1/WUP2 on GPIO1/GPIO2
  DEFAULT_OPT = 0,
  // T-2CAN: SDA/SCL on GPIO1/GPIO2
  I2C_DISPLAY_SSD1306 = 1,
  // T-2CAN: ESTOP on GPIO1, BMS_POWER on GPIO2
  ESTOP_BMS_POWER = 2,
  Highest
};
extern GPIOOPT1 user_selected_gpioopt1;
#endif
enum class GPIOOPT2 {
  // T-CAN485: Default, BMS power on PIN18
  DEFAULT_OPT_BMS_POWER_18 = 0,
  // T-CAN485: Move BMS power to PIN25
  BMS_POWER_25 = 1,
  Highest
};
enum class GPIOOPT3 {
  // T-CAN485: Default, SMA inverter pin PIN5
  DEFAULT_SMA_ENABLE_05 = 0,
  // T-CAN485: Move SMA inverter pin to PIN33
  SMA_ENABLE_33 = 1,
  Highest
};
enum class GPIOOPT4 {
  // T-CAN485: Default, uSD Card
  DEFAULT_SD_CARD = 0,
  // T-CAN485: Disable SD,Enable Display on Pins 14,15
  I2C_DISPLAY_SSD1306 = 1,
  Highest
};

extern GPIOOPT2 user_selected_gpioopt2;
extern GPIOOPT3 user_selected_gpioopt3;
extern GPIOOPT4 user_selected_gpioopt4;

#endif
