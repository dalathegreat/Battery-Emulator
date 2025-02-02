#include "comm_can.h"
#include "../../include.h"
#include "src/devboard/sdcard/sdcard.h"

// Parameters

CAN_device_t CAN_cfg;          // CAN Config
const int rx_queue_size = 10;  // Receive Queue size
volatile bool send_ok_2515 = 0;
volatile bool send_ok_2518 = 0;

#ifdef CAN_ADDON
static const uint32_t QUARTZ_FREQUENCY = CRYSTAL_FREQUENCY_MHZ * 1000000UL;  //MHZ configured in USER_SETTINGS.h
SPIClass SPI2515;
ACAN2515 can(MCP2515_CS, SPI2515, MCP2515_INT);
static ACAN2515_Buffer16 gBuffer;
#endif  //CAN_ADDON
#ifdef CANFD_ADDON
SPIClass SPI2517;
ACAN2517FD canfd(MCP2517_CS, SPI2517, MCP2517_INT);
#endif  //CANFD_ADDON

// Initialization functions

void init_CAN() {
// CAN pins
#ifdef CAN_SE_PIN
  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);
#endif  // CAN_SE_PIN
  CAN_cfg.speed = CAN_SPEED_500KBPS;
#ifdef NATIVECAN_250KBPS  // Some component is requesting lower CAN speed
  CAN_cfg.speed = CAN_SPEED_250KBPS;
#endif  // NATIVECAN_250KBPS
  CAN_cfg.tx_pin_id = CAN_TX_PIN;
  CAN_cfg.rx_pin_id = CAN_RX_PIN;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();

#ifdef CAN_ADDON
#ifdef DEBUG_LOG
  logging.println("Dual CAN Bus (ESP32+MCP2515) selected");
#endif  // DEBUG_LOG
  gBuffer.initWithSize(25);
  SPI2515.begin(MCP2515_SCK, MCP2515_MISO, MCP2515_MOSI);
  ACAN2515Settings settings2515(QUARTZ_FREQUENCY, 500UL * 1000UL);  // CAN bit rate 500 kb/s
  settings2515.mRequestedMode = ACAN2515Settings::NormalMode;
  const uint16_t errorCode2515 = can.begin(settings2515, [] { can.isr(); });
  if (errorCode2515 == 0) {
#ifdef DEBUG_LOG
    logging.println("Can ok");
#endif  // DEBUG_LOG
  } else {
#ifdef DEBUG_LOG
    logging.print("Error Can: 0x");
    logging.println(errorCode2515, HEX);
#endif  // DEBUG_LOG
    set_event(EVENT_CANMCP2515_INIT_FAILURE, (uint8_t)errorCode2515);
  }
#endif  // CAN_ADDON

#ifdef CANFD_ADDON
#ifdef DEBUG_LOG
  logging.println("CAN FD add-on (ESP32+MCP2517) selected");
#endif  // DEBUG_LOG
  SPI2517.begin(MCP2517_SCK, MCP2517_SDO, MCP2517_SDI);
  ACAN2517FDSettings settings2517(CANFD_ADDON_CRYSTAL_FREQUENCY_MHZ, 500 * 1000,
                                  DataBitRateFactor::x4);  // Arbitration bit rate: 500 kbit/s, data bit rate: 2 Mbit/s
#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  settings2517.mRequestedMode = ACAN2517FDSettings::Normal20B;  // ListenOnly / Normal20B / NormalFD
#else                                                           // not USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  settings2517.mRequestedMode = ACAN2517FDSettings::NormalFD;  // ListenOnly / Normal20B / NormalFD
#endif                                                          // USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  const uint32_t errorCode2517 = canfd.begin(settings2517, [] { canfd.isr(); });
  canfd.poll();
  if (errorCode2517 == 0) {
#ifdef DEBUG_LOG
    logging.print("Bit Rate prescaler: ");
    logging.println(settings2517.mBitRatePrescaler);
    logging.print("Arbitration Phase segment 1: ");
    logging.print(settings2517.mArbitrationPhaseSegment1);
    logging.print(" segment 2: ");
    logging.print(settings2517.mArbitrationPhaseSegment2);
    logging.print(" SJW: ");
    logging.println(settings2517.mArbitrationSJW);
    logging.print("Actual Arbitration Bit Rate: ");
    logging.print(settings2517.actualArbitrationBitRate());
    logging.print(" bit/s");
    logging.print(" (Exact:");
    logging.println(settings2517.exactArbitrationBitRate() ? "yes)" : "no)");
    logging.print("Arbitration Sample point: ");
    logging.print(settings2517.arbitrationSamplePointFromBitStart());
    logging.println("%");
#endif  // DEBUG_LOG
  } else {
#ifdef DEBUG_LOG
    logging.print("CAN-FD Configuration error 0x");
    logging.println(errorCode2517, HEX);
#endif  // DEBUG_LOG
    set_event(EVENT_CANMCP2517FD_INIT_FAILURE, (uint8_t)errorCode2517);
  }
