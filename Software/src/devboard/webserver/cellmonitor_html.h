#ifndef CELLMONITOR_H
#define CELLMONITOR_H

class AsyncWebServerRequest;

/**
 * @brief Renders the cell monitor page for the battery given in the "battery" query parameter
 *        (1-3, defaults to the first pack).
 *
 * @param[in] request
 */
void send_cellmonitor_page(AsyncWebServerRequest* request);

#endif
