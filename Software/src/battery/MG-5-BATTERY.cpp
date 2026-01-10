#include "MG-5-BATTERY.h"
#include <Arduino.h>
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

//#ifndef SMALL_FLASH_DEVICE

/* TODO: 
- Get contactor closing working
- Figure out which CAN messages need to be sent towards the battery to keep it alive
- Map all values from battery CAN messages
- Most important ones 
*/

inline const char* getBMStatus(int index) {
  switch (index) {
    case 0:
      return "INVALID";
    case 1:
      return "DISCONNECTED";
    case 2:
      return "PRECHARGE";
    case 3:
      return "CONNECTED";
    //case 6:
    //  return "AC CHARGING";
    //case 7:
    // return "DC CHARGING";
    case 15:
      return "ISOLATION FAULT";
    default:
      return "UNKNOWN";
  }
}

void Mg5Battery::update_soc(uint16_t soc_times_ten) {

#if MG5_USE_FULL_CAPACITY
  // The SoC hits 100% at 4.1V/cell. To get the full 4.2V/cell we need to use
  // voltage instead for the last bit.

  if (cellVoltageValidTime == 0) {
    // We don't have a recent cell max voltage reading, so can't do
    // voltage-based SoC.
  } else if (soc_times_ten > 900 && datalayer.battery.status.cell_max_voltage_mV < 4000) {
    // Something is wrong with our max cell voltage reading (it is too low), so
    // don't trust it - we'll just let the SoC hit 100%.
  } else if (soc_times_ten == 1000 && datalayer.battery.status.cell_max_voltage_mV >= 4100) {
    // We've hit 100%, so use voltage-based-SoC calculation for the last bit.

    // We usually hit 92% at ~369V, and the pack max is 378V.

    // Scale so that 100% becomes 92%
    soc_times_ten = (uint16_t)(((uint32_t)soc_times_ten * 9200) / 10000);

    // Add on the last 100mV as the last 8% of SoC.
    soc_times_ten += (uint16_t)((((uint32_t)datalayer.battery.status.cell_max_voltage_mV - 4100) * 800) / 1000);
    if (soc_times_ten > 1000) {
      soc_times_ten = 1000;  // Don't let it go above 100%
    }
  } else {
    // Scale so that 100% becomes 92%
    soc_times_ten = (uint16_t)(((uint32_t)soc_times_ten * 9200) / 10000);
  }
#endif

  // Set the state of charge in the datalayer
  datalayer.battery.status.real_soc = soc_times_ten * 10;

  uint32_t remaining =
      (datalayer.battery.info.total_capacity_Wh * (datalayer.battery.status.real_soc - DISCHARGE_MIN_SOC)) /
      (10000 - DISCHARGE_MIN_SOC);
  if (remaining > 0) {
    datalayer.battery.status.remaining_capacity_Wh = remaining;
  } else {
    datalayer.battery.status.remaining_capacity_Wh = 0;
  }

  datalayer.battery.status.max_charge_power_W = taper_charge_power_linear(
      datalayer.battery.status.real_soc, MAX_CHARGE_POWER_W, CHARGE_TRICKLE_POWER_W, DERATE_CHARGE_ABOVE_SOC);

  datalayer.battery.status.max_discharge_power_W = taper_discharge_power_linear(
      datalayer.battery.status.real_soc, MAX_DISCHARGE_POWER_W, DISCHARGE_MIN_SOC, DERATE_DISCHARGE_BELOW_SOC);
}

void Mg5Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //all data are already update when it is received via CAN

  //reduce timout valeue for cell voltage timeout, reduce by 1 every second
  if (cellVoltageValidTime > 0) {
    cellVoltageValidTime--;
  }
}