#endif  // CANFD_ADDON
}

// Transmit functions
void transmit_can() {
  if (!allowed_to_send_CAN) {
    return;  //Global block of CAN messages
  }

  transmit_can_battery();

#ifdef CAN_INVERTER_SELECTED
  transmit_can_inverter();
#endif  // CAN_INVERTER_SELECTED

#ifdef CHARGER_SELECTED
  transmit_can_charger();
#endif  // CHARGER_SELECTED

#ifdef CAN_SHUNT_SELECTED
  transmit_can_shunt();
#endif  // CAN_SHUNT_SELECTED
}

void transmit_can_frame(CAN_frame* tx_frame, int interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, frameDirection(MSG_TX));

#ifdef LOG_CAN_TO_SD
  add_can_frame_to_buffer(*tx_frame, frameDirection(MSG_TX));
#endif

  switch (interface) {
    case CAN_NATIVE:
      CAN_frame_t frame;
      frame.MsgID = tx_frame->ID;
      frame.FIR.B.FF = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      frame.FIR.B.DLC = tx_frame->DLC;
      frame.FIR.B.RTR = CAN_no_RTR;
      for (uint8_t i = 0; i < tx_frame->DLC; i++) {
        frame.data.u8[i] = tx_frame->data.u8[i];
      }
      ESP32Can.CANWriteFrame(&frame);
      break;
    case CAN_ADDON_MCP2515: {
#ifdef CAN_ADDON
      //Struct with ACAN2515 library format, needed to use the MCP2515 library for CAN2
      CANMessage MCP2515Frame;
      MCP2515Frame.id = tx_frame->ID;
      MCP2515Frame.ext = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      MCP2515Frame.len = tx_frame->DLC;
      MCP2515Frame.rtr = false;
      for (uint8_t i = 0; i < MCP2515Frame.len; i++) {
        MCP2515Frame.data[i] = tx_frame->data.u8[i];
      }

      send_ok_2515 = can.tryToSend(MCP2515Frame);
      if (!send_ok_2515) {
        set_event(EVENT_CAN_BUFFER_FULL, interface);
      } else {
        clear_event(EVENT_CAN_BUFFER_FULL);
      }
#else   // Interface not compiled, and settings try to use it
      set_event(EVENT_INTERFACE_MISSING, interface);
#endif  //CAN_ADDON
    } break;
    case CANFD_NATIVE:
    case CANFD_ADDON_MCP2518: {
#ifdef CANFD_ADDON
      CANFDMessage MCP2518Frame;
      if (tx_frame->FD) {
        MCP2518Frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
      } else {  //Classic CAN message
        MCP2518Frame.type = CANFDMessage::CAN_DATA;
      }
      MCP2518Frame.id = tx_frame->ID;
      MCP2518Frame.ext = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      MCP2518Frame.len = tx_frame->DLC;
      for (uint8_t i = 0; i < MCP2518Frame.len; i++) {
        MCP2518Frame.data[i] = tx_frame->data.u8[i];
      }
      send_ok_2518 = canfd.tryToSend(MCP2518Frame);
      if (!send_ok_2518) {
        set_event(EVENT_CANFD_BUFFER_FULL, interface);
      } else {
        clear_event(EVENT_CANFD_BUFFER_FULL);
      }
#else   // Interface not compiled, and settings try to use it
      set_event(EVENT_INTERFACE_MISSING, interface);
#endif  //CANFD_ADDON
    } break;
    default:
      // Invalid interface sent with function call. TODO: Raise event that coders messed up
      break;
  }
}

// Receive functions
void receive_can() {
  receive_frame_can_native();  // Receive CAN messages from native CAN port
#ifdef CAN_ADDON
  receive_frame_can_addon();  // Receive CAN messages on add-on MCP2515 chip
#endif                        // CAN_ADDON
#ifdef CANFD_ADDON
  receive_frame_canfd_addon();  // Receive CAN-FD messages.
#endif                          // CANFD_ADDON
}

void receive_frame_can_native() {  // This section checks if we have a complete CAN message incoming on native CAN port
  CAN_frame_t rx_frame_native;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame_native, 0) == pdTRUE) {
    CAN_frame rx_frame;
    rx_frame.ID = rx_frame_native.MsgID;
    if (rx_frame_native.FIR.B.FF == CAN_frame_std) {
      rx_frame.ext_ID = false;
    } else {  //CAN_frame_ext == 1
      rx_frame.ext_ID = true;
    }
    rx_frame.DLC = rx_frame_native.FIR.B.DLC;
    for (uint8_t i = 0; i < rx_frame.DLC && i < 8; i++) {
      rx_frame.data.u8[i] = rx_frame_native.data.u8[i];
    }
    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CAN_NATIVE);
  }
}

