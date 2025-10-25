#include "MG-5-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

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
      return "STANDBY";
    case 3:
      return "RUNNING/DRIVING";
    case 6:
      return "AC CHARGING";
    case 7:
      return "DC CHARGING";
    default:
      return "UNKNOWN";
  }
}

void Mg5Battery::print_formatted_dtc(uint32_t dtc24, uint8_t status)
{
  // DTC bytes: A B C (24 bits). SAE letter from top 2 bits of A.
  uint8_t A = (dtc24 >> 16) & 0xFF;
  uint8_t B = (dtc24 >>  8) & 0xFF;
  // uint8_t C =  dtc24        & 0xFF; // often a failure-type byte; keep if you need it

  const char sysMap[4] = { 'P', 'C', 'B', 'U' };
  char sys = sysMap[(A & 0xC0) >> 6];

  // Four digits: D1 D2 D3 D4 from the remaining nibbles of A and B
  uint8_t d1 = (A & 0x30) >> 4;
  uint8_t d2 = (A & 0x0F);
  uint8_t d3 = (B & 0xF0) >> 4;
  uint8_t d4 = (B & 0x0F);

  #ifdef DEBUG_LOG
    logging.print("DTC ");
    logging.print(sys);
    logging.print(d1);
    logging.print(d2, HEX);
    logging.print(d3, HEX);
    logging.print(d4, HEX);

    // print hex byte first
    logging.print("  status=0x");
    logging.print(status, HEX);
    logging.print(" [");

    bool first = true;
    auto add = [&](const char* s){
      if (!first) logging.print(", ");
      logging.print(s);
      first = false;
    };

    if (status & 0x08) add("Confirmed");
    if (status & 0x04) add("Pending");
    if (status & 0x20) add("FailSinceClear");
    if (status & 0x01) add("Fail");
    if (status & 0x10) add("NotCompSinceClear");
    if (status & 0x40) add("NotCompThisCycle");
    if (status & 0x80) add("MIL");
    if (status & 0x02) add("FailThisCycle");

    if (first) logging.print("NoFlags");
    logging.println("]");
  #endif
}

void Mg5Battery::startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID) {
  gUDSContext.UDS_inProgress = true;
  gUDSContext.UDS_expectedLength = totalLength;
  gUDSContext.UDS_bytesReceived = 0;
  gUDSContext.UDS_moduleID = moduleID;
  gUDSContext.receivedInBatch = 0;  
  memset(gUDSContext.UDS_buffer, 0, sizeof(gUDSContext.UDS_buffer));
  gUDSContext.UDS_lastFrameMillis = millis();  // if you want to track timeouts
}

bool Mg5Battery::storeUDSPayload(const uint8_t* payload, uint8_t length) {
  if (gUDSContext.UDS_bytesReceived + length > sizeof(gUDSContext.UDS_buffer)) {
    // Overflow => abort
    gUDSContext.UDS_inProgress = false;
#ifdef DEBUG_LOG
    logging.println("UDS Payload Overflow");
#endif  // DEBUG_LOG
    return false;
  }
  memcpy(&gUDSContext.UDS_buffer[gUDSContext.UDS_bytesReceived], payload, length);
  gUDSContext.UDS_bytesReceived += length;
  gUDSContext.UDS_lastFrameMillis = millis();


  #ifdef DEBUG_LOG
    //logging.println("received UDS payload chunk, total bytes received: " + String(gUDSContext.UDS_bytesReceived));
    //logging.println("Total bytes expected: " + String(gUDSContext.UDS_expectedLength));
  #endif  // DEBUG_LOG
  // If we’ve reached or exceeded the expected length, mark complete
  if (gUDSContext.UDS_bytesReceived >= gUDSContext.UDS_expectedLength) {
    gUDSContext.UDS_inProgress = false;
     #ifdef DEBUG_LOG
         logging.println("Recived all expected UDS bytes");
     #endif  // DEBUG_LOG
  }
  return true;
}

bool Mg5Battery::isUDSMessageComplete() {
  return (!gUDSContext.UDS_inProgress && gUDSContext.UDS_bytesReceived > 0);
}

