#ifndef SimpleISA_h
#define SimpleISA_h

/*  This library supports the ISA Scale IVT Modular current/voltage sensor device.  These devices measure current, up to three voltages, and provide temperature compensation.

    This library was written by Jack Rickard of EVtv - http://www.evtv.me copyright 2016
    You are licensed to use this library for any purpose, commercial or private, 
    without restriction.
    
*/ 
#include <Arduino.h>
#include "../miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

class ISA 
{
	
	
		
		

	public:
		ISA();
    ~ISA();
		void initialize();
		void begin(int Port, int speed);
		void initCurrent();
		void sendSTORE();
		void STOP();
		void START();
		void RESTART();
		void deFAULT();
		

		float Amperes;   // Floating point with current in Amperes
		double AH;      //Floating point with accumulated ampere-hours
		double KW;
		double KWH;


		double Voltage;
		double Voltage1;
		double Voltage2;
		double Voltage3;
		double VoltageHI;
		double Voltage1HI;
		double Voltage2HI;
		double Voltage3HI;
		double VoltageLO;
		double Voltage1LO;
		double Voltage2LO;
		double Voltage3LO;

		double Temperature;
				
		bool debug;
		bool debug2;
		bool firstframe;
		int framecount;
		unsigned long timestamp;
		double milliamps;
		long watt;
		long As;
		long lastAs;
		long wh;
  		long lastWh;	
		void handleFrame(CAN_frame_t *frame); // CAN handler
		uint8_t page;
		
	private:
		CAN_frame_t frame;
		unsigned long elapsedtime;
		double  ampseconds;
		int milliseconds ;
		int seconds;
		int minutes;
		int hours;
		char buffer[9];
		char bigbuffer[90];
		uint32_t inbox;
		CAN_frame_t outframe = {
		    .FIR = {.B = {
		                .DLC = 8,
		                .unknown_2 = 0,
		                .RTR = CAN_no_RTR,
		                .FF = CAN_frame_std,
		            }},
		    .MsgID = 0x411,
		    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


		void printCAN(CAN_frame_t *frame);
		void handle521(CAN_frame_t *frame);
		void handle522(CAN_frame_t *frame);
		void handle523(CAN_frame_t *frame);
		void handle524(CAN_frame_t *frame);
		void handle525(CAN_frame_t *frame);
		void handle526(CAN_frame_t *frame);
		void handle527(CAN_frame_t *frame);
		void handle528(CAN_frame_t *frame);
		
								
};

#endif /* SimpleISA_h */
