#ifndef _COMM_CAN_H_
#define _COMM_CAN_H_

#include "../../include.h"

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/value_mapping.h"
#include "../../lib/me-no-dev-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#ifdef CAN_ADDON
#include "../../lib/pierremolinaro-acan2515/ACAN2515.h"
#endif  //CAN_ADDON
#ifdef CANFD_ADDON
#include "../../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#endif  //CANFD_ADDON

/**
 * @brief Initialization function for CAN.
 *
 * @param[in] void
 *
 * @return void
 */
void init_CAN();

void transmit_can();

void send_can();

void receive_can();

void receive_canfd_addon();

void receive_can_native();

void receive_can_addon();

enum frameDirection { MSG_RX, MSG_TX };  //RX = 0, TX = 1
void print_can_frame(CAN_frame frame, frameDirection msgDir);

#endif
