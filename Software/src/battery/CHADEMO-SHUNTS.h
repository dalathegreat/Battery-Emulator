#ifndef CHADEMO_SHUNTS_H
#define CHADEMO_SHUNTS_H

uint16_t get_measured_voltage();
uint16_t get_measured_current();
inline void ISA_handler(CAN_frame_t* frame);
inline void ISA_handle521(CAN_frame_t* frame);
inline void ISA_handle522(CAN_frame_t* frame);
inline void ISA_handle523(CAN_frame_t* frame);
inline void ISA_handle524(CAN_frame_t* frame);
inline void ISA_handle525(CAN_frame_t* frame);
inline void ISA_handle526(CAN_frame_t* frame);
inline void ISA_handle527(CAN_frame_t* frame);
inline void ISA_handle528(CAN_frame_t* frame);

#endif
