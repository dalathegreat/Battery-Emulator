#ifndef METRICS_HTML_H
#define METRICS_HTML_H

#include <Arduino.h>

#include "../utils/events.h"  // Adjust paths as needed

// Global device name for metrics
extern String PROMETHEUS_DEVICE_NAME;

// Function to generate Prometheus metrics
String metrics_html_processor();

#endif  // METRICS_HTML_H
