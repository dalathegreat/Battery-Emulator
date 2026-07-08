#ifndef TESLA_ALERT_NAMES_H
#define TESLA_ALERT_NAMES_H

// Authoritative Tesla Model 3/Y alert-matrix (DTC) names, mapped from tesla-m3-pack-findings
// (firmware 2019.20.4.2). Index order matches datalayer_extended.tesla.*_alertMatrixActive[]
// filled in TeslaBattery::update_values(). Single shared copy used by both the battery debug
// output and the Faults web page.
extern const char* const TESLA_BMS_ALERT_NAMES[100];
extern const char* const TESLA_PCS_ALERT_NAMES[94];
extern const char* const TESLA_CP_ALERT_NAMES[96];

#endif  // TESLA_ALERT_NAMES_H
