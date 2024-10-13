#ifndef _DATALAYER_EXTENDED_H_
#define _DATALAYER_EXTENDED_H_

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

typedef struct {
  /** uint8_t */
  /** Service disconnect switch status */
  bool SDSW = 0;
  /** uint8_t */
  /** Pilotline status */
  bool pilotline = 0;
  /** uint8_t */
  /** HVIL status, 0 = Init, 1 = Closed, 2 = Open!, 3 = Fault */
  uint8_t HVIL = 0;
  /** uint8_t */
  /** 0 = HV inactive, 1 = HV active, 2 = Balancing, 3 = Extern charging, 4 = AC charging, 5 = Battery error, 6 = DC charging, 7 = init */
  uint8_t BMS_mode = 0;
  /** uint8_t */
  /** 1 = Battery display, 4 = Battery display OK, 4 = Display battery charging, 6 = Display battery check, 7 = Fault */
  uint8_t battery_diagnostic = 0;
  /** uint8_t */
  /** 0 = init, 1 = no open HV line detected, 2 = open HV line , 3 = fault */
  uint8_t status_HV_line = 0;
  /** uint8_t */
  /** 0 = OK, 1 = Not OK, 0x06 = init, 0x07 = fault */
  uint8_t warning_support = 0;

} DATALAYER_INFO_MEB;

class DataLayerExtended {
 public:
  DATALAYER_INFO_TESLA tesla;
  DATALAYER_INFO_NISSAN_LEAF nissanleaf;
  DATALAYER_INFO_MEB meb;
};

extern DataLayerExtended datalayer_extended;

#endif
