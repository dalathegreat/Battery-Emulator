#ifndef _DATALAYER_WEB_H_
#define _DATALAYER_WEB_H_

#include "../include.h"

typedef struct {
  /** uint8_t */
  /** Contactor status */
  uint8_t status_contactor = 0;
  /** uint8_t */
  /** Contactor status */
  uint8_t hvil_status = 0;
  /** uint8_t */
  /** Negative contactor state */
  uint8_t packContNegativeState = 0;
  /** uint8_t */
  /** Positive contactor state */
  uint8_t packContPositiveState = 0;
  /** uint8_t */
  /** Set state of contactors */
  uint8_t packContactorSetState = 0;
  /** uint8_t */
  /** Battery pack allows closing of contacors */
  uint8_t packCtrsClosingAllowed = 0;
  /** uint8_t */
  /** Pyro test in progress */
  uint8_t pyroTestInProgress = 0;

} DATALAYER_INFO_TESLA;

typedef struct {
  /** uint8_t */
  /** Enum, ZE0 = 0, AZE0 = 1, ZE1 = 2 */
  uint8_t LEAF_gen = 0;
  /** uint16_t */
  /** 77Wh per gid. LEAF specific unit */
  uint16_t GIDS = 0;
  /** uint16_t */
  /** Max regen power in kW */
  uint16_t ChargePowerLimit = 0;
  /** int16_t */
  /** Max charge power in kW */
  int16_t MaxPowerForCharger = 0;
  /** bool */
  /** Interlock status */
  bool Interlock = false;
  /** uint8_t */
  /** battery_FAIL status */
  uint8_t RelayCutRequest = 0;
  /** uint8_t */
  /** battery_STATUS status */
  uint8_t FailsafeStatus = 0;
  /** bool */
  /** True if fully charged */
  bool Full = false;
  /** bool */
  /** True if battery empty */
  bool Empty = false;
  /** bool */
  /** Battery pack allows closing of contacors */
  bool MainRelayOn = false;
  /** bool */
  /** True if heater exists */
  bool HeatExist = false;
  /** bool */
  /** Heater stopped */
  bool HeatingStop = false;
  /** bool */
  /** Heater starting */
  bool HeatingStart = false;
  /** bool */
  /** Heat request sent*/
  bool HeaterSendRequest = false;

} DATALAYER_INFO_NISSAN_LEAF;

class DataLayerWeb {
 public:
  DATALAYER_INFO_TESLA tesla;
  DATALAYER_INFO_NISSAN_LEAF nissanleaf;
};

extern DataLayerWeb datalayer_web;

#endif