void Mg5Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  uint16_t v, cell_id;

  switch (rx_frame.ID) {
    case 0x297: {  //BMS state
      // Contains battery status in rx_frame.data.u8[1]
      // Presumed mapping:
      // 1 = disconnected
      // 2 = precharge
      // 3 = connected
      // 15 = isolation fault
      // 0/8 = checking

      if (rx_frame.data.u8[1] != previousState) {
        logging.print("MG5: Battery status changed to: ");
        logging.println(getBMStatus(rx_frame.data.u8[1]));
      }

      if (rx_frame.data.u8[1] == 0xf && previousState != 0xf) {
        // Isolation fault, set event
        set_event(EVENT_BATTERY_ISOLATION, rx_frame.data.u8[0]);
      } else if (rx_frame.data.u8[1] != 0xf && previousState == 0xf) {
        // Isolation fault has cleared, clear event
        clear_event(EVENT_BATTERY_ISOLATION);
      }

      if (rx_frame.data.u8[1] == 0x03 && previousState != 0x03) {
        datalayer.system.status.battery_allows_contactor_closing = true;  //signal to the UI that contactors are closed
      } else {
        datalayer.system.status.battery_allows_contactor_closing = false;
      }

      previousState = rx_frame.data.u8[1];
      break;
    }
    case 0x172:
      break;
    case 0x173:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN

      // Contains cell min/max voltages
      v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      if (v > 0 && v < 0x2000) {
        datalayer.battery.status.cell_max_voltage_mV = v;
        cellVoltageValidTime = CELL_VOLTAGE_TIMEOUT;
      }
      v = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      if (v > 0 && v < 0x2000) {
        datalayer.battery.status.cell_min_voltage_mV = v;
      }
      break;
    case 0x293:
      break;
    case 0x295:
      break;
    case 0x29B:
      break;
    case 0x29C:
      break;
    case 0x2A0:
      break;
    case 0x2A2:

      if (rx_frame.data.u8[0] < 0xfe) {
        // Max cell temp
        datalayer.battery.status.temperature_max_dC = ((rx_frame.data.u8[0] << 8) / 50) - 400;
      }
      if (rx_frame.data.u8[5] < 0xfe) {
        // Min cell temp
        datalayer.battery.status.temperature_min_dC = ((rx_frame.data.u8[5] << 8) / 50) - 400;
      }
      break;
    case 0x322:
      break;
    case 0x334:
      break;
    case 0x33F:
      break;
    case 0x391:
      break;
    case 0x393:
      break;
    case 0x3AB:
      break;
    case 0x3AC:  // battery summary: SoC, HV voltage, HV current

      // Contains SoCs, voltage, current. Is emitted by both PTCAN and HybridCAN, but
      // There does not seem to be 2 SOC"s present like in MG HS battery, so we just read the meassages from the PTCAN bus

      if ((((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]) != 0) {
        // 3AC message contains a nonzero voltage (so must have come from PTCAN)

        // battery voltage
        v = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]);
        if (v > 0 && v < 4000) {
          datalayer.battery.status.voltage_dV = v * 2.5f;
        }
        // Current
        v = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        if (v > 0 && v < 0xf000) {
          datalayer.battery.status.current_dA = -(v - 20000) * 0.5f;
        }

        // SOC
        uint16_t soc = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        if (soc < 1022) {
          update_soc(soc);
        }
      }
      break;
    case 0x3B8:
      break;
    case 0x3BA:
      break;
    case 0x3BC:
      break;
    case 0x3BE:
      // Per-cell voltages and temps
      cell_id = rx_frame.data.u8[5];
      if (cell_id < 96) {
        v = 1000 + ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
        datalayer.battery.status.cell_voltages_mV[cell_id] = v < 10000 ? v : 0;
        // cell temperature is rx_frame.data.u8[1]-40 but BE doesn't use it
      }
      break;
    case 0x3C0:
      break;
    case 0x3C2:
      break;
    case 0x400:
      break;
    case 0x402:
      break;
    case 0x418:
      break;
    case 0x44C:
      break;
    case 0x620:
      break;
      // case 0x789: {  // response from UDS diagnostic service (ISO-TP)

      //   uint8_t pciByte = rx_frame.data.u8[0];  // 0x10/0x21/etc.
      //   uint8_t pciType = pciByte >> 4;         // 0=SF,1=FF,2=CF,3=FC
      //   uint8_t pciLower = pciByte & 0x0F;      // length nibble or sequence

      //   switch (pciType) {
      //     case 0x0: {  // Single Frame (SF)
      //       uint8_t sfLength = pciLower;
      //       (void)sfLength;  // if unused, silence warning

      //       uint8_t sid = rx_frame.data.u8[1];

      //       // Positive response to Diagnostic Session Control (0x10 -> 0x50)
      //       if (sid == 0x50) {
      //         uint8_t sub = rx_frame.data.u8[2];  // 0x01=Default, 0x03=Extended, etc.

      //         if (sub == 0x03 || sub == 0x01) {
      //           logging.print("entered ");
      //           logging.println(sub == 0x03 ? "extended diagnostic session" : "default session");

      //           // mark this UDS transaction done
      //           uds_tx_in_flight = false;
      //         }
      //         break;
      //       }

      //       // Positive response to clear DTC request
      //       if (sid == 0x54) {
      //         //  [0] PCI
      //         //  [1] SID (0x54)
      //         //  [2] DTC high byte  (0x02)
      //         //  [3] DTC mid byte   (0x93)
      //         //  [4] DTC low byte   (0x00)
      //         uint8_t b2 = rx_frame.data.u8[2];
      //         uint8_t b3 = rx_frame.data.u8[3];
      //         uint8_t b4 = rx_frame.data.u8[4];

      //         bool allDtcCleared = ((b2 == 0xFF && b3 == 0xFF && b4 == 0xFF) || (b2 == 0xAA && b3 == 0xAA && b4 == 0xAA));

      //         if (allDtcCleared) {
      //           logging.println("UDS: positive response, ALL DTCs cleared");
      //           userRequestClearDTC = false;  // reset the request
      //         } else {
      //           logging.print("UDS: positive ClearDTC response, group/DTC = ");
      //           logging.print(b2, HEX);
      //           logging.print(' ');
      //           logging.print(b3, HEX);
      //           logging.print(' ');
      //           logging.print(b4, HEX);
      //           logging.println();
      //         }

      //         uds_tx_in_flight = false;
      //         break;
      //       }

      //       // Negative response
      //       if (sid == 0x7F) {
      //         uint8_t origSid = rx_frame.data.u8[2];
      //         uint8_t nrc = rx_frame.data.u8[3];

      //         logging.print("UDS negative response to 0x");
      //         logging.print(origSid, HEX);
      //         logging.print(": NRC=0x");
      //         logging.print(nrc, HEX);
      //         logging.println();

      //         if (nrc != 0x78) {  // 0x78 = Response Pending; otherwise we’re done with this tx
      //           uds_tx_in_flight = false;
      //         }
      //         break;
      //       }

      //       if (sid == 0x62) {  // ReadDataByIdentifier response
      //         uint16_t did = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

      //         // signal that UDS transaction is complete
      //         uds_tx_in_flight = false;

      //         switch (did) {
      //           case 0xB041: {
      //             float v = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      //             (void)v;
      //             //logging.print("single frame UDS ReadDataByIdentifier bus voltage: ");
      //             //logging.println(v); //scaling to be checked

      //             break;
      //           }
      //           case 0xB042: {
      //             float v = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.25f;
      //             (void)v;
      //             //logging.print("single frame UDS ReadDataByIdentifier battery voltage: ");
      //             //logging.println(v);
      //             break;
      //           }
      //           case 0xB043: {
      //             float i = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5])) * 1;
      //             (void)i;
      //             //logging.print("single frame UDS ReadDataByIdentifier battery current: ");
      //             //logging.println(i); //scaling to be checked
      //             break;
      //           }
      //           case 0xB045: {
      //             float r = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 1;
      //             (void)r;
      //             //logging.print("single frame UDS ReadDataByIdentifier battery resistance: ");
      //             //logging.println(r); //scaling to be checked
      //             break;
      //           }
      //           case 0xB046: {
      //             float soc = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.1f;
      //             (void)soc;
      //             //logging.print("single frame UDS ReadDataByIdentifier battery state of charge: ");
      //             //logging.println(soc);
      //             break;
      //           }
      //           case 0xB047: {
      //             uint8_t e = rx_frame.data.u8[4];
      //             (void)e;
      //             //logging.print("single frame UDS ReadDataByIdentifier error: ");
      //             //logging.println(e);
      //             break;
      //           }
      //           case 0xB048: {
      //             uint8_t s = rx_frame.data.u8[4];
      //             (void)s;
      //             //logging.print("single frame UDS ReadDataByIdentifier status: ");
      //             //logging.println (getBMStatus(s));
      //             break;
      //           }
      //           case 0xB049: {
      //             uint8_t rB = rx_frame.data.u8[4];
      //             (void)rB;
      //             //logging.print("single frame UDS ReadDataByIdentifier relay B: ");
      //             //logging.println (rB); //normally 1, seems to go to 0 during change of contactor state
      //             break;
      //           }
      //           case 0xB04A: {
      //             uint8_t rG = rx_frame.data.u8[4];
      //             (void)rG;
      //             //logging.print("single frame UDS ReadDataByIdentifier relay G: ");
      //             //logging.println (rG); //normally 1, seems to go to 0 during change of contactor state
      //             break;
      //           }
      //           case 0xB052: {
      //             uint8_t rP = rx_frame.data.u8[4];
      //             (void)rP;
      //             //logging.print("single frame UDS ReadDataByIdentifier relay P: ");
      //             //logging.println (rP); //normally 1, seems to go to 0 during change of contactor state
      //             break;
      //           }
      //           case 0xB056: {
      //             float t = rx_frame.data.u8[4] - 100;
      //             (void)t;
      //             //logging.print("single frame UDS ReadDataByIdentifier BATTERY TEMP: ");
      //             //logging.println (t);
      //             break;
      //           }
      //           case 0xB058: {
      //             float mv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.001f;
      //             (void)mv;
      //             //logging.print("single frame UDS ReadDataByIdentifier max cell voltage: ");
      //             //logging.println (mv);
      //             break;
      //           }
      //           case 0xB059: {
      //             float nv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.001f;
      //             (void)nv;
      //             //logging.print("single frame UDS ReadDataByIdentifier min cell voltage: ");
      //             //logging.println (nv);
      //             break;
      //           }
      //           case 0xB05C: {
      //             float c = rx_frame.data.u8[4] - 100;
      //             (void)c;
      //             //logging.print("single frame UDS ReadDataByIdentifier coolant temperature: ");
      //             //logging.println (c);
      //             break;
      //           }
      //           case 0xB061: {
      //             float soh = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      //             (void)soh;
      //             //logging.print("single frame UDS ReadDataByIdentifier state of health: ");
      //             //logging.println (soh*0.01f);
      //             datalayer.battery.status.soh_pptt = soh;
      //             break;
      //           }
      //           case 0xB06D: {
      //             uint32_t t = (uint32_t(rx_frame.data.u8[4]) << 24) | (uint32_t(rx_frame.data.u8[5]) << 16) |
      //                          (uint32_t(rx_frame.data.u8[6]) << 8) | uint32_t(rx_frame.data.u8[7]);
      //             (void)t;
      //             //logging.print("single frame UDS ReadDataByIdentifier BMS time: ");
      //             //logging.println (t); //scaling to be checked

      //             break;
      //           } break;
      //         }
      //       }
      //       break;
      //     }

      //     case 0x1: {  // First Frame (FF)
      //       uint16_t totalLength = (uint16_t(pciLower) << 8) | rx_frame.data.u8[1];

      //       // DTC ReadDTCInformation response (0x59 0x02) at bytes [2..3]
      //       if (rx_frame.DLC >= 4 && rx_frame.data.u8[2] == 0x59 && rx_frame.data.u8[3] == 0x02) {
      //         startUDSMultiFrameReception(totalLength, 0x59);
      //         // FF payload for normal addressing starts at data[2]
      //         uint8_t avail = (rx_frame.DLC > 2) ? (rx_frame.DLC - 2) : 0;
      //         uint16_t remain = gUDSContext.UDS_expectedLength - gUDSContext.UDS_bytesReceived;
      //         uint8_t toStore = uint8_t(remain < avail ? remain : avail);
      //         if (toStore)
      //           storeUDSPayload(&rx_frame.data.u8[2], toStore);

      //         transmit_can_frame(&MG5_781_RQ_CONTINUE_MULTIFRAME);
      //         uds_timeout_ms = UDS_TIMEOUT_AFTER_FF_MS;  // extend while MF running
      //       }
      //       break;
      //     }

      //     case 0x2: {  // Consecutive Frame (CF)
      //       if (!gUDSContext.UDS_inProgress)
      //         break;
      //       uint8_t avail = (rx_frame.DLC > 1) ? (rx_frame.DLC - 1) : 0;  // CF payload at data[1]
      //       uint16_t remain = gUDSContext.UDS_expectedLength - gUDSContext.UDS_bytesReceived;
      //       uint8_t toStore = uint8_t(remain < avail ? remain : avail);
      //       if (toStore)
      //         storeUDSPayload(&rx_frame.data.u8[1], toStore);

      //       gUDSContext.receivedInBatch++;
      //       if (gUDSContext.receivedInBatch == 3) {
      //         transmit_can_frame(&MG5_781_RQ_CONTINUE_MULTIFRAME);
      //         gUDSContext.receivedInBatch = 0;
      //       }

      //       if (isUDSMessageComplete()) {
      //         if (gUDSContext.UDS_moduleID == 0x59) {
      //           const uint8_t* p = gUDSContext.UDS_buffer;
      //           uint16_t len = gUDSContext.UDS_bytesReceived;

      //           // 0: SID(0x59), 1: subfunc(0x02), 2: status availability mask
      //           if (len >= 3 && p[0] == 0x59 && p[1] == 0x02) {
      //             uint16_t off = 3;

      //             logging.print("UDS DTC list (");
      //             logging.print(len - off);
      //             logging.println(" bytes of data)");

      //             // entries are 3-byte DTC + 1-byte status
      //             while (off + 4 <= len) {
      //               uint32_t dtc = (uint32_t(p[off]) << 16) | (uint32_t(p[off + 1]) << 8) | p[off + 2];
      //               uint8_t status = p[off + 3];

      //               print_formatted_dtc(dtc, status);
      //               off += 4;
      //             }
      //           }
      //         }

      //         // signal that UDS transaction is complete
      //         uds_tx_in_flight = false;
      //         userRequestReadDTC = false;

      //         // ready for the next transaction
      //         gUDSContext.UDS_inProgress = false;
      //         gUDSContext.UDS_expectedLength = 0;
      //         gUDSContext.UDS_bytesReceived = 0;
      //       }
      //       break;
      //     }
      //     case 0x3: {  // Flow Control from ECU (rare)
      //       // optional: parse / ignore
      //       break;
      //     }
      //   }
      // }

    default:
      break;
  }
}

