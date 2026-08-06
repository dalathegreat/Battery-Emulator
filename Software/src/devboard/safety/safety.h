#ifndef SAFETY_H
#define SAFETY_H
#include <stdint.h>
#include <string>

#define MAX_CAN_FAILURES 50
#define BATTERY_MAX_TEMPERATURE_DEVIATION 150  // 150 = 15.0 °C
#define BATTERY_MAXTEMPERATURE 500
#define BATTERY_MINTEMPERATURE -250
#define MAX_CHARGE_DISCHARGE_LIMIT_FAILURES 5
#define CELL_CRITICAL_MV 100   // If cells go this much outside design voltage, shut battery down!
#define CELL_HYSTERESIS_MV 20  // Re-allow charge only once max cell drops this far below limit (avoids chatter at knee)
#define HYSTERESIS_OFFSET_DV 20  // Release a user-set voltage limit only once the pack is this far back inside it
#define LOWEST_ALLOWED_CELLVOLTAGE_RECOVERY_CHARGE_MV 2000  // Below this, emergency recovery charge is refused
#define MAX_CHARGEPOWER_RECOVERY_CHARGE_DA 50               // Ceiling on the current recovery charge may allow

//battery pause status begin
enum battery_pause_status { NORMAL = 0, PAUSING = 1, PAUSED = 2, RESUMING = 3 };
extern bool emulator_pause_request_ON;
extern bool emulator_pause_CAN_send_ON;
extern battery_pause_status emulator_pause_status;
extern bool allowed_to_send_CAN;
//battery pause status end

extern bool battery_detected;
extern bool battery2_detected;
extern bool battery3_detected;

extern void store_settings_equipment_stop();

void update_machineryprotection();
void graceful_restart();
void update_restart_progress();

typedef enum : uint8_t { UNCHANGED = 0, STOP = 1, RESUME = 2 } EquipmentStop;

//battery pause status begin
void setBatteryPause(bool pause_battery, bool pause_CAN, EquipmentStop equipment_stop = UNCHANGED,
                     bool store_settings = true);
void update_pause_state();
std::string get_emulator_pause_status();
//battery pause status end

#endif
