#include "../include.h"
#ifdef KIA_E_GMP_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "KIA-E-GMP-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10ms = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis20ms = 0;   // will store last time a 20ms CAN Message was send
static unsigned long previousMillis30ms = 0;   // will store last time a 30ms CAN Message was send
static unsigned long previousMillis100ms = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200ms = 0;  // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was send
static unsigned long previousMillis1s = 0;     // will store last time a 1s CAN Message was send
static unsigned long previousMillis2s = 0;     // will store last time a 2s CAN Message was send

#define MAX_CELL_VOLTAGE 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950  //Battery is put into emergency stop if one cell goes below this value

const unsigned char crc8_table[256] =
    {  // CRC8_SAE_J1850_ZER0 formula,0x1D Poly,initial value 0x3F,Final XOR value varies
        0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0,
        0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
        0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C, 0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23,
        0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
        0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B,
        0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65, 0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
        0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8,
        0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
        0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC,
        0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
        0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7, 0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C,
        0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
        0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47,
        0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09, 0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
        0xE3, 0xFE, 0xD9, 0xC4};

static uint16_t inverterVoltageFrameHigh = 0;
static uint16_t inverterVoltage = 0;
static uint16_t soc_calculated = 0;
static uint16_t SOC_BMS = 0;
static uint16_t SOC_Display = 0;
static uint16_t batterySOH = 1000;
static uint16_t CellVoltMax_mV = 3700;
static uint16_t CellVoltMin_mV = 3700;
static uint16_t batteryVoltage = 6700;
static int16_t leadAcidBatteryVoltage = 120;
static int16_t batteryAmps = 0;
static int16_t powerWatt = 0;
static int16_t temperatureMax = 0;
static int16_t temperatureMin = 0;
static int16_t allowedDischargePower = 0;
static int16_t allowedChargePower = 0;
static int16_t poll_data_pid = 0;
static uint8_t CellVmaxNo = 0;
static uint8_t CellVminNo = 0;
static uint8_t batteryManagementMode = 0;
static uint8_t BMS_ign = 0;
static uint8_t batteryRelay = 0;
static uint8_t waterleakageSensor = 164;
static uint8_t startedUp = false;
static uint8_t counter_200 = 0;
static uint8_t KIA_7E4_COUNTER = 0x01;
static int8_t temperature_water_inlet = 0;
static int8_t powerRelayTemperature = 0;
static int8_t heatertemp = 0;

static uint8_t ticks_200ms_counter = 0;
static uint8_t EGMP_1CF_counter = 0;
static uint8_t EGMP_3XF_counter = 0;

/* These messages are needed for contactor closing */
static uint8_t counter_10ms = 0;
static uint8_t EGMP_counter_byte2[16] = {0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93,
                                         0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b};
static uint8_t EGMP_F5_byte0[16] = {0x85, 0xC3, 0x09, 0x4F, 0x0B, 0x4D, 0x87, 0xC1,
                                    0x32, 0x74, 0xBE, 0xF8, 0x79, 0x3F, 0xF5, 0xB3};
static uint8_t EGMP_F5_byte1[16] = {0x13, 0x4a, 0xa1, 0xf8, 0x48, 0x11, 0xfa, 0xa3,
                                    0x3d, 0x64, 0x8f, 0xd6, 0xa2, 0xfb, 0x10, 0x49};
static uint8_t EGMP_10A_byte0[16] = {0x62, 0x24, 0xee, 0xa8, 0xec, 0xaa, 0x60, 0x26,
                                     0xd5, 0x93, 0x59, 0x1f, 0x9e, 0xd8, 0x12, 0x54};
static uint8_t EGMP_10A_byte1[16] = {0x36, 0x6f, 0x84, 0xdd, 0x6d, 0x34, 0xdf, 0x86,
                                     0x18, 0x41, 0xaa, 0xf3, 0x87, 0xde, 0x35, 0x6c};
static uint8_t EGMP_120_byte0[16] = {0xd4, 0x92, 0x58, 0x1e, 0x5a, 0x1c, 0xd6, 0x90,
                                     0x63, 0x25, 0xef, 0xa9, 0x28, 0x6e, 0xa4, 0xe2};
static uint8_t EGMP_120_byte1[16] = {0x1b, 0x42, 0xa9, 0xf0, 0x40, 0x19, 0xf2, 0xab,
                                     0x35, 0x6c, 0x87, 0xde, 0xaa, 0xf3, 0x18, 0x41};
