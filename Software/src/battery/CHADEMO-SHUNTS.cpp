/*  Portions of this file are an adaptation of the SimpleISA library, originally authored by Jack Rickard.
 *
 *  At present, this code supports the Scale IVT Modular current/voltage sensor device.  
 *  These devices measure current, up to three voltages, and provide temperature compensation.
 *  Additional sensors are planned to provide flexibility/lower BOM costs.
 *
 *  Original license/copyright header of SimpleISA is shown below:
 *   This library was written by Jack Rickard of EVtv - http://www.evtv.me
 *   copyright 2014
 *   You are licensed to use this library for any purpose, commercial or private, 
 *   without restriction.
 *
 *  2024 - Modified to make use of ESP32-Arduino-CAN by miwagner
 *
 */
#include "../include.h"
#ifdef CHADEMO_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "CHADEMO-BATTERY-INTERNAL.h"
#include "CHADEMO-BATTERY.h"
#include "CHADEMO-SHUNTS.h"

/* Initial frames received from ISA shunts provide invalid during initialization */
static int framecount = 0;

/* original variables/names/types from SimpleISA. These warrant refinement */
float Amperes;  // Floating point with current in Amperes
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

bool firstframe;
double milliamps;
long watt;
long As;
long lastAs;
long wh;
long lastWh;

/* Output command frame used to alter or initialize ISA shunt behavior
 * Please note that all delay/sleep operations are solely in this section of code,
 * not used during normal operation. Such delays are currently commented out.
 */
CAN_frame outframe = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x411,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

uint16_t get_measured_voltage() {
  return (uint16_t)Voltage;
}

uint16_t get_measured_current() {
  return (uint16_t)Amperes;
}

//This is our CAN interrupt service routine to catch inbound frames
inline void ISA_handleFrame(CAN_frame_t* frame) {

  if (frame->MsgID < 0x521 || frame->MsgID > 0x528) {
    return;
  }

  framecount++;

  switch (frame->MsgID) {
    case 0x511:
      break;

    case 0x521:
      ISA_handle521(frame);
      break;

    case 0x522:
      ISA_handle522(frame);
      break;

    case 0x523:
      ISA_handle523(frame);
      break;

    case 0x524:
      ISA_handle524(frame);
      break;

    case 0x525:
      ISA_handle525(frame);
      break;

    case 0x526:
      ISA_handle526(frame);
      break;

    case 0x527:
      ISA_handle527(frame);
      break;

    case 0x528:
      ISA_handle528(frame);
      break;
  }

  return;
}

//handle frame for Amperes
inline void ISA_handle521(CAN_frame_t* frame) {
  long current = 0;
  current =
      (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));

  milliamps = current;
  Amperes = current / 1000.0f;
}

//handle frame for Voltage
inline void ISA_handle522(CAN_frame_t* frame) {
  long volt =
      (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));

  Voltage = volt / 1000.0f;
  Voltage1 = Voltage - (Voltage2 + Voltage3);

  if (framecount < 150) {
    VoltageLO = Voltage;
    Voltage1LO = Voltage1;
  } else {
    if (Voltage < VoltageLO)
      VoltageLO = Voltage;
    if (Voltage > VoltageHI)
      VoltageHI = Voltage;
    if (Voltage1 < Voltage1LO)
      Voltage1LO = Voltage1;
    if (Voltage1 > Voltage1HI)
      Voltage1HI = Voltage1;
  }
}

//handle frame for Voltage 2
inline void ISA_handle523(CAN_frame_t* frame) {
  long volt =
      (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));

  Voltage2 = volt / 1000.0f;
  if (Voltage2 > 3)
    Voltage2 -= Voltage3;

  if (framecount < 150) {
    Voltage2LO = Voltage2;
  } else {
    if (Voltage2 < Voltage2LO)
      Voltage2LO = Voltage2;
    if (Voltage2 > Voltage2HI)
      Voltage2HI = Voltage2;
  }
}

//handle frame for Voltage3
inline void ISA_handle524(CAN_frame_t* frame) {
  long volt =
      (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));

  Voltage3 = volt / 1000.0f;

  if (framecount < 150) {
    Voltage3LO = Voltage3;
  } else {
    if (Voltage3 < Voltage3LO && Voltage3 > 10)
      Voltage3LO = Voltage3;
    if (Voltage3 > Voltage3HI)
      Voltage3HI = Voltage3;
  }
}

//handle frame for Temperature
inline void ISA_handle525(CAN_frame_t* frame) {
  long temp = 0;
  temp = (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));

  Temperature = temp / 10;
}

