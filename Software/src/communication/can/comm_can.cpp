#include "comm_can.h"
#include "../../include.h"

// Parameters

CAN_device_t CAN_cfg;          // CAN Config
const int rx_queue_size = 10;  // Receive Queue size
volatile bool send_ok = 0;

#ifdef CAN_ADDON
static const uint32_t QUARTZ_FREQUENCY = CRYSTAL_FREQUENCY_MHZ * 1000000UL;  //MHZ configured in USER_SETTINGS.h
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);
static ACAN2515_Buffer16 gBuffer;
#endif  //CAN_ADDON
#ifdef CANFD_ADDON
ACAN2517FD canfd(MCP2517_CS, SPI, MCP2517_INT);
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
#ifdef DEBUG_VIA_USB
  Serial.println("Dual CAN Bus (ESP32+MCP2515) selected");
#endif  // DEBUG_VIA_USB
  gBuffer.initWithSize(25);
  SPI.begin(MCP2515_SCK, MCP2515_MISO, MCP2515_MOSI);
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 500UL * 1000UL);  // CAN bit rate 500 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  const uint16_t errorCodeMCP = can.begin(settings, [] { can.isr(); });
  if (errorCodeMCP == 0) {
#ifdef DEBUG_VIA_USB
    Serial.println("Can ok");
#endif  // DEBUG_VIA_USB
  } else {
#ifdef DEBUG_VIA_USB
    Serial.print("Error Can: 0x");
    Serial.println(errorCodeMCP, HEX);
#endif  // DEBUG_VIA_USB
    set_event(EVENT_CANMCP_INIT_FAILURE, (uint8_t)errorCodeMCP);
  }
#endif  // CAN_ADDON

#ifdef CANFD_ADDON
#ifdef DEBUG_VIA_USB
  Serial.println("CAN FD add-on (ESP32+MCP2517) selected");
#endif  // DEBUG_VIA_USB
  SPI.begin(MCP2517_SCK, MCP2517_SDO, MCP2517_SDI);
  ACAN2517FDSettings settings(CANFD_ADDON_CRYSTAL_FREQUENCY_MHZ, 500 * 1000,
                              DataBitRateFactor::x4);  // Arbitration bit rate: 500 kbit/s, data bit rate: 2 Mbit/s
#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  settings.mRequestedMode = ACAN2517FDSettings::Normal20B;  // ListenOnly / Normal20B / NormalFD
#else                                                       // not USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  settings.mRequestedMode = ACAN2517FDSettings::NormalFD;  // ListenOnly / Normal20B / NormalFD
#endif                                                      // USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  const uint32_t errorCode = canfd.begin(settings, [] { canfd.isr(); });
  canfd.poll();
  if (errorCode == 0) {
#ifdef DEBUG_VIA_USB
    Serial.print("Bit Rate prescaler: ");
    Serial.println(settings.mBitRatePrescaler);
    Serial.print("Arbitration Phase segment 1: ");
    Serial.println(settings.mArbitrationPhaseSegment1);
    Serial.print("Arbitration Phase segment 2: ");
    Serial.println(settings.mArbitrationPhaseSegment2);
    Serial.print("Arbitration SJW:");
    Serial.println(settings.mArbitrationSJW);
    Serial.print("Actual Arbitration Bit Rate: ");
    Serial.print(settings.actualArbitrationBitRate());
    Serial.println(" bit/s");
    Serial.print("Exact Arbitration Bit Rate ? ");
    Serial.println(settings.exactArbitrationBitRate() ? "yes" : "no");
    Serial.print("Arbitration Sample point: ");
    Serial.print(settings.arbitrationSamplePointFromBitStart());
    Serial.println("%");
#endif  // DEBUG_VIA_USB
  } else {
#ifdef DEBUG_VIA_USB
    Serial.print("CAN-FD Configuration error 0x");
    Serial.println(errorCode, HEX);
#endif  // DEBUG_VIA_USB
    set_event(EVENT_CANFD_INIT_FAILURE, (uint8_t)errorCode);
  }
