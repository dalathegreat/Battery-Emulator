#ifndef _COMM_CAN_H_
#define _COMM_CAN_H_

#include "../../include.h"

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/value_mapping.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

extern bool use_canfd_as_can;

void dump_can_frame(CAN_frame& frame, frameDirection msgDir);
void transmit_can_frame(CAN_frame* tx_frame, int interface);

// Register a receiver object for a given CAN interface
void register_can_receiver(CanReceiver* receiver, CAN_Interface interface);

/**
 * @brief Initializes all CAN interfaces requested earlier by other modules (see register_can_receiver)
 *
 * @param[in] void
 *
 * @return true if CAN interfaces were initialized successfully, false otherwise.
 */
bool init_CAN();

/**
 * @brief Receive CAN messages from all interfaces 
 *
 * @param[in] void
 *
 * @return void
 */
void receive_can();

/**
 * @brief Receive CAN messages from CAN tranceiver natively installed on Lilygo hardware
 *
 * @param[in] void
 *
 * @return void
 */
void receive_frame_can_native();

/**
 * @brief Receive CAN messages from CAN addon chip
 *
 * @param[in] void
 *
 * @return void
 */
void receive_frame_can_addon();

/**
 * @brief Receive CAN messages from CANFD addon chip
 *
 * @param[in] void
 *
 * @return void
 */
void receive_frame_canfd_addon();

/**
 * @brief print CAN frames via USB
 *
 * @param[in] void
 *
 * @return void
 */
void print_can_frame(CAN_frame frame, frameDirection msgDir);

#endif