//handle frame for Kilowatts
inline void ISA_handle526(CAN_frame_t* frame) {
  watt = 0;
  watt = (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));
  KW = watt / 1000.0f;
}

//handle frame for Ampere-Hours
inline void ISA_handle527(CAN_frame_t* frame) {
  As = 0;
  As = (frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]);

  AH += (As - lastAs) / 3600.0f;
  lastAs = As;
}

//handle frame for kiloWatt-hours
inline void ISA_handle528(CAN_frame_t* frame) {
  wh = (long)((frame->data.u8[5] << 24) | (frame->data.u8[4] << 16) | (frame->data.u8[3] << 8) | (frame->data.u8[2]));
  KWH += (wh - lastWh) / 1000.0f;
  lastWh = wh;
}

/*
void ISA_initialize() {
    firstframe=false;
    STOP();
    delay(700);
    for(int i=0;i<9;i++) {
        Serial.println("initialization \n");

        outframe.data.u8[0]=(0x20+i);
        outframe.data.u8[1]=0x42;
        outframe.data.u8[2]=0x02;
        outframe.data.u8[3]=(0x60+(i*18));
        outframe.data.u8[4]=0x00;
        outframe.data.u8[5]=0x00;
        outframe.data.u8[6]=0x00;
        outframe.data.u8[7]=0x00;

        transmit_can((&outframe, can_config.battery);

        delay(500);

        sendSTORE();
        delay(500);
     }

    START();
    delay(500);
    lastAs=As;
    lastWh=wh;

}

void ISA_STOP() {
    outframe.data.u8[0]=0x34;
    outframe.data.u8[1]=0x00;
    outframe.data.u8[2]=0x01;
    outframe.data.u8[3]=0x00;
    outframe.data.u8[4]=0x00;
    outframe.data.u8[5]=0x00;
    outframe.data.u8[6]=0x00;
    outframe.data.u8[7]=0x00;
    transmit_can((&outframe, can_config.battery);

}

void ISA_sendSTORE() {
    outframe.data.u8[0]=0x32;
    outframe.data.u8[1]=0x00;
    outframe.data.u8[2]=0x00;
    outframe.data.u8[3]=0x00;
    outframe.data.u8[4]=0x00;
    outframe.data.u8[5]=0x00;
    outframe.data.u8[6]=0x00;
    outframe.data.u8[7]=0x00;
    transmit_can((&outframe, can_config.battery);
}

void ISA_START() {
    outframe.data.u8[0]=0x34;
    outframe.data.u8[1]=0x01;
    outframe.data.u8[2]=0x01;
    outframe.data.u8[3]=0x00;
    outframe.data.u8[4]=0x00;
    outframe.data.u8[5]=0x00;
    outframe.data.u8[6]=0x00;
    outframe.data.u8[7]=0x00;
    transmit_can((&outframe, can_config.battery);
}

void ISA_RESTART() {
    //Has the effect of zeroing AH and KWH  
    outframe.data.u8[0]=0x3F;
    outframe.data.u8[1]=0x00;
    outframe.data.u8[2]=0x00;
    outframe.data.u8[3]=0x00;
    outframe.data.u8[4]=0x00;
    outframe.data.u8[5]=0x00;
    outframe.data.u8[6]=0x00;
    outframe.data.u8[7]=0x00;
    transmit_can((&outframe, can_config.battery);
}

void ISA_deFAULT() {
    //Returns module to original defaults  
    outframe.data.u8[0]=0x3D;
    outframe.data.u8[1]=0x00;
    outframe.data.u8[2]=0x00;
    outframe.data.u8[3]=0x00;
    outframe.data.u8[4]=0x00;
    outframe.data.u8[5]=0x00;
    outframe.data.u8[6]=0x00;
    outframe.data.u8[7]=0x00;
    transmit_can((&outframe, can_config.battery);
}

void ISA_initCurrent() {
    STOP();
    delay(500);
    
    Serial.println("initialization \n");
    
    outframe.data.u8[0]=0x21;
    outframe.data.u8[1]=0x42;
    outframe.data.u8[2]=0x01;
    outframe.data.u8[3]=0x61;
    outframe.data.u8[4]=0x00;
    outframe.data.u8[5]=0x00;
    outframe.data.u8[6]=0x00;
    outframe.data.u8[7]=0x00;

    transmit_can((&outframe, can_config.battery);

    delay(500);

    sendSTORE();
    delay(500);

    START();
    delay(500);
    lastAs=As;
    lastWh=wh;
}
*/

#endif