uint16_t Mg5Battery::handle_pid(uint16_t pid, uint32_t value) {
  // This receives on 0x789
  switch (pid) {
    case POLL_BUS_VOLTAGE:
      // currently ignored
      return POLL_BAT_SOH;
    case POLL_BAT_SOH:
      datalayer.battery.status.soh_pptt = value;
      break;
  }
  return 0;  // Continue normal PID cycling
}

void Mg5Battery::transmit_can(unsigned long currentMillis) {
  //Send 10ms message
  // if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
  //   previousMillis10 = currentMillis;
  // }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if (datalayer.battery.status.bms_status != FAULT                  // Fault, so open contactors!
        && userRequestContactorClose == true                          // User requested contactor closing
        && datalayer.system.status.inverter_allows_contactor_closing  // Inverter requests contactor closing
    ) {
      MG5_8A.data.u8[5] = 0x02;  // Command to close contactors
      //logging.println("contactor close command sent");
      if (contactorClosed == false) {
        // Just changed to closed
        contactorClosed = true;
        reset_DTC();
        //userRequestClearDTC = true;  //clear DTCs to clear DTC 293, otherwise contactors won't close
        //datalayer.battery.status.max_charge_power_W = MaxChargePower; //set the power limits, as they are set to zero when contactors are open
        //datalayer.battery.status.max_discharge_power_W = MaxDischargePower;
      }
    } else {
      contactorClosed = false;
      MG5_8A.data.u8[5] = 0x00;  // Command to open contactors
      //userRequestClearDTC = true; //clear DTCs to be able to close the contactors afterwards
      //logging.println("conctactor open command sent");
    }

    transmit_can_frame(&MG5_1F1);
    transmit_can_frame(&MG5_8A);
  }

  // if (uds_tx_in_flight == false) {     // No UDS transaction is in progress
  //   if (userRequestReadDTC == true) {  // DTC requested by user
  //     transmit_can_frame(&MG5_781_RQ_DTCs);
  //     uds_tx_in_flight = true;                    //singal that a UDS transaction is in progress
  //     uds_req_started_ms = currentMillis;         //timestamp when request was sent for timeout tracking
  //     uds_timeout_ms = UDS_TIMEOUT_BEFORE_FF_MS;  //increase timeout for multi-frame response
  //     logging.println("UDS DTC RQ sent");

  //   } else {
  //     if (userRequestClearDTC == true) {  // Clear DTC requested by user
  //       transmit_can_frame(&MG5_781_CLEAR_DTCs);
  //       uds_tx_in_flight = true;                    //singal that a UDS transaction is in progress
  //       uds_req_started_ms = currentMillis;         //timestamp when request was sent for timeout tracking
  //       uds_timeout_ms = UDS_TIMEOUT_BEFORE_FF_MS;  //increase timeout for multi-frame response
  //       logging.println("UDS Clear DTC RQ sent");
  //     } else {
  //       // Time to send next PID request, DTC has priority since it is slower
  //       if (currentMillis - previousMillisPID >= UDS_PID_REFRESH_MS) {
  //         previousMillisPID = currentMillis;
  //         // normal single-frame poll round-robin
  //         uds_slow_req_id_counter = increment_uds_req_id_counter(uds_slow_req_id_counter, numSlowUDSreqs);
  //         transmit_can_frame(UDS_REQUESTS_SLOW[uds_slow_req_id_counter]);
  //         uds_tx_in_flight = true;
  //         uds_req_started_ms = currentMillis;
  //         uds_timeout_ms = UDS_TIMEOUT_BEFORE_FF_MS;
  //       }
  //     }
  //   }
  // } else {  // UDS transaction in progress
  //   // Timeout / retry Session Control ------------------------------------
  //   if ((currentMillis - uds_req_started_ms) > uds_timeout_ms) {
  //     // re-enter Extended Session
  //     transmit_can_frame(&MG5_781_ses_ctrl);
  //     uds_tx_in_flight = true;
  //     uds_req_started_ms = currentMillis;
  //     uds_timeout_ms = UDS_TIMEOUT_BEFORE_FF_MS;
  //   }
  // }

  // TODO - session control stuff

  transmit_uds_can(currentMillis);
}

void Mg5Battery::setup(void) {  // Performs one time setup at startup
  setup_uds(0x781, POLL_BUS_VOLTAGE);
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.total_capacity_Wh = TOTAL_BATTERY_CAPACITY_WH;
  datalayer.battery.info.number_of_cells = 96;
  // uds_tx_in_flight = true;                  // Make sure UDS doesn't start right away
  // uds_req_started_ms = millis();            // prevent immediate timeout
  // uds_timeout_ms = UDS_TIMEOUT_AFTER_BOOT;  // initial delay to restart UDS after boot-up
}

//#endif