void Mg5Battery::update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = battery_soc;
  datalayer.battery.status.voltage_dV = battery_voltage;
  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;
}




void Mg5Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  //datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x297:{ //BMS state
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN
      battery_BMS_state = rx_frame.data.u8[1]; // BMS state B1
      break;
    }
    case 0x172:
      break;
    case 0x173:
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
    case 0x3AC:{// battery summary: SoC, HV voltage, HV current
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN
  
      // SoC: B3-B4 (0.1 %)
      uint16_t soc_raw = (uint16_t(rx_frame.data.u8[2]) << 8) | rx_frame.data.u8[3];
      battery_soc = soc_raw * 0.1f;                  

      // HV Voltage: HIGH nibble of B5 + full B6 (12-bit), 0.25 V/bit
      uint16_t v_raw = ((rx_frame.data.u8[4] & 0xF0) >> 4) << 8 | rx_frame.data.u8[5];
      battery_voltage = v_raw * 0.25f;                

      // HV Current: B7-B8, 0.05 A/bit, offset -1000 A
      uint16_t i_raw = (uint16_t(rx_frame.data.u8[6]) << 8) | rx_frame.data.u8[7];
      battery_current = (i_raw * 0.05f) - 1000.0f;

      break;
    }
    case 0x3B8:
      break;
    case 0x3BA:
      break;
    case 0x3BC:
      break;
    case 0x3BE:
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
    case 0x789: { // response from UDS diagnostic service (ISO-TP)

      #ifdef DEBUG_LOG
        //logging.println("received CAN frame 0x789");
      #endif
      uint8_t pciByte  = rx_frame.data.u8[0];  // 0x10/0x21/etc.
      uint8_t pciType  = pciByte >> 4;         // 0=SF,1=FF,2=CF,3=FC
      uint8_t pciLower = pciByte & 0x0F;       // length nibble or sequence

      switch (pciType) {
        case 0x0: { // Single Frame (SF)
          uint8_t sfLength = pciLower;
          (void)sfLength; // if unused, silence warning

          uint8_t sid = rx_frame.data.u8[1];

          // Positive response to Diagnostic Session Control (0x10 -> 0x50)
          if (sid == 0x50) {
            uint8_t sub = rx_frame.data.u8[2]; // 0x01=Default, 0x03=Extended, etc.

            if (sub == 0x03 || sub == 0x01) {
              #ifdef DEBUG_LOG
                logging.print("entered ");
                logging.println(sub == 0x03 ? "extended diagnostic session" : "default session");
                logging.print("uds_timeout_ms: ");
                logging.println(uds_timeout_ms);
              #endif

              // mark this UDS transaction done
              uds_tx_in_flight = false;
            }
            break;
          }

          // Negative response
          if (sid == 0x7F) {
            uint8_t origSid = rx_frame.data.u8[2];
            uint8_t nrc     = rx_frame.data.u8[3];

            #ifdef DEBUG_LOG
              logging.print("UDS negative response to 0x");
              logging.print(origSid, HEX);
              logging.print(": NRC=0x");
              logging.println(nrc, HEX);
            #endif

            if (nrc != 0x78) { // 0x78 = Response Pending; otherwise we’re done with this tx
              //uds_tx_in_flight = false;
            }
            break;
          }


          if (sid == 0x62) { // ReadDataByIdentifier response
            uint16_t did = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

              // signal that UDS transaction is complete
            uds_tx_in_flight = false;

            switch (did) {
              case 0xB041: { float v = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.25f; (void)v;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier bus voltage");
                  //logging.println (v);
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB042: { float v = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.25f; (void)v;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier battery voltage");
                  //logging.println (v);
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB043: { float i = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) - 1000.0f) * 0.025f; (void)i;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier battery current");
                  //logging.println (i);
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB045: { float r = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.5f; (void)r;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier battery resistance");
                  //logging.println (r);
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB046: { float soc = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.1f; (void)soc; 
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier battery state of charge");
                  //logging.println (soc);
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB047: { uint8_t e = rx_frame.data.u8[4]; (void)e;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier error");
                  //logging.println (e);  
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB048: { uint8_t s = rx_frame.data.u8[4]; (void)s; 
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier status");
                  //logging.println (getBMStatus(s));
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB049: { uint8_t rB= rx_frame.data.u8[4]; (void)rB;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier relay B");
                  //logging.println (rB);
                #endif  // DEBUG_LOG
                break;
              }
              case 0xB04A: { uint8_t rG= rx_frame.data.u8[4]; (void)rG;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier relay G");
                  //logging.println (rG);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB052: { uint8_t rP= rx_frame.data.u8[4]; (void)rP;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier relay P");
                  //logging.println (rP);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB056: { float t = rx_frame.data.u8[4]*0.5f - 2.0f; (void)t;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier BATTERY TEMP");
                  //logging.println (t);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB058: { float mv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.001f; (void)mv;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier max cell voltage");
                  //logging.println (mv);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB059: { float nv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.001f; (void)nv;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier min cell voltage");
                  //logging.println (nv);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB05C: { float c = rx_frame.data.u8[4]*0.5f - 2.0f; (void)c;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier coolant temperature");
                  //logging.println (c);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB061: { float soh = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.01f; (void)soh;
                #ifdef DEBUG_LOG
                  //logging.println("single frame UDS ReadDataByIdentifier state of health");
                  //logging.println (soh);
                #endif  // DEBUG_LOG
              break;
             }
              case 0xB06D: {
                uint32_t t = (uint32_t(rx_frame.data.u8[4]) << 24) |
                            (uint32_t(rx_frame.data.u8[5]) << 16) |
                            (uint32_t(rx_frame.data.u8[6]) <<  8) |
                              uint32_t(rx_frame.data.u8[7]);
                (void)t;
                #ifdef DEBUG_LOG
                  logging.println("single frame UDS ReadDataByIdentifier BMS time");
                  logging.println (t);
                #endif  // DEBUG_LOG
                break;
              }
            }


          }
          break;
        }

        case 0x1: { // First Frame (FF)
          uint16_t totalLength = (uint16_t(pciLower) << 8) | rx_frame.data.u8[1];

          // DTC ReadDTCInformation response (0x59 0x02) at bytes [2..3]
          if (rx_frame.DLC >= 4 && rx_frame.data.u8[2] == 0x59 && rx_frame.data.u8[3] == 0x02) {
            startUDSMultiFrameReception(totalLength, 0x59);
            // FF payload for normal addressing starts at data[2]
            uint8_t avail   = (rx_frame.DLC > 2) ? (rx_frame.DLC - 2) : 0;
            uint16_t remain = gUDSContext.UDS_expectedLength - gUDSContext.UDS_bytesReceived;
            uint8_t toStore = uint8_t(remain < avail ? remain : avail);
            if (toStore) storeUDSPayload(&rx_frame.data.u8[2], toStore);

            transmit_can_frame(&MG5_781_RQ_CONTINUE_MULTIFRAME, can_config.battery);
            uds_timeout_ms = UDS_TIMEOUT_AFTER_FF_MS; // extend while MF running

          }
          break;
        }

        case 0x2: { // Consecutive Frame (CF)
          if (!gUDSContext.UDS_inProgress) break;
          uint8_t avail   = (rx_frame.DLC > 1) ? (rx_frame.DLC - 1) : 0; // CF payload at data[1]
          uint16_t remain = gUDSContext.UDS_expectedLength - gUDSContext.UDS_bytesReceived;
          uint8_t toStore = uint8_t(remain < avail ? remain : avail);
          if (toStore) storeUDSPayload(&rx_frame.data.u8[1], toStore);

          gUDSContext.receivedInBatch++;
          if (gUDSContext.receivedInBatch  == 3) {
            transmit_can_frame(&MG5_781_RQ_CONTINUE_MULTIFRAME, can_config.battery);
            gUDSContext.receivedInBatch = 0;
          }
          break;
        }

        case 0x3: { // Flow Control from ECU (rare)
          // optional: parse / ignore
          break;
        }

      } // end inner switch

      if (isUDSMessageComplete()) {
        if (gUDSContext.UDS_moduleID == 0x59) {
          const uint8_t* p = gUDSContext.UDS_buffer;
          uint16_t len = gUDSContext.UDS_bytesReceived;

          // 0: SID(0x59), 1: subfunc(0x02), 2: status availability mask
          if (len >= 3 && p[0] == 0x59 && p[1] == 0x02) {
            uint16_t off = 3;

            #ifdef DEBUG_LOG
              logging.print("UDS DTC list (");
              logging.print(len - off);
              logging.println(" bytes of data)");
            #endif

            // entries are 3-byte DTC + 1-byte status
            while (off + 4 <= len) {
              uint32_t dtc = (uint32_t(p[off]) << 16) | (uint32_t(p[off+1]) << 8) | p[off+2];
              uint8_t status = p[off+3];

              print_formatted_dtc(dtc, status);
              off += 4;
            }
          }

        }

        // signal that UDS transaction is complete
        uds_tx_in_flight = false;
      

        // ready for the next transaction
        gUDSContext.UDS_inProgress     = false;
        gUDSContext.UDS_expectedLength = 0;
        gUDSContext.UDS_bytesReceived  = 0;
        

      break; // <<— keep this break for the outer switch(rx_frame.ID)
    }
  }
    default:
      break;
  
  }
}

void Mg5Battery::transmit_can(unsigned long currentMillis) {
  //Send 10ms message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //transmit_can_frame(&MG_5_100, can_config.battery);
  }

  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;


    //uds_fast_req_id_counter = increment_uds_req_id_counter(
    //uds_fast_req_id_counter, numFastUDSreqs);  //Loop through and send a different UDS request each cycle
    //transmit_can_frame(UDS_REQUESTS_FAST[uds_fast_req_id_counter], can_config.battery);

    //transmit_can_frame(&MG_5_100, can_config.battery);
  }

  if (currentMillis - previousMillis1000 >= INTERVAL_1000_MS) {
    previousMillis1000 = currentMillis;
  }

  if (currentMillis - previousMillis2000 >= INTERVAL_2000_MS) {
    previousMillis2000 = currentMillis;

  }

  if(uds_tx_in_flight  == false){ // No UDS transaction is in progress
    if(currentMillis - previousMillisDTC >= UDS_DTC_REFRESH_MS){ // Time to request DTCs
      previousMillisDTC = currentMillis;
      transmit_can_frame(&MG5_781_RQ_DTCs, can_config.battery);
      uds_tx_in_flight   = true;
      uds_req_started_ms = currentMillis;
      uds_timeout_ms     = UDS_TIMEOUT_BEFORE_FF_MS;
      #ifdef DEBUG_LOG
        //logging.println("UDS DTC RQ sent");
      #endif  // DEBUG_LOG
    
    }
    else{  // Time to send next PID request, DTC has priority since it is slower
      if(currentMillis - previousMillisPID >= UDS_PID_REFRESH_MS){
        previousMillisPID = currentMillis;
        // normal single-frame poll round-robin
        uds_slow_req_id_counter = increment_uds_req_id_counter(
        uds_slow_req_id_counter, numSlowUDSreqs);
        transmit_can_frame(UDS_REQUESTS_SLOW[uds_slow_req_id_counter], can_config.battery);
        uds_tx_in_flight   = true;
        uds_req_started_ms = currentMillis;
        uds_timeout_ms     = UDS_TIMEOUT_BEFORE_FF_MS;
        }
    }
  }
  else{ // UDS transaction in progress
    // Timeout / retry Session Control ------------------------------------
    if ((currentMillis - uds_req_started_ms) > uds_timeout_ms) {
      // re-enter Extended Session
      transmit_can_frame(&MG5_781_ses_ctrl, can_config.battery);
      uds_tx_in_flight   = true;
      uds_req_started_ms = currentMillis;
      uds_timeout_ms     = UDS_TIMEOUT_BEFORE_FF_MS;
    }  
  }
}

void Mg5Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "MG 5 battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;

  uds_tx_in_flight  == true; // Make sure UDS doesn't start right away
  uds_req_started_ms = millis(); // prevent immediate timeout
  uds_timeout_ms = UDS_TIMEOUT_AFTER_BOOT; // initial delay to restart UDS after boot-up
  
}
