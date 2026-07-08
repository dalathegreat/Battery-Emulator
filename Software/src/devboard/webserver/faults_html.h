#ifndef FAULTS_H
#define FAULTS_H

#include <WString.h>

/**
 * @brief Replaces placeholder with content section in web page
 *
 * Renders the Tesla Model 3/Y CAN alert-matrix (DTC) faults.
 * Data mapped from tesla-m3-pack-findings (firmware 2019.20.4.2).
 *
 * @param[in] var
 *
 * @return String
 */
String faults_processor(const String& var);

#endif
