#include "../include.h"
#ifdef GROWATT_CAN
#include "../datalayer/datalayer.h"
#include "GROWATT-CAN.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100ms = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis500ms = 0;   // will store last time a 500ms CAN Message was send
static unsigned long previousMillis1000ms = 0;   // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis2s = 0;   // will store last time a 2s CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 10s CAN Message was send
static unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

#define STACK_SUM 1
#define CLUSTER_SUM 1
#define SINGLE_CLUSTER_MODULE_NUMBER 1
#define SINGLE_MODULE_CELL_NUMBER 1
#define MODULE_RATED_VOLTAGE 4.2
#define MODULE_RATED_AH 10

CAN_frame GRW_1AC0 = {.FD = false,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x1AC0F1F3,
                     .data = {STACK_SUM, CLUSTER_SUM,
			SINGLE_CLUSTER_MODULE_NUMBER, SINGLE_MODULE_CELL_NUMBER,
			((uint16_t)(MODULE_RATED_VOLTAGE / 0.01)) >> 8,
			((uint16_t)(MODULE_RATED_VOLTAGE / 0.01)) & 0xFF,
			((uint16_t)(MODULE_RATED_AH / 0.1)) >> 8,
			((uint16_t)(MODULE_RATED_AH / 0.1)) && 0xFF}}; //Battery system composition information

CAN_frame GRW_1AC2 = {.FD = false,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x1AC2F1F3,
                     .data = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

CAN_frame GRW_1AC3 = {.FD = false,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x1AC3F1F3,
                     .data = {0x00, 0xFF, 0x00, 0xFF, 
			((uint16_t)(440 / 0.1)) >> 8,
			((uint16_t)(440 / 0.1)) & 0xFF,
			((uint16_t)(360 / 0.1)) >> 8,
			((uint16_t)(360 / 0.1)) & 0xFF
		     }};

CAN_frame GRW_1AC4 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AC4F1F3,
                       .data = {
			((uint16_t)(4.2 / 0.001)) >> 8,
			((uint16_t)(4.2 / 0.001)) & 0xFF,
			((uint16_t)(3.6 / 0.001)) >> 8,
			((uint16_t)(3.6 / 0.001)) & 0xFF,
			((uint16_t)(430 / 0.1)) >> 8,
			((uint16_t)(430 / 0.1)) & 0xFF,
			0x00, 0x00}};
CAN_frame GRW_1AC5 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AC5F1F3,
                       .data = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}};  CAN_frame GRW_1AC6 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AC6,
                       .data = {75, 99, 
			((uint16_t)(133 / 0.1)) >> 8,
			((uint16_t)(133 / 0.1)) & 0xFF,
			((uint16_t)(460 / 0.1)) >> 8,
			((uint16_t)(460 / 0.1)) & 0xFF}};
CAN_frame GRW_1AC7 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AC7F1F3,
                       .data = {
			((uint16_t)(460 / 0.1)) >> 8,
			((uint16_t)(460 / 0.1)) & 0xFF,
			((uint16_t)(460 / 0.1)) >> 8,
			((uint16_t)(460 / 0.1)) & 0xFF,
			((uint16_t)(0 / 0.1 + 1000)) >> 8,
			((uint16_t)(0 / 0.1 + 1000)) & 0xFF,
			((uint16_t)(66 / 0.1)) >> 8,
			((uint16_t)(66 / 0.1)) & 0xFF}};
CAN_frame GRW_1AC8 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AC8F1F3,
                       .data = {'Z','Y','A','B', 0, 1, 1 ,1}};
CAN_frame GRW_1AC9 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AC9,
                       .data = {1,1,1,0, 
			((uint16_t)(4.5 / 0.01)) >> 8,
			((uint16_t)(4.5 / 0.01)) & 0xFF,
			((uint16_t)(4.45 / 0.01)) >> 8,
			((uint16_t)(4.45 / 0.01)) & 0xFF}};
CAN_frame GRW_1ACA = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1ACAF1F3,
                       .data = {1,1,2,0, 
			((uint16_t)(4.4 / 0.01)) >> 8,
			((uint16_t)(4.4 / 0.01)) & 0xFF,
			0x00, 0x00}};
