/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, Thomas Barth, barth-dev.de
 *               2017, Jaime Breva, jbreva@nayarsystems.com
 * 				 2018, Michael Wagner, mw@iot-make.de
 *
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
 *
 */

#include "CAN.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_intr_alloc.h" // Renamed when migrating ESP32 2.x -> 3.x
#include "soc/dport_reg.h"
#include "soc/gpio_sig_map.h"
#include <math.h>

#include "driver/gpio.h"

#include "can_regdef.h"
#include "CAN_config.h"

// CAN Filter - no acceptance filter
static CAN_filter_t __filter = { Dual_Mode, 0, 0, 0, 0, 0Xff, 0Xff, 0Xff, 0Xff };

static void CAN_read_frame_phy();
static void CAN_isr(void *arg_p);
static int CAN_write_frame_phy(const CAN_frame_t *p_frame);
static SemaphoreHandle_t sem_tx_complete;

static void CAN_isr(void *arg_p) {

	// Interrupt flag buffer
	__CAN_IRQ_t interrupt;
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	// Read interrupt status and clear flags
	interrupt = MODULE_CAN->IR.U;

	// Handle RX frame available interrupt
	if ((interrupt & __CAN_IRQ_RX) != 0)
		CAN_read_frame_phy(&higherPriorityTaskWoken);

	// Handle TX complete interrupt
	// Handle error interrupts.
	if ((interrupt & (__CAN_IRQ_TX | __CAN_IRQ_ERR //0x4
	                  | __CAN_IRQ_DATA_OVERRUN     // 0x8
	                  | __CAN_IRQ_WAKEUP           // 0x10
	                  | __CAN_IRQ_ERR_PASSIVE      // 0x20
	                  | __CAN_IRQ_ARB_LOST         // 0x40
	                  | __CAN_IRQ_BUS_ERR          // 0x80
	                  )) != 0) {
		xSemaphoreGiveFromISR(sem_tx_complete, &higherPriorityTaskWoken);
	}

	// check if any higher priority task has been woken by any handler
	if (higherPriorityTaskWoken)
		portYIELD_FROM_ISR();
}

static void CAN_read_frame_phy(BaseType_t *higherPriorityTaskWoken) {

	// byte iterator
	uint8_t __byte_i;

	// frame read buffer
	CAN_frame_t __frame;

	// check if we have a queue. If not, operation is aborted.
	if (CAN_cfg.rx_queue == NULL) {
		// Let the hardware know the frame has been read.
		MODULE_CAN->CMR.B.RRB = 1;
		return;
	}

	// get FIR
	__frame.FIR.U = MODULE_CAN->MBX_CTRL.FCTRL.FIR.U;

	// check if this is a standard or extended CAN frame
	// standard frame
	if (__frame.FIR.B.FF == CAN_frame_std) {

		// Get Message ID
		__frame.MsgID = _CAN_GET_STD_ID;

		// deep copy data bytes
		for (__byte_i = 0; __byte_i < __frame.FIR.B.DLC; __byte_i++)
			__frame.data.u8[__byte_i] = MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.STD.data[__byte_i];
	}
	// extended frame
	else {

		// Get Message ID
		__frame.MsgID = _CAN_GET_EXT_ID;

		// deep copy data bytes
		for (__byte_i = 0; __byte_i < __frame.FIR.B.DLC; __byte_i++)
			__frame.data.u8[__byte_i] = MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.EXT.data[__byte_i];
	}

	// send frame to input queue
	xQueueSendToBackFromISR(CAN_cfg.rx_queue, &__frame, higherPriorityTaskWoken);

	// Let the hardware know the frame has been read.
	MODULE_CAN->CMR.B.RRB = 1;
}

static int CAN_write_frame_phy(const CAN_frame_t *p_frame) {

	// byte iterator
	uint8_t __byte_i;

	// copy frame information record
	MODULE_CAN->MBX_CTRL.FCTRL.FIR.U = p_frame->FIR.U;

	// standard frame
	if (p_frame->FIR.B.FF == CAN_frame_std) {

		// Write message ID
		_CAN_SET_STD_ID(p_frame->MsgID);

		// Copy the frame data to the hardware
		for (__byte_i = 0; __byte_i < p_frame->FIR.B.DLC; __byte_i++)
			MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.STD.data[__byte_i] = p_frame->data.u8[__byte_i];

	}
	// extended frame
	else {

		// Write message ID
		_CAN_SET_EXT_ID(p_frame->MsgID);

		// Copy the frame data to the hardware
		for (__byte_i = 0; __byte_i < p_frame->FIR.B.DLC; __byte_i++)
			MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.EXT.data[__byte_i] = p_frame->data.u8[__byte_i];
	}

	// Transmit frame
	MODULE_CAN->CMR.B.TR = 1;

	return 0;
}

