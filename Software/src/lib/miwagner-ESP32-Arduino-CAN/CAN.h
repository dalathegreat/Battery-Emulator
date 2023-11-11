/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, Thomas Barth, barth-dev.de
 *               2018, Michael Wagner, mw@iot-make.de
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __DRIVERS_CAN_H__
#define __DRIVERS_CAN_H__

#include <stdint.h>
#include "CAN_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief CAN frame type (standard/extended)
 */
typedef enum {
	CAN_frame_std = 0, /**< Standard frame, using 11 bit identifer. */
	CAN_frame_ext = 1  /**< Extended frame, using 29 bit identifer. */
} CAN_frame_format_t;

/**
 * \brief CAN RTR
 */
typedef enum {
	CAN_no_RTR = 0, /**< No RTR frame. */
	CAN_RTR = 1     /**< RTR frame. */
} CAN_RTR_t;

/** \brief Frame information record type */
typedef union {
	uint32_t U; /**< \brief Unsigned access */
	struct {
		uint8_t DLC : 4;               /**< \brief [3:0] DLC, Data length container */
		unsigned int unknown_2 : 2;    /**< \brief \internal unknown */
		CAN_RTR_t RTR : 1;             /**< \brief [6:6] RTR, Remote Transmission Request */
		CAN_frame_format_t FF : 1;     /**< \brief [7:7] Frame Format, see# CAN_frame_format_t*/
		unsigned int reserved_24 : 24; /**< \brief \internal Reserved */
	} B;
} CAN_FIR_t;

/** \brief CAN Frame structure */
typedef struct {
	CAN_FIR_t FIR;  /**< \brief Frame information record*/
	uint32_t MsgID; /**< \brief Message ID */
	union {
		uint8_t u8[8];   /**< \brief Payload byte access*/
		uint32_t u32[2]; /**< \brief Payload u32 access*/
		uint64_t u64;    /**< \brief Payload u64 access*/
	} data;
} CAN_frame_t;

typedef enum {
	Dual_Mode=0, 							/**< \brief The dual acceptance filter option is enabled (two filters, each with the length of 16 bit are active) */
	Single_Mode=1 							/**< \brief The single acceptance filter option is enabled (one filter with the length of 32 bit is active) */
} CAN_filter_mode_t;

/** \brief CAN Filter structure */
typedef struct {
    CAN_filter_mode_t 	FM:1;          		/**< \brief [0:0] Filter Mode */
    uint8_t 			ACR0;				/**< \brief Acceptance Code Register ACR0 */
	uint8_t 			ACR1;				/**< \brief Acceptance Code Register ACR1 */
	uint8_t 			ACR2;				/**< \brief Acceptance Code Register ACR2 */
	uint8_t 			ACR3;				/**< \brief Acceptance Code Register ACR3 */
	uint8_t 			AMR0;				/**< \brief Acceptance Mask Register AMR0 */
	uint8_t 			AMR1;				/**< \brief Acceptance Mask Register AMR1 */
	uint8_t 			AMR2;				/**< \brief Acceptance Mask Register AMR2 */
	uint8_t 			AMR3;				/**< \brief Acceptance Mask Register AMR3 */
} CAN_filter_t;

/**
 * \brief Initialize the CAN Module
 *
 * \return 0 CAN Module had been initialized
 */
int CAN_init(void);

/**
 * \brief Send a can frame
 *
 * \param	p_frame	Pointer to the frame to be send, see #CAN_frame_t
 * \return  0 Frame has been written to the module
 */
int CAN_write_frame(const CAN_frame_t *p_frame);

/**
 * \brief Stops the CAN Module
 *
 * \return 0 CAN Module was stopped
 */
int CAN_stop(void);

/**
 * \brief Config CAN Filter, must call before CANInit()
 *
 * \param	p_filter Pointer to the filter, see #CAN_filter_t
 * \return  0 CAN Filter had been initialized
 */
int CAN_config_filter(const CAN_filter_t* p_filter);


#ifdef __cplusplus
}
#endif

#endif
