#ifndef CHEVYVOLT_CHARGER_H
#define CHEVYVOLT_CHARGER_H
#include <Arduino.h>
#include "../include.h"

#define CHARGER_SELECTED

/* Charger hardware limits
 *
 * Relative to runtime settings, expectations are:
 * hw minimum <= setting minimum <= setting maximum <= hw max
 */
#define CHEVYVOLT_MAX_HVDC 420.0
#define CHEVYVOLT_MIN_HVDC 200.0
#define CHEVYVOLT_MAX_AMP 11.5
#define CHEVYVOLT_MAX_POWER 3300

class ChevyVoltCharger : Charger {
 public:
  virtual void map_can_frame_to_variable_charger(CAN_frame rx_frame);
  virtual void transmit_can();
};

#endif