CAN_frame GRW_1ACB = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1ACBF1F3,
                       .data = {1,1,1,1,1, 10,  
			((uint16_t)(13 / 0.1 + 40)) >> 8,
			((uint16_t)(13 / 0.1 + 40)) & 0xFF,
			((uint16_t)(13.1 / 0.1 + 40)) >> 8,
			((uint16_t)(13.1 / 0.1 + 40)) & 0xFF}};
CAN_frame GRW_1ACC = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1ACCF1F3,
                       .data = {1,1,1,1,1, 12,  
			((uint16_t)(14 / 0.1 + 40)) >> 8,
			((uint16_t)(14 / 0.1 + 40)) & 0xFF,
			((uint16_t)(13.5 / 0.1 + 40)) >> 8,
			((uint16_t)(13.5 / 0.1 + 40)) & 0xFF}};
CAN_frame GRW_1ACD = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1ACDF1F3,
                       .data = {1,1,1,1,1, 13,  
			((uint16_t)(12 / 0.1 + 40)) >> 8,
			((uint16_t)(12 / 0.1 + 40)) & 0xFF,
			((uint16_t)(13 / 0.1 + 40)) >> 8,
			((uint16_t)(13 / 0.1 + 40)) & 0xFF}};
CAN_frame GRW_1ACE = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1ACEF1F3,
                       .data = {1,1,1,1,1, 13,  
			((uint16_t)(4.05 / 0.001)) >> 8,
			((uint16_t)(4.05 / 0.001)) & 0xFF,
			((uint16_t)(4.0 / 0.001)) >> 8,
			((uint16_t)(4.0 / 0.001)) & 0xFF}};
CAN_frame GRW_1ACF = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1ACFF1F3,
                       .data = {1,1,1,1,1, 13,  
			((uint16_t)(3.95 / 0.001)) >> 8,
			((uint16_t)(3.95 / 0.001)) & 0xFF,
			0x00, 0x00}};
CAN_frame GRW_1AD0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AD0F1F3,
                       .data = {1,1,1,1,  
			75, 75, 75, 0x00}};
CAN_frame GRW_1AD1 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AD1F1F3,
                       .data = {0x00,0x00,0x00,
				0x00,0x00,0x00,
			((uint16_t)(0 + 3200)) >> 8,
			((uint16_t)(0 + 3200)) & 0xFF}};
CAN_frame GRW_1AD8 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AD8F1F3,
                       .data = {0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00}};
CAN_frame GRW_1AD9 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1AD9F1F3,
                       .data = {0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00}};
CAN_frame GRW_1A74 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x1A74F1F3,
                       .data = {
			((uint16_t)(3600)) >> 8,
			((uint16_t)(3600)) & 0xFF,
			0x00,0x00,0x00,0x00,0x00,0x00}};


