#ifndef ESP32CAN_H
#define ESP32CAN_H

#include "src/lib/ThomasBarth-ESP32-CAN-Driver/CAN_config.h"
#include "src/lib/ThomasBarth-ESP32-CAN-Driver/CAN.h"
extern uint8_t LEDcolor;

class ESP32CAN
{
    public: 
        bool tx_ok = true;
        int CANInit();
		    int CANConfigFilter(const CAN_filter_t* p_filter);
        int CANWriteFrame(const CAN_frame_t* p_frame);
        int CANStop();
        void CANSetCfg(CAN_device_t *can_cfg);
};

extern ESP32CAN ESP32Can;
#endif