#endif  // CANFD_ADDON
}

// Transmit functions

void transmit_can(CAN_frame* tx_frame, int interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, frameDirection(MSG_TX));

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
      can.tryToSend(MCP2515Frame);
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
      send_ok = canfd.tryToSend(MCP2518Frame);
      if (!send_ok) {
        set_event(EVENT_CANFD_BUFFER_FULL, interface);
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

void send_can() {
  if (!allowed_to_send_CAN) {
    return;
  }
  send_can_battery();

#ifdef CAN_INVERTER_SELECTED
  send_can_inverter();
#endif  // CAN_INVERTER_SELECTED

#ifdef CHARGER_SELECTED
  send_can_charger();
#endif  // CHARGER_SELECTED
#ifdef CAN_SHUNT_SELECTED
  send_can_shunt();
#endif  // CAN_SHUNT_SELECTED
}

// Receive functions

void receive_can(CAN_frame* rx_frame, int interface) {
  print_can_frame(*rx_frame, frameDirection(MSG_RX));

  if (interface == can_config.battery) {
    receive_can_battery(*rx_frame);
#ifdef CHADEMO_BATTERY
    ISA_handleFrame(rx_frame);
#endif
  }
  if (interface == can_config.inverter) {
#ifdef CAN_INVERTER_SELECTED
    receive_can_inverter(*rx_frame);
#endif
  }
  if (interface == can_config.battery_double) {
#ifdef DOUBLE_BATTERY
    receive_can_battery2(*rx_frame);
#endif
  }
  if (interface == can_config.charger) {
#ifdef CHARGER_SELECTED
    receive_can_charger(*rx_frame);
#endif
  }
  if (interface == can_config.shunt) {
#ifdef CAN_SHUNT_SELECTED
    receive_can_shunt(*rx_frame);
#endif
  }
}

void receive_can_native() {  // This section checks if we have a complete CAN message incoming on native CAN port
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
    receive_can(&rx_frame, CAN_NATIVE);
  }
}

#ifdef CAN_ADDON
void receive_can_addon() {  // This section checks if we have a complete CAN message incoming on add-on CAN port
  CAN_frame rx_frame;       // Struct with our CAN format
  CANMessage MCP2515Frame;  // Struct with ACAN2515 library format, needed to use the MCP2515 library

  if (can.available()) {
    can.receive(MCP2515Frame);

    rx_frame.ID = MCP2515Frame.id;
    rx_frame.ext_ID = MCP2515Frame.ext ? CAN_frame_ext : CAN_frame_std;
    rx_frame.DLC = MCP2515Frame.len;
    for (uint8_t i = 0; i < MCP2515Frame.len && i < 8; i++) {
      rx_frame.data.u8[i] = MCP2515Frame.data[i];
    }

    //message incoming, pass it on to the handler
    receive_can(&rx_frame, CAN_ADDON_MCP2515);
  }
}
#endif  // CAN_ADDON

#ifdef CANFD_ADDON
// Functions
void receive_canfd_addon() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage frame;
  int count = 0;
  while (canfd.available() && count++ < 16) {
    canfd.receive(frame);

    CAN_frame rx_frame;
    rx_frame.ID = frame.id;
    rx_frame.ext_ID = frame.ext;
    rx_frame.DLC = frame.len;
    memcpy(rx_frame.data.u8, frame.data, MIN(rx_frame.DLC, 64));
    //message incoming, pass it on to the handler
    receive_can(&rx_frame, CANFD_ADDON_MCP2518);
    receive_can(&rx_frame, CANFD_NATIVE);
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
    offset +=
        snprintf(message_string + offset, message_string_size - offset, "%s ", (msgDir == MSG_RX) ? "RX0" : "TX1");

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
}