int CAN_init() {

	// Time quantum
	double __tq;

	// enable module
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_CAN_CLK_EN);
	DPORT_SET_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_CAN_RST); //Added https://github.com/miwagner/ESP32-Arduino-CAN/pull/37/commits/feccb722866fbdcc7628b941efe9f79295b0cf81
	DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_CAN_RST);

	// configure TX pin
	gpio_set_level(CAN_cfg.tx_pin_id, 1);
	gpio_set_direction(CAN_cfg.tx_pin_id, GPIO_MODE_OUTPUT);
	gpio_matrix_out(CAN_cfg.tx_pin_id, TWAI_TX_IDX, 0, 0);
	gpio_pad_select_gpio(CAN_cfg.tx_pin_id);

	// configure RX pin
	gpio_set_direction(CAN_cfg.rx_pin_id, GPIO_MODE_INPUT);
	gpio_matrix_in(CAN_cfg.rx_pin_id, TWAI_RX_IDX, 0);
	gpio_pad_select_gpio(CAN_cfg.rx_pin_id);

	// set to PELICAN mode
	MODULE_CAN->CDR.B.CAN_M = 0x1;

	// synchronization jump width is the same for all baud rates
	MODULE_CAN->BTR0.B.SJW = 0x1;

	// TSEG2 is the same for all baud rates
	MODULE_CAN->BTR1.B.TSEG2 = 0x1;

	// select time quantum and set TSEG1
	switch (CAN_cfg.speed) {
	case CAN_SPEED_1000KBPS:
		MODULE_CAN->BTR1.B.TSEG1 = 0x4;
		__tq = 0.125;
		break;

	case CAN_SPEED_800KBPS:
		MODULE_CAN->BTR1.B.TSEG1 = 0x6;
		__tq = 0.125;
		break;

	case CAN_SPEED_200KBPS:
		MODULE_CAN->BTR1.B.TSEG1 = 0xc;
		MODULE_CAN->BTR1.B.TSEG2 = 0x5;
		__tq = 0.25;
		break;

	default:
		MODULE_CAN->BTR1.B.TSEG1 = 0xc;
		__tq = ((float) 1000 / CAN_cfg.speed) / 16;
	}

	// set baud rate prescaler
	MODULE_CAN->BTR0.B.BRP = (uint8_t) round((((APB_CLK_FREQ * __tq) / 2) - 1) / 1000000) - 1;

	/* Set sampling
	 * 1 -> triple; the bus is sampled three times; recommended for low/medium speed buses     (class A and B) where
	 * filtering spikes on the bus line is beneficial 0 -> single; the bus is sampled once; recommended for high speed
	 * buses (SAE class C)*/
	MODULE_CAN->BTR1.B.SAM = 0x0;

	// enable all interrupts
	MODULE_CAN->IER.U = 0xef; //ESP32 V3 0XEF     ESP32 NOT V3 0XFF

	 // Set acceptance filter	
	MODULE_CAN->MOD.B.AFM = __filter.FM;	
    MODULE_CAN->MBX_CTRL.ACC.CODE[0] = __filter.ACR0;
    MODULE_CAN->MBX_CTRL.ACC.CODE[1] = __filter.ACR1;
    MODULE_CAN->MBX_CTRL.ACC.CODE[2] = __filter.ACR2;
    MODULE_CAN->MBX_CTRL.ACC.CODE[3] = __filter.ACR3;
    MODULE_CAN->MBX_CTRL.ACC.MASK[0] = __filter.AMR0;
    MODULE_CAN->MBX_CTRL.ACC.MASK[1] = __filter.AMR1;
    MODULE_CAN->MBX_CTRL.ACC.MASK[2] = __filter.AMR2;
    MODULE_CAN->MBX_CTRL.ACC.MASK[3] = __filter.AMR3;

	// set to normal mode
	MODULE_CAN->OCR.B.OCMODE = __CAN_OC_NOM;

	// clear error counters
	MODULE_CAN->TXERR.U = 0;
	MODULE_CAN->RXERR.U = 0;
	(void) MODULE_CAN->ECC;

	// clear interrupt flags
	(void) MODULE_CAN->IR.U;

	// install CAN ISR
	esp_intr_alloc(ETS_CAN_INTR_SOURCE, 0, CAN_isr, NULL, NULL);

	// allocate the tx complete semaphore
	sem_tx_complete = xSemaphoreCreateBinary();

	// Showtime. Release Reset Mode.
	MODULE_CAN->MOD.B.RM = 0;

	return 0;
}

bool CAN_write_frame(const CAN_frame_t *p_frame) {
	if (sem_tx_complete == NULL) {
		return false;
	}

	// Write the frame to the controller
	CAN_write_frame_phy(p_frame);

	return xSemaphoreTake(sem_tx_complete, 20) == pdTRUE ? true : false;
}

int CAN_stop() {
	// enter reset mode
	MODULE_CAN->MOD.B.RM = 1;

	return 0;
}

int CAN_config_filter(const CAN_filter_t* p_filter) {
	
	__filter.FM = p_filter->FM;	
    __filter.ACR0 = p_filter->ACR0;
    __filter.ACR1 = p_filter->ACR1;
    __filter.ACR2 = p_filter->ACR2;
    __filter.ACR3 = p_filter->ACR3;
    __filter.AMR0 = p_filter->AMR0;
    __filter.AMR1 = p_filter->AMR1;
    __filter.AMR2 = p_filter->AMR2;
    __filter.AMR3 = p_filter->AMR3;
	
	return 0;
}
