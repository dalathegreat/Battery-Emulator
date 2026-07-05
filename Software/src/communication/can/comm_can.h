#ifndef _COMM_CAN_H_
#define _COMM_CAN_H_

#include "../../devboard/utils/types.h"

extern bool use_canfd_as_can;
extern bool use_canfd2_as_can;
extern uint16_t user_selected_CAN_ID_cutoff_filter;

void dump_can_frame(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir);
void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface);

//These defines are not used if user updates values via Settings page
#define CRYSTAL_FREQUENCY_MHZ 8

class CanReceiver;

typedef struct {
  CAN_Interface battery;
  CAN_Interface inverter;
  CAN_Interface battery_double;
  CAN_Interface battery_triple;
  CAN_Interface charger;
  CAN_Interface shunt;
} CAN_Configuration;

extern volatile CAN_Configuration can_config;

enum class CAN_Speed {
  CAN_SPEED_100KBPS = 100,
  CAN_SPEED_125KBPS = 125,
  CAN_SPEED_200KBPS = 200,
  CAN_SPEED_250KBPS = 250,
  CAN_SPEED_500KBPS = 500,
  CAN_SPEED_800KBPS = 800,
  CAN_SPEED_1000KBPS = 1000
};

// Register a receiver object for a given CAN interface.
// By default receivers expect the CAN interface to be operated at "fast" speed.
// If halfSpeed is true, half speed is used.
void register_can_receiver(CanReceiver* receiver, CAN_Interface interface,
                           CAN_Speed speed = CAN_Speed::CAN_SPEED_500KBPS);

/**
 * @brief Initializes all CAN interfaces requested earlier by other modules (see register_can_receiver)
 *
 * @param[in] void
 *
 * @return true if CAN interfaces were initialized successfully, false otherwise.
 */
bool init_CAN();

/**
 * @brief Receive CAN messages from all interfaces. Respective CanReceivers are called.
 *
 * @param[in] void
 *
 * @return void
 */
void receive_can();

// Stop/pause CAN communication for all interfaces
void stop_can();

// Restart CAN communication for all interfaces
void restart_can();

// Change the speed of the CAN interface. Returns true if successful.
bool change_can_speed(CAN_Interface interface, CAN_Speed speed);

#endif