static uint8_t inverter_name[7] = {0};
static int16_t temperature_average = 0;
static uint16_t inverter_voltage = 0;
static uint16_t inverter_SOC = 0;
static int16_t inverter_current = 0;
static int16_t inverter_temperature = 0;
static uint16_t remaining_capacity_ah = 0;
static uint16_t fully_charged_capacity_ah = 0;
static long inverter_timestamp = 0;
static bool initialDataSent = 0;

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
#if 0
  /* Calculate temperature */
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  /* Calculate capacity, Amp hours(Ah) = Watt hours (Wh) / Voltage (V)*/
  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    remaining_capacity_ah =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
    fully_charged_capacity_ah =
        ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
  }

  //Map values to CAN messages
  //Maxvoltage (eg 400.0V = 4000 , 16bits long)
  BYD_110.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  BYD_110.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  //Minvoltage (eg 300.0V = 3000 , 16bits long)
  BYD_110.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  BYD_110.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  //Maximum discharge power allowed (Unit: A+1)
  BYD_110.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  BYD_110.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Maximum charge power allowed (Unit: A+1)
  BYD_110.data.u8[6] = (datalayer.battery.status.max_charge_current_dA >> 8);
  BYD_110.data.u8[7] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

  //SOC (100.00%)
  BYD_150.data.u8[0] = (datalayer.battery.status.reported_soc >> 8);
  BYD_150.data.u8[1] = (datalayer.battery.status.reported_soc & 0x00FF);
  //StateOfHealth (100.00%)
  BYD_150.data.u8[2] = (datalayer.battery.status.soh_pptt >> 8);
  BYD_150.data.u8[3] = (datalayer.battery.status.soh_pptt & 0x00FF);
  //Remaining capacity (Ah+1)
  BYD_150.data.u8[4] = (remaining_capacity_ah >> 8);
  BYD_150.data.u8[5] = (remaining_capacity_ah & 0x00FF);
  //Fully charged capacity (Ah+1)
  BYD_150.data.u8[6] = (fully_charged_capacity_ah >> 8);
  BYD_150.data.u8[7] = (fully_charged_capacity_ah & 0x00FF);

  //Voltage (ex 370.0)
  BYD_1D0.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  BYD_1D0.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Current (ex 81.0A)
  BYD_1D0.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  BYD_1D0.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Temperature average
  BYD_1D0.data.u8[4] = (temperature_average >> 8);
  BYD_1D0.data.u8[5] = (temperature_average & 0x00FF);

  //Temperature max
  BYD_210.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  BYD_210.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //Temperature min
  BYD_210.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  BYD_210.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
#endif
#ifdef DEBUG_VIA_USB
  if (inverter_name[0] != 0) {
    Serial.print("Detected inverter: ");
    for (uint16_t i = 0; i < 7; i++) {
      Serial.print((char)inverter_name[i]);
    }
    Serial.println();
  }
#endif
}

void receive_can_inverter(CAN_frame rx_frame) {
#ifdef DEBUG_VIA_USB
    Serial.println("Message from inverter received");
#endif
  switch (rx_frame.ID&0xFFFF0000) {
    case 0x1ABEDFF1&0xFFFF0000 :  //Message originating from Growatt inverter.
#ifdef DEBUG_VIA_USB
    Serial.println("Message 1ABE from inverter received");
#endif
      transmit_can(&GRW_1AC2, can_config.inverter);
      break;
    default:
      break;
  }

}

void send_can_inverter() {
  unsigned long currentMillis = millis();
  // Send initial CAN data once on bootup
  if (!initialDataSent) {
    send_intial_data();
    initialDataSent = 1;
  }

  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can(&GRW_1AC3, can_config.inverter);
    transmit_can(&GRW_1AC4, can_config.inverter);
    transmit_can(&GRW_1AC5, can_config.inverter);
    transmit_can(&GRW_1AC7, can_config.inverter);
    transmit_can(&GRW_1ACE, can_config.inverter);
    transmit_can(&GRW_1ACF, can_config.inverter);
    transmit_can(&GRW_1AD8, can_config.inverter);
    transmit_can(&GRW_1AD9, can_config.inverter);
  }
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can(&GRW_1AC6, can_config.inverter);
    transmit_can(&GRW_1AC8, can_config.inverter);
    transmit_can(&GRW_1A74, can_config.inverter);
  }
  if (currentMillis - previousMillis1000ms >= INTERVAL_1_S) {
    previousMillis1000ms = currentMillis;

    transmit_can(&GRW_1AC9, can_config.inverter);
    transmit_can(&GRW_1ACA, can_config.inverter);
    transmit_can(&GRW_1ACB, can_config.inverter);
    transmit_can(&GRW_1ACC, can_config.inverter);
    transmit_can(&GRW_1ACD, can_config.inverter);
    transmit_can(&GRW_1AD0, can_config.inverter);
    transmit_can(&GRW_1AD1, can_config.inverter);
  }

  // Send 2s CAN Message
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;

    transmit_can(&GRW_1AC0, can_config.inverter);
  }
}

void send_intial_data() {
}

void setup_inverter(void) {  // Performs one time setup at startup over CAN bus
//  strncpy(datalayer.system.info.inverter_protocol, "BYD Battery-Box Premium HVS over CAN Bus", 63);
//  datalayer.system.info.inverter_protocol[63] = '\0';
}
#endif