static uint8_t EGMP_35_byte0[16] = {0x24, 0x62, 0xa8, 0xee, 0xaa, 0xec, 0x26, 0x60,
                                    0x93, 0xd5, 0x1f, 0x59, 0xd8, 0x9e, 0x54, 0x12};
static uint8_t EGMP_35_byte1[16] = {0x8e, 0xd7, 0x3c, 0x65, 0xd5, 0x8c, 0x67, 0x3e,
                                    0xa0, 0xf9, 0x12, 0x4b, 0x3f, 0x66, 0x8d, 0xd4};
static uint8_t EGMP_19A_byte0[16] = {0x24, 0xd7, 0x91, 0x5b, 0x1d, 0x6b, 0x2d, 0xe7,
                                     0xa1, 0x52, 0x14, 0xde, 0x98, 0x19, 0x5f, 0x95};
static uint8_t EGMP_19A_byte1[16] = {0x9b, 0x05, 0x5c, 0xb7, 0xee, 0xa2, 0xfb, 0x10,
                                     0x49, 0xd7, 0x8e, 0x65, 0x3c, 0x48, 0x11, 0xfa};
static uint8_t EGMP_19A_byte2[16] = {0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82,
                                     0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a};
CAN_frame EGMP_F5 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0xF5,
    .data = {0x85, 0x13, 0x8c, 0x46, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_10A = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x10A,
    .data = {0x62, 0x36, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x01, 0x00, 0x00, 0x36, 0x39, 0x35, 0x35,
             0xc9, 0x02, 0x00, 0x00, 0x10, 0x00, 0x00, 0x35, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_120 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x120,
    .data = {0xd4, 0x1b, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x01, 0x00, 0x00, 0x37, 0x35, 0x37, 0x37,
             0xc9, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_19A = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x19A,
    .data = {0x24, 0x9b, 0x7b, 0x55, 0x44, 0x64, 0xd8, 0x1b, 0x40, 0x20, 0x00, 0x00, 0x00, 0x00, 0x11, 0x52,
             0x00, 0x12, 0x02, 0x64, 0x00, 0x00, 0x00, 0x08, 0x13, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00}};
CAN_frame EGMP_35 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x35,
    .data = {0x62, 0xd7, 0x8d, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00, 0x00,
             0x06, 0x8d, 0x00, 0x00, 0xf0, 0x39, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_30A = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x30A,
    .data = {0xb1, 0xe0, 0x26, 0x08, 0x54, 0x01, 0x04, 0x15, 0x00, 0x1a, 0x76, 0x00, 0x25, 0x01, 0x10, 0x27,
             0x4f, 0x06, 0x18, 0x04, 0x33, 0x15, 0x34, 0x28, 0x00, 0x00, 0x10, 0x06, 0x21, 0x00, 0x4b, 0x06}};
CAN_frame EGMP_320 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x320,
    .data = {0xc6, 0xab, 0x26, 0x41, 0x00, 0x00, 0x01, 0x3c, 0xac, 0x0d, 0x40, 0x20, 0x05, 0xc8, 0xa0, 0x03,
             0x40, 0x20, 0x2b, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_2AA = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2AA,
    .data = {0x86, 0xea, 0x42, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x40, 0x00, 0x00,
             0x00, 0x01, 0x40, 0x00, 0xff, 0xff, 0xff, 0x0f, 0x01, 0x00, 0xff, 0x3f, 0x40, 0x01, 0x00, 0x00}};
CAN_frame EGMP_2B5 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2B5,
    .data = {0xbd, 0xb2, 0x42, 0x00, 0x00, 0x00, 0x00, 0x80, 0x59, 0x00, 0x2b, 0x00, 0x00, 0x04, 0x00, 0x00,
             0xfa, 0xd0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_2E0 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2E0,
    .data = {0xc1, 0xf2, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x70, 0x01, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_2D5 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2D5,
    .data = {0x79, 0xfb, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_27A = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x27A,
    .data = {0x8f, 0x99, 0x41, 0xf1, 0x1b, 0x0d, 0x00, 0xfe, 0x00, 0x15, 0x10, 0x8e, 0xc9, 0x02, 0x2c, 0x01,
             0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_2EA = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2EA,
    .data = {0x6e, 0xbb, 0xa0, 0x0d, 0x04, 0x01, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0xc7, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_306 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x306,
    .data = {0x00, 0x00, 0x00, 0xd2, 0x06, 0x92, 0x05, 0x34, 0x07, 0x8e, 0x08, 0x73, 0x05, 0x80, 0x05, 0x83,
             0x05, 0x73, 0x05, 0x80, 0x05, 0xed, 0x01, 0xdd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_308 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x308,
    .data = {0xbe, 0x84, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0xff, 0x75, 0x6c, 0x86, 0x0d, 0xfb, 0xdf, 0x03, 0x36, 0xc3, 0x86, 0x11, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_33A = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x33A,
    .data = {0x1a, 0x23, 0x26, 0x10, 0x27, 0x4f, 0x06, 0x00, 0xf8, 0x1b, 0x19, 0x04, 0x30, 0x01, 0x00, 0x06,
             0x00, 0x00, 0x00, 0x2e, 0x2d, 0x81, 0x25, 0x20, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_350 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x350,
    .data = {0x26, 0x82, 0x26, 0xf4, 0x01, 0x00, 0x00, 0x50, 0x90, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_2E5 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2E5,
    .data = {0x69, 0x8a, 0x3f, 0x01, 0x00, 0x00, 0x00, 0x15, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_255 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x255,
    .data = {0x49, 0x1a, 0x3f, 0x15, 0x00, 0x00, 0x80, 0x01, 0x00, 0x96, 0x00, 0x28, 0x77, 0x07, 0x06, 0x96,
             0x00, 0xbf, 0x1b, 0x00, 0x30, 0x15, 0x00, 0x24, 0xaf, 0x05, 0x92, 0x18, 0x44, 0x02, 0x00, 0x00}};
CAN_frame EGMP_3B5 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x3B5,
    .data = {0xa3, 0xc8, 0x9f, 0x00, 0x00, 0x00, 0x00, 0x36, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0xc7, 0x02, 0x00, 0x00, 0x00, 0x00, 0x6a, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_2C0 = {
    .FD = true,
    .ext_ID = false,
    .DLC = 32,
    .ID = 0x2C0,
    .data = {0xcc, 0xcd, 0xa2, 0x21, 0x00, 0xa1, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/* These messages are rest of the vehicle messages, to reduce number of active fault codes */
CAN_frame EGMP_1CF = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x1CF,
                      .data = {0x56, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_3AA = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3AA,
                      .data = {0xFF, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00}};
CAN_frame EGMP_3E0 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3E0,
                      .data = {0xC3, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_3E1 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3E1,
                      .data = {0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_36F = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x36F,
                      .data = {0x28, 0x31, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_37F = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x37F,
                      .data = {0x9B, 0x30, 0x52, 0x24, 0x41, 0x02, 0x00, 0x00}};
CAN_frame EGMP_4B4 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4B4,
                      .data = {0x00, 0x00, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4B5 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4B5,
                      .data = {0x08, 0x00, 0xF0, 0x07, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4B7 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4B7,
                      .data = {0x08, 0x00, 0xF0, 0x07, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4CC = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4CC,
                      .data = {0x08, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4CE = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4CE,
                      .data = {0x16, 0xCF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame EGMP_4D8 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4D8,
                      .data = {0x40, 0x10, 0xF0, 0xF0, 0x40, 0xF2, 0x1E, 0xCC}};
CAN_frame EGMP_4DD = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4DD,
                      .data = {0x3F, 0xFC, 0xFF, 0x00, 0x38, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4E7 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4E7,
                      .data = {0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00}};
CAN_frame EGMP_4E9 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4E9,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4EA = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4EA,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4EB = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4EB,
                      .data = {0x01, 0x50, 0x0B, 0x26, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4EC = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4EC,
                      .data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F}};
CAN_frame EGMP_4ED = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4ED,
                      .data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F}};
CAN_frame EGMP_4EE = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4EE,
                      .data = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4EF = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4EF,
                      .data = {0x2B, 0xFE, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}};
CAN_frame EGMP_405 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x405,
                      .data = {0xE4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_410 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x410,
                      .data = {0xA6, 0x10, 0xFF, 0x3C, 0xFF, 0x7F, 0xFF, 0xFF}};
CAN_frame EGMP_411 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x411,
                      .data = {0xEA, 0x22, 0x50, 0x51, 0x00, 0x00, 0x00, 0x40}};
CAN_frame EGMP_412 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x412,
                      .data = {0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_413 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x413,
                      .data = {0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_414 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x414,
                      .data = {0xF0, 0x10, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00}};
CAN_frame EGMP_416 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x416,
                      .data = {0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_417 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x417,
                      .data = {0xC7, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}};
CAN_frame EGMP_418 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x418,
                      .data = {0x17, 0x20, 0x00, 0x00, 0x14, 0x0C, 0x00, 0x00}};
CAN_frame EGMP_3C1 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3C1,
                      .data = {0x59, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_3C2 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3C2,
                      .data = {0x07, 0x00, 0x11, 0x40, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_4F0 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4F0,
                      .data = {0x8A, 0x0A, 0x0D, 0x34, 0x60, 0x18, 0x12, 0xFC}};
CAN_frame EGMP_4F2 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4F2,
                      .data = {0x0A, 0xC3, 0xD5, 0xFF, 0x0F, 0x21, 0x80, 0x2B}};
CAN_frame EGMP_4FE = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4FE,
                      .data = {0x69, 0x3F, 0x00, 0x04, 0xDF, 0x01, 0x4C, 0xA8}};
CAN_frame EGMP_48F = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x48F,
                      .data = {0xAD, 0x10, 0x41, 0x00, 0x05, 0x00, 0x00, 0x00}};
CAN_frame EGMP_419 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x419,
                      .data = {0xC7, 0x90, 0xB9, 0xD2, 0x0D, 0x62, 0x7A, 0x00}};
CAN_frame EGMP_422 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x422,
                      .data = {0x15, 0x10, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_444 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x444,
                      .data = {0x96, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame EGMP_641 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x641,
                      .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF}};
CAN_frame EGMP_7E4 = {.FD = true,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x7E4,
                      .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 01
CAN_frame EGMP_7E4_ack = {
    .FD = true,
    .ext_ID = false,
    .DLC = 8,
    .ID = 0x7E4,
    .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Ack frame, correct PID is returned

void set_cell_voltages(CAN_frame rx_frame, int start, int length, int startCell) {
  for (size_t i = 0; i < length; i++) {
    if ((rx_frame.data.u8[start + i] * 20) > 1000) {
      datalayer.battery.status.cell_voltages_mV[startCell + i] = (rx_frame.data.u8[start + i] * 20);
    }
  }
}

static uint8_t calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0)

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //datalayer.battery.status.max_charge_power_W = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //The allowed charge power is not available. We hardcode this value for now
  datalayer.battery.status.max_charge_power_W = MAXCHARGEPOWERALLOWED;

  //datalayer.battery.status.max_discharge_power_W = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts
  //The allowed discharge power is not available. We hardcode this value for now
  datalayer.battery.status.max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;

  powerWatt = ((batteryVoltage * batteryAmps) / 100);

  datalayer.battery.status.active_power_W = powerWatt;  //Power in watts, Negative = charging batt

  datalayer.battery.status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery.status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!datalayer.battery.status.CAN_battery_still_alive) {
    set_event(EVENT_CANFD_RX_FAILURE, 0);
  } else {
    datalayer.battery.status.CAN_battery_still_alive--;
    clear_event(EVENT_CANFD_RX_FAILURE);
  }

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Check if cell voltages are within allowed range
  if (CellVoltMax_mV >= MAX_CELL_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (CellVoltMin_mV <= MIN_CELL_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }

  /* Safeties verified. Perform USB serial printout if configured to do so */

#ifdef DEBUG_VIA_USB
  Serial.println();  //sepatator
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
  Serial.print((uint16_t)SOC_BMS / 10.0, 1);
  Serial.print("%  |  SOC Display: ");
  Serial.print((uint16_t)SOC_Display / 10.0, 1);
  Serial.print("%  |  SOH ");
  Serial.print((uint16_t)batterySOH / 10.0, 1);
  Serial.println("%");
  Serial.print((int16_t)batteryAmps / 10.0, 1);
  Serial.print(" Amps  |  ");
  Serial.print((uint16_t)batteryVoltage / 10.0, 1);
  Serial.print(" Volts  |  ");
  Serial.print((int16_t)datalayer.battery.status.active_power_W);
  Serial.println(" Watts");
  Serial.print("Allowed Charge ");
  Serial.print((uint16_t)allowedChargePower * 10);
  Serial.print(" W  |  Allowed Discharge ");
  Serial.print((uint16_t)allowedDischargePower * 10);
  Serial.println(" W");
  Serial.print("MaxCellVolt ");
  Serial.print(CellVoltMax_mV);
  Serial.print(" mV  No  ");
  Serial.print(CellVmaxNo);
  Serial.print("  |  MinCellVolt ");
  Serial.print(CellVoltMin_mV);
  Serial.print(" mV  No  ");
  Serial.println(CellVminNo);
  Serial.print("TempHi ");
  Serial.print((int16_t)temperatureMax);
  Serial.print("°C  TempLo ");
  Serial.print((int16_t)temperatureMin);
  Serial.print("°C  WaterInlet ");
  Serial.print((int8_t)temperature_water_inlet);
  Serial.print("°C  PowerRelay ");
  Serial.print((int8_t)powerRelayTemperature * 2);
  Serial.println("°C");
  Serial.print("Aux12volt: ");
  Serial.print((int16_t)leadAcidBatteryVoltage / 10.0, 1);
  Serial.println("V  |  ");
  Serial.print("BmsManagementMode ");
  Serial.print((uint8_t)batteryManagementMode, BIN);
  if (bitRead((uint8_t)BMS_ign, 2) == 1) {
    Serial.print("  |  BmsIgnition ON");
  } else {
    Serial.print("  |  BmsIgnition OFF");
  }

  if (bitRead((uint8_t)batteryRelay, 0) == 1) {
    Serial.print("  |  PowerRelay ON");
  } else {
    Serial.print("  |  PowerRelay OFF");
  }
  Serial.print("  |  Inverter ");
  Serial.print(inverterVoltage);
  Serial.println(" Volts");
#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x055:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x215:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x21A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x235:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x245:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x25A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x275:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2FA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x325:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x330:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x335:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x365:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3BA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:
      // print_canfd_frame(frame);
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          // Serial.println ("Send ack");
          poll_data_pid = rx_frame.data.u8[4];
          // if (rx_frame.data.u8[4] == poll_data_pid) {
          transmit_can(&EGMP_7E4_ack, can_config.battery);  //Send ack to BMS if the same frame is sent as polled
          // }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
            allowedChargePower = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]);
            allowedDischargePower = ((rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6]);
            SOC_BMS = rx_frame.data.u8[2] * 5;  //100% = 200 ( 200 * 5 = 1000 )

          } else if (poll_data_pid == 2) {
            // set cell voltages data, start bite, data length from start, start cell
            set_cell_voltages(rx_frame, 2, 6, 0);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 2, 6, 32);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 2, 6, 64);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 2, 6, 96);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 2, 6, 128);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 2, 6, 160);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 1) {
            batteryVoltage = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
            batteryAmps = (rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2];
            temperatureMax = rx_frame.data.u8[5];
            temperatureMin = rx_frame.data.u8[6];
            // temp1 = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 6);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 38);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 70);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 102);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 134);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 166);
          } else if (poll_data_pid == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
            // temp2 = rx_frame.data.u8[1];
            // temp3 = rx_frame.data.u8[2];
            // temp4 = rx_frame.data.u8[3];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 13);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 45);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 77);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 109);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 141);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 173);
          } else if (poll_data_pid == 5) {
            // ac = rx_frame.data.u8[3];
            // Vdiff = rx_frame.data.u8[4];

            // airbag = rx_frame.data.u8[6];
            heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
            CellVminNo = rx_frame.data.u8[3];
            // fanMod = rx_frame.data.u8[4];
            // fanSpeed = rx_frame.data.u8[5];
            leadAcidBatteryVoltage = rx_frame.data.u8[6];  //12v Battery Volts
            //cumulative_charge_current[0] = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 20);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 52);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 84);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 116);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 148);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 180);
          } else if (poll_data_pid == 5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
            // maxDetCell = rx_frame.data.u8[4];
            // minDet = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6];
            // minDetCell = rx_frame.data.u8[7];
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_charge_current[1] = rx_frame.data.u8[1];
            //cumulative_charge_current[2] = rx_frame.data.u8[2];
            //cumulative_charge_current[3] = rx_frame.data.u8[3];
            //cumulative_discharge_current[0] = rx_frame.data.u8[4];
            //cumulative_discharge_current[1] = rx_frame.data.u8[5];
            //cumulative_discharge_current[2] = rx_frame.data.u8[6];
            //cumulative_discharge_current[3] = rx_frame.data.u8[7];
            //set_cumulative_charge_current();
            //set_cumulative_discharge_current();
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 5, 27);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 5, 59);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 5, 91);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 5, 123);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 5, 155);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 5, 187);
            //set_cell_count();
          } else if (poll_data_pid == 5) {
            // datalayer.battery.info.number_of_cells = 98;
            SOC_Display = rx_frame.data.u8[1] * 5;
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_charged[0] = rx_frame.data.u8[1];
            // cumulative_energy_charged[1] = rx_frame.data.u8[2];
            //cumulative_energy_charged[2] = rx_frame.data.u8[3];
            //cumulative_energy_charged[3] = rx_frame.data.u8[4];
            //cumulative_energy_discharged[0] = rx_frame.data.u8[5];
            //cumulative_energy_discharged[1] = rx_frame.data.u8[6];
            //cumulative_energy_discharged[2] = rx_frame.data.u8[7];
            // set_cumulative_energy_charged();
          }
          break;
        case 0x27:  //Seventh datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_discharged[3] = rx_frame.data.u8[1];

            //opTimeBytes[0] = rx_frame.data.u8[2];
            //opTimeBytes[1] = rx_frame.data.u8[3];
            //opTimeBytes[2] = rx_frame.data.u8[4];
            //opTimeBytes[3] = rx_frame.data.u8[5];

            BMS_ign = rx_frame.data.u8[6];
            inverterVoltageFrameHigh = rx_frame.data.u8[7];  // BMS Capacitoir

            // set_cumulative_energy_discharged();
            // set_opTime();
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (poll_data_pid == 1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];  // BMS Capacitoir
          }
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {

  unsigned long currentMillis = millis();

  //Send 10ms CANFD message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    EGMP_F5.data.u8[0] = EGMP_F5_byte0[counter_10ms];
    EGMP_F5.data.u8[1] = EGMP_F5_byte1[counter_10ms];
    EGMP_F5.data.u8[2] = EGMP_counter_byte2[counter_10ms];

    EGMP_10A.data.u8[0] = EGMP_10A_byte0[counter_10ms];
    EGMP_10A.data.u8[1] = EGMP_10A_byte1[counter_10ms];
    EGMP_10A.data.u8[2] = EGMP_counter_byte2[counter_10ms];

    EGMP_120.data.u8[0] = EGMP_120_byte0[counter_10ms];
    EGMP_120.data.u8[1] = EGMP_120_byte1[counter_10ms];
    EGMP_120.data.u8[2] = EGMP_counter_byte2[counter_10ms];

    EGMP_35.data.u8[0] = EGMP_35_byte0[counter_10ms];
    EGMP_35.data.u8[1] = EGMP_35_byte1[counter_10ms];
    EGMP_35.data.u8[2] = EGMP_counter_byte2[counter_10ms];

    EGMP_19A.data.u8[0] = EGMP_19A_byte0[counter_10ms];
    EGMP_19A.data.u8[1] = EGMP_19A_byte1[counter_10ms];
    EGMP_19A.data.u8[2] = EGMP_19A_byte2[counter_10ms];

    counter_10ms++;
    if (counter_10ms > 15) {
      counter_10ms = 0;
    }

    transmit_can(&EGMP_F5, can_config.battery);   // Needed for contactor closing
    transmit_can(&EGMP_10A, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_120, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_19A, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_35, can_config.battery);   // Needed for contactor closing
  }

  //Send 20ms CANFD message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    previousMillis20ms = currentMillis;

    EGMP_1CF.data.u8[1] = (EGMP_1CF_counter % 15) * 0x10;
    EGMP_1CF_counter++;
    if (EGMP_1CF_counter > 0xE) {
      EGMP_1CF_counter = 0;
    }
    EGMP_1CF.data.u8[0] = calculateCRC(EGMP_1CF, EGMP_1CF.DLC, 0x0A);  // Set CRC bit, initial Value 0x0A

    transmit_can(&EGMP_1CF, can_config.battery);
  }

  //Send 30ms CANFD message
  if (currentMillis - previousMillis30ms >= INTERVAL_30_MS) {
    previousMillis30ms = currentMillis;

    transmit_can(&EGMP_419, can_config.battery);  // TODO: Handle variations better
  }

  //Send 100ms CANFD message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    EGMP_36F.data.u8[1] = ((EGMP_3XF_counter % 15) << 4) + 0x01;
    EGMP_37F.data.u8[1] = ((EGMP_3XF_counter % 15) << 4);
    EGMP_3XF_counter++;
    if (EGMP_3XF_counter > 0xE) {
      EGMP_3XF_counter = 0;
    }
    EGMP_36F.data.u8[0] = calculateCRC(EGMP_36F, EGMP_36F.DLC, 0x8A);  // Set CRC bit, initial Value 0x8A
    EGMP_37F.data.u8[0] = calculateCRC(EGMP_37F, EGMP_37F.DLC, 0x38);  // Set CRC bit, initial Value 0x38

    transmit_can(&EGMP_30A, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_320, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_2AA, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_2B5, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_2E0, can_config.battery);  // Needed for contactor closing
    transmit_can(&EGMP_2D5, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_27A, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_2EA, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_306, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_308, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_33A, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_350, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_2E5, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_255, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_3B5, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_2C0, can_config.battery);  // Needed for contactor closing (UNSURE IF THIS IS 100ms)
    transmit_can(&EGMP_36F, can_config.battery);
    transmit_can(&EGMP_37F, can_config.battery);
  }

  //Send 200ms CANFD message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    transmit_can(&EGMP_4B4, can_config.battery);
    transmit_can(&EGMP_4B5, can_config.battery);
    transmit_can(&EGMP_4B7, can_config.battery);
    transmit_can(&EGMP_4CC, can_config.battery);
    transmit_can(&EGMP_4CE, can_config.battery);
    transmit_can(&EGMP_4D8, can_config.battery);
    transmit_can(&EGMP_4DD, can_config.battery);
    transmit_can(&EGMP_4E7, can_config.battery);
    transmit_can(&EGMP_4E9, can_config.battery);
    transmit_can(&EGMP_4EA, can_config.battery);
    transmit_can(&EGMP_4EB, can_config.battery);
    transmit_can(&EGMP_4EC, can_config.battery);
    transmit_can(&EGMP_4ED, can_config.battery);
    transmit_can(&EGMP_4EE, can_config.battery);
    transmit_can(&EGMP_4EF, can_config.battery);
    transmit_can(&EGMP_641, can_config.battery);
    transmit_can(&EGMP_3AA, can_config.battery);
    transmit_can(&EGMP_3E0, can_config.battery);
    transmit_can(&EGMP_3E1, can_config.battery);
    transmit_can(&EGMP_422, can_config.battery);
    transmit_can(&EGMP_444, can_config.battery);
    transmit_can(&EGMP_405, can_config.battery);
    transmit_can(&EGMP_410, can_config.battery);
    transmit_can(&EGMP_411, can_config.battery);
    transmit_can(&EGMP_412, can_config.battery);
    transmit_can(&EGMP_412, can_config.battery);
    transmit_can(&EGMP_413, can_config.battery);
    transmit_can(&EGMP_414, can_config.battery);
    transmit_can(&EGMP_416, can_config.battery);
    transmit_can(&EGMP_417, can_config.battery);
    transmit_can(&EGMP_418, can_config.battery);
    transmit_can(&EGMP_3C1, can_config.battery);
    transmit_can(&EGMP_3C2, can_config.battery);
    transmit_can(&EGMP_4F0, can_config.battery);  //TODO: could be handled better
    transmit_can(&EGMP_4F2, can_config.battery);  //TODO: could be handled better

    if (ticks_200ms_counter < 254) {
      ticks_200ms_counter++;
    }
    if (ticks_200ms_counter > 11) {
      EGMP_412.data.u8[0] = 0x48;
      EGMP_412.data.u8[1] = 0x10;
      EGMP_412.data.u8[6] = 0x04;

      EGMP_418.data.u8[0] = 0xCE;
      EGMP_418.data.u8[1] = 0x30;
      EGMP_418.data.u8[4] = 0x14;
      EGMP_418.data.u8[5] = 0x4C;
      if (ticks_200ms_counter > 39) {
        EGMP_412.data.u8[0] = 0xB3;
        EGMP_412.data.u8[1] = 0x20;
        EGMP_412.data.u8[6] = 0x00;

        EGMP_418.data.u8[0] = 0xA6;
        EGMP_418.data.u8[1] = 0x40;
        EGMP_418.data.u8[5] = 0x0C;
      }
    }
    if (ticks_200ms_counter > 20) {
      EGMP_413.data.u8[0] = 0xF5;
      EGMP_413.data.u8[1] = 0x10;
      EGMP_413.data.u8[3] = 0x41;
    }
    if (ticks_200ms_counter > 28) {
      EGMP_4B4.data.u8[2] = 0;
      EGMP_4B4.data.u8[3] = 0;
    }
    if (ticks_200ms_counter > 26) {
      EGMP_411.data.u8[0] = 0x9E;
      EGMP_411.data.u8[1] = 0x32;
      EGMP_411.data.u8[7] = 0x50;

      EGMP_417.data.u8[0] = 0x9E;
      EGMP_417.data.u8[1] = 0x20;
      EGMP_417.data.u8[4] = 0x04;
      EGMP_417.data.u8[5] = 0x01;
    }
    if (ticks_200ms_counter > 32) {
      EGMP_4CE.data.u8[0] = 0x22;
      EGMP_4CE.data.u8[1] = 0x41;
      EGMP_4CE.data.u8[6] = 0x47;
      EGMP_4CE.data.u8[7] = 0x1F;
    }
    if (ticks_200ms_counter > 43) {
      EGMP_4EB.data.u8[2] = 0x0D;
      EGMP_4EB.data.u8[3] = 0x3B;
    }
    if (ticks_200ms_counter > 46) {
      EGMP_4EB.data.u8[2] = 0x0E;
      EGMP_4EB.data.u8[3] = 0x00;
    }
    if (ticks_200ms_counter > 24) {
      EGMP_4ED.data.u8[1] = 0x00;
      EGMP_4ED.data.u8[2] = 0x00;
      EGMP_4ED.data.u8[3] = 0x00;
      EGMP_4ED.data.u8[4] = 0x00;
    }
    if (ticks_200ms_counter > 21) {
      EGMP_3E1.data.u8[0] = 0x49;
      EGMP_3E1.data.u8[1] = 0x10;
      EGMP_3E1.data.u8[2] = 0x12;
      EGMP_3E1.data.u8[3] = 0x15;

      EGMP_422.data.u8[0] = 0xEE;
      EGMP_422.data.u8[1] = 0x20;
      EGMP_422.data.u8[2] = 0x11;
      EGMP_422.data.u8[6] = 0x04;

      EGMP_405.data.u8[0] = 0xD2;
      EGMP_405.data.u8[1] = 0x10;
      EGMP_405.data.u8[5] = 0x01;
    }
    if (ticks_200ms_counter > 12) {
      EGMP_444.data.u8[0] = 0xEE;
      EGMP_444.data.u8[1] = 0x30;
      EGMP_444.data.u8[3] = 0x20;
      if (ticks_200ms_counter > 23) {  // TODO: Could be handled better
        EGMP_444.data.u8[0] = 0xE4;
        EGMP_444.data.u8[1] = 0x60;
        EGMP_444.data.u8[2] = 0x25;
        EGMP_444.data.u8[3] = 0x4E;
        EGMP_444.data.u8[4] = 0x04;
      }
    }
  }

  //Send 500ms CANFD message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {

    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis500ms >= INTERVAL_500_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis500ms));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis500ms = currentMillis;

    EGMP_7E4.data.u8[3] = KIA_7E4_COUNTER;
    transmit_can(&EGMP_7E4, can_config.battery);

    KIA_7E4_COUNTER++;
    if (KIA_7E4_COUNTER > 0x0D) {  // gets up to 0x010C before repeating
      KIA_7E4_COUNTER = 0x01;
    }
  }
  //Send 1s CANFD message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    transmit_can(&EGMP_48F, can_config.battery);
  }
  //Send 2s CANFD message
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;

    transmit_can(&EGMP_4FE, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Hyundai E-GMP (Electric Global Modular Platform) battery selected");
#endif

  datalayer.system.status.battery_allows_contactor_closing = true;

  datalayer.battery.info.number_of_cells = 192;  // TODO: will vary depending on battery

  datalayer.battery.info.max_design_voltage_dV =
      8064;  // TODO: define when battery is known, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV =
      4320;  // TODO: define when battery is known. discharging further is disabled
}

#endif
