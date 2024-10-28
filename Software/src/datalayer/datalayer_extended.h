#ifndef _DATALAYER_EXTENDED_H_
#define _DATALAYER_EXTENDED_H_

#include "../include.h"

typedef struct {
  /** uint16_t */
  /** SOC% raw battery value. Might not always reach 100% */
  uint16_t SOC_raw = 0;
  /** uint16_t */
  /** SOC% instrumentation cluster value. Will always reach 100% */
  uint16_t SOC_dash = 0;
  /** uint16_t */
  /** SOC% OBD2 value, polled actively */
  uint16_t SOC_OBD2 = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_ext = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_int = 0;
  /** uint8_t */
  /** Status cooling valve error, 0 not evaluated, 1 OK valve closed, 2 error active valve open, 3 Invalid signal*/
  uint8_t ST_valve_cooling = 0;
  /** uint8_t */
  /** Status interlock error, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_interlock = 0;
  /** uint8_t */
  /** Status precharge, 0 no statement, 1 Not active closing not blocked, 2 error precharge blocked, 3 Invalid signal*/
  uint8_t ST_precharge = 0;
  /** uint8_t */
  /** Status DC switch, 0 contactors open, 1 precharge ongoing, 2 contactors engaged, 3 Invalid signal*/
  uint8_t ST_DCSW = 0;
  /** uint8_t */
  /** Status emergency, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_EMG = 0;
  /** uint8_t */
  /** Status welding detection, 0 Contactors OK, 1 One contactor welded, 2 Two contactors welded, 3 Invalid signal*/
  uint8_t ST_WELD = 0;
  /** uint8_t */
  /** Status isolation, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_isolation = 0;
  /** uint8_t */
  /** Status cold shutoff valve, 0 OK, 1 Short circuit to GND, 2 Short circuit to 12V, 3 Line break, 6 Driver error, 12 Stuck, 13 Stuck, 15 Invalid Signal*/
  uint8_t ST_cold_shutoff_valve = 0;
} DATALAYER_INFO_BMWI3;

typedef struct {
  /** uint16_t */
  /** SOC% estimate. Estimated from total pack voltage */
  uint16_t SOC_estimated = 0;
  /** uint16_t */
  /** SOC% raw battery value. Highprecision. Can be locked if pack is crashed */
  uint16_t SOC_highprec = 0;
  /** uint16_t */
  /** SOC% polled OBD2 value. Can be locked if pack is crashed */
  uint16_t SOC_polled = 0;
  /** uint16_t */
  /** Voltage raw battery value */
  uint16_t voltage_periodic = 0;
  /** uint16_t */
  /** Voltage polled OBD2*/
  uint16_t voltage_polled = 0;

} DATALAYER_INFO_BYDATTO3;

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

class DataLayerExtended {
 public:
  DATALAYER_INFO_BMWI3 bmwi3;
  DATALAYER_INFO_BYDATTO3 bydAtto3;
  DATALAYER_INFO_TESLA tesla;
  DATALAYER_INFO_NISSAN_LEAF nissanleaf;
};

extern DataLayerExtended datalayer_extended;

#endif