#ifdef CAN_ADDON
void receive_frame_can_addon() {  // This section checks if we have a complete CAN message incoming on add-on CAN port
  CAN_frame rx_frame;             // Struct with our CAN format
  CANMessage MCP2515frame;        // Struct with ACAN2515 library format, needed to use the MCP2515 library

  if (can.available()) {
    can.receive(MCP2515frame);

    rx_frame.ID = MCP2515frame.id;
    rx_frame.ext_ID = MCP2515frame.ext ? CAN_frame_ext : CAN_frame_std;
    rx_frame.DLC = MCP2515frame.len;
    for (uint8_t i = 0; i < MCP2515frame.len && i < 8; i++) {
      rx_frame.data.u8[i] = MCP2515frame.data[i];
    }

    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CAN_ADDON_MCP2515);
  }
}
#endif  // CAN_ADDON

#ifdef CANFD_ADDON
void receive_frame_canfd_addon() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage MCP2518frame;
  int count = 0;
  while (canfd.available() && count++ < 16) {
    canfd.receive(MCP2518frame);

    CAN_frame rx_frame;
    rx_frame.ID = MCP2518frame.id;
    rx_frame.ext_ID = MCP2518frame.ext;
    rx_frame.DLC = MCP2518frame.len;
    memcpy(rx_frame.data.u8, MCP2518frame.data, MIN(rx_frame.DLC, 64));
    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CANFD_ADDON_MCP2518);
    map_can_frame_to_variable(&rx_frame, CANFD_NATIVE);
  }
}
#endif  // CANFD_ADDON

// Support functions
void print_can_frame(CAN_frame frame, frameDirection msgDir) {
#ifdef DEBUG_CAN_DATA  // If enabled in user settings, print out the CAN messages via USB
  uint8_t i = 0;
  Serial.print("(");
  Serial.print(millis() / 1000.0);
  (msgDir == MSG_RX) ? Serial.print(") RX0 ") : Serial.print(") TX1 ");
  Serial.print(frame.ID, HEX);
  Serial.print(" [");
  Serial.print(frame.DLC);
  Serial.print("] ");
  for (i = 0; i < frame.DLC; i++) {
    Serial.print(frame.data.u8[i] < 16 ? "0" : "");
    Serial.print(frame.data.u8[i], HEX);
    if (i < frame.DLC - 1)
      Serial.print(" ");
  }
  Serial.println("");
#endif  // DEBUG_CAN_DATA

  if (datalayer.system.info.can_logging_active) {  // If user clicked on CAN Logging page in webserver, start recording
    dump_can_frame(frame, msgDir);
  }
}

void map_can_frame_to_variable(CAN_frame* rx_frame, int interface) {
  print_can_frame(*rx_frame, frameDirection(MSG_RX));

#ifdef LOG_CAN_TO_SD
  add_can_frame_to_buffer(*rx_frame, frameDirection(MSG_RX));
#endif

  if (interface == can_config.battery) {
    handle_incoming_can_frame_battery(*rx_frame);
#ifdef CHADEMO_BATTERY
    ISA_handleFrame(rx_frame);
#endif
  }
  if (interface == can_config.inverter) {
#ifdef CAN_INVERTER_SELECTED
    map_can_frame_to_variable_inverter(*rx_frame);
#endif
  }
  if (interface == can_config.battery_double) {
#ifdef DOUBLE_BATTERY
    handle_incoming_can_frame_battery2(*rx_frame);
#endif
  }
  if (interface == can_config.charger) {
#ifdef CHARGER_SELECTED
    map_can_frame_to_variable_charger(*rx_frame);
#endif
  }
  if (interface == can_config.shunt) {
#ifdef CAN_SHUNT_SELECTED
    handle_incoming_can_frame_shunt(*rx_frame);
#endif
  }
}
void dump_can_frame(CAN_frame& frame, frameDirection msgDir) {
  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

  if (offset + 128 > sizeof(datalayer.system.info.logged_can_messages)) {
    // Not enough space, reset and start from the beginning
    offset = 0;
  }
  unsigned long currentTime = millis();
  // Add timestamp
  offset += snprintf(message_string + offset, message_string_size - offset, "(%lu.%03lu) ", currentTime / 1000,
                     currentTime % 1000);

  // Add direction. The 0 and 1 after RX and TX ensures that SavvyCAN puts TX and RX in a different bus.
  offset += snprintf(message_string + offset, message_string_size - offset, "%s ", (msgDir == MSG_RX) ? "RX0" : "TX1");

  // Add ID and DLC
  offset += snprintf(message_string + offset, message_string_size - offset, "%X [%u] ", frame.ID, frame.DLC);

  // Add data bytes
  for (uint8_t i = 0; i < frame.DLC; i++) {
    if (i < frame.DLC - 1) {
      offset += snprintf(message_string + offset, message_string_size - offset, "%02X ", frame.data.u8[i]);
    } else {
      offset += snprintf(message_string + offset, message_string_size - offset, "%02X", frame.data.u8[i]);
    }
  }
  // Add linebreak
  offset += snprintf(message_string + offset, message_string_size - offset, "\n");

  datalayer.system.info.logged_can_messages_offset = offset;  // Update offset in buffer
}
