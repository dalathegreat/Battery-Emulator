#include "comm_can.h"
#include <algorithm>
#include <map>
#include "../../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "../../lib/pierremolinaro-acan-esp32/ACAN_ESP32.h"
#include "../../lib/pierremolinaro-acan2515/ACAN2515.h"
#include "CanReceiver.h"
#include "USER_SETTINGS.h"
#include "comm_can.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/hal/hal.h"
#include "src/devboard/safety/safety.h"
#include "src/devboard/sdcard/sdcard.h"
#include "src/devboard/utils/logging.h"

struct CanReceiverRegistration {
  CanReceiver* receiver;
  CAN_Speed speed;
};

static std::multimap<CAN_Interface, CanReceiverRegistration> can_receivers;

const uint8_t rx_queue_size = 10;  // Receive Queue size
volatile bool send_ok_native = 0;
volatile bool send_ok_2515 = 0;
volatile bool send_ok_2518 = 0;
static unsigned long previousMillis10 = 0;

#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
const bool use_canfd_as_can_default = true;
#else
const bool use_canfd_as_can_default = false;
#endif
bool use_canfd_as_can = use_canfd_as_can_default;

void map_can_frame_to_variable(CAN_frame* rx_frame, CAN_Interface interface);

void register_can_receiver(CanReceiver* receiver, CAN_Interface interface, CAN_Speed speed) {
  can_receivers.insert({interface, {receiver, speed}});
  DEBUG_PRINTF("CAN receiver registered, total: %d\n", can_receivers.size());
}

ACAN_ESP32_Settings* settingsespcan;
ACAN_ESP32_Settings* settingsespcan2;

uint8_t user_selected_can_addon_crystal_frequency_mhz = 0;
static uint32_t QUARTZ_FREQUENCY;
SPIClass SPI2515;

ACAN2515* can2515;
ACAN2515Settings* settings2515;

static ACAN2515_Buffer16 gBuffer;

SPIClass SPI2517;
ACAN2517FD* canfd;
ACAN2517FDSettings* settings2517;

// Initialization functions

bool native_can_initialized = false;

bool init_CAN() {

  if (user_selected_can_addon_crystal_frequency_mhz > 0) {
    QUARTZ_FREQUENCY = user_selected_can_addon_crystal_frequency_mhz * 1000000UL;
  } else {
    QUARTZ_FREQUENCY = CRYSTAL_FREQUENCY_MHZ * 1000000UL;
  }

  auto nativeIt = can_receivers.find(CAN_NATIVE);
  if (nativeIt != can_receivers.end()) {
    auto se_pin = esp32hal->CAN_SE_PIN();
    auto tx_pin = esp32hal->CAN_TX_PIN();
    auto rx_pin = esp32hal->CAN_RX_PIN();

    if (se_pin != GPIO_NUM_NC) {
      if (!esp32hal->alloc_pins("CAN", se_pin)) {
        return false;
      }
      pinMode(se_pin, OUTPUT);
      digitalWrite(se_pin, LOW);
    }

    if (!esp32hal->alloc_pins("CAN", tx_pin, rx_pin)) {
      return false;
    }

    settingsespcan = new ACAN_ESP32_Settings((int)nativeIt->second.speed * 1000UL);
    settingsespcan->mRequestedCANMode = ACAN_ESP32_Settings::NormalMode;
    settingsespcan->mTxPin = tx_pin;
    settingsespcan->mRxPin = rx_pin;

    const uint32_t errorCode = ACAN_ESP32::can.begin(*settingsespcan);
    if (errorCode == 0) {
      native_can_initialized = true;
      logging.println("Native Can ok");
      logging.print("Bit Rate prescaler: ");
      logging.println(settingsespcan->mBitRatePrescaler);
      logging.print("Time Segment 1:     ");
      logging.println(settingsespcan->mTimeSegment1);
      logging.print("Time Segment 2:     ");
      logging.println(settingsespcan->mTimeSegment2);
      logging.print("RJW:                ");
      logging.println(settingsespcan->mRJW);
      logging.print("Triple Sampling:    ");
      logging.println(settingsespcan->mTripleSampling ? "yes" : "no");
      logging.print("Actual bit rate:    ");
      logging.print(settingsespcan->actualBitRate());
      logging.println(" bit/s");
      logging.print("Exact bit rate ?    ");
      logging.println(settingsespcan->exactBitRate() ? "yes" : "no");
      logging.print("Sample point:       ");
      logging.print(settingsespcan->samplePointFromBitStart());
      logging.println("%");
    } else {
      logging.print("Error Native Can: 0x");
      logging.println(errorCode, HEX);
      return false;
    }
  }

#ifdef HW_C6
  auto cannative2It = can_receivers.find(CAN_NATIVE_2);
  if (cannative2It != can_receivers.end()) {
    auto tx_pin = esp32hal->CAN2_TX_PIN();
    auto rx_pin = esp32hal->CAN2_RX_PIN();

    if (!esp32hal->alloc_pins("CAN2", tx_pin, rx_pin)) {
      return false;
    }

    settingsespcan2 = new ACAN_ESP32_Settings((int)nativeIt->second.speed * 1000UL);
    settingsespcan2->mRequestedCANMode = ACAN_ESP32_Settings::NormalMode;
    settingsespcan2->mTxPin = tx_pin;
    settingsespcan2->mRxPin = rx_pin;

    const uint32_t errorCode = ACAN_ESP32::can1.begin(*settingsespcan2);
    if (errorCode == 0) {
      //native_can_initialized = true;
      logging.println("Native Can ok");
      logging.print("Bit Rate prescaler: ");
      logging.println(settingsespcan2->mBitRatePrescaler);
      logging.print("Time Segment 1:     ");
      logging.println(settingsespcan2->mTimeSegment1);
      logging.print("Time Segment 2:     ");
      logging.println(settingsespcan2->mTimeSegment2);
      logging.print("RJW:                ");
      logging.println(settingsespcan2->mRJW);
      logging.print("Triple Sampling:    ");
      logging.println(settingsespcan2->mTripleSampling ? "yes" : "no");
      logging.print("Actual bit rate:    ");
      logging.print(settingsespcan2->actualBitRate());
      logging.println(" bit/s");
      logging.print("Exact bit rate ?    ");
      logging.println(settingsespcan2->exactBitRate() ? "yes" : "no");
      logging.print("Sample point:       ");
      logging.print(settingsespcan2->samplePointFromBitStart());
      logging.println("%");
    } else {
      logging.print("Error Native Can: 0x");
      logging.println(errorCode, HEX);
      return false;
    }
  }
#endif  // HW_C6

  auto addonIt = can_receivers.find(CAN_ADDON_MCP2515);
  if (addonIt != can_receivers.end()) {
    auto cs_pin = esp32hal->MCP2515_CS();
    auto int_pin = esp32hal->MCP2515_INT();
    auto sck_pin = esp32hal->MCP2515_SCK();
    auto miso_pin = esp32hal->MCP2515_MISO();
    auto mosi_pin = esp32hal->MCP2515_MOSI();
    auto rst_pin = esp32hal->MCP2515_RST();

    if (!esp32hal->alloc_pins("CAN", cs_pin, int_pin, sck_pin, miso_pin, mosi_pin)) {
      return false;
    }

    logging.println("Dual CAN Bus (ESP32+MCP2515) selected");
    gBuffer.initWithSize(25);

    if (rst_pin != GPIO_NUM_NC) {
      pinMode(rst_pin, OUTPUT);
      digitalWrite(rst_pin, HIGH);
      delay(100);
      digitalWrite(rst_pin, LOW);
      delay(100);
      digitalWrite(rst_pin, HIGH);
      delay(100);
    }

    can2515 = new ACAN2515(cs_pin, SPI2515, int_pin);

    SPI2515.begin(sck_pin, miso_pin, mosi_pin);

    // CAN bit rate 250 or 500 kb/s
    auto bitRate = (int)addonIt->second.speed * 1000UL;

    settings2515 = new ACAN2515Settings(QUARTZ_FREQUENCY, bitRate);
    settings2515->mRequestedMode = ACAN2515Settings::NormalMode;
    const uint16_t errorCode2515 = can2515->begin(*settings2515, [] { can2515->isr(); });
    if (errorCode2515 == 0) {
      logging.println("Can ok");
    } else {
      logging.print("Error Can: 0x");
      logging.println(errorCode2515, HEX);
      set_event(EVENT_CANMCP2515_INIT_FAILURE, (uint8_t)errorCode2515);
      return false;
    }
  }

  auto fdNativeIt = can_receivers.find(CANFD_NATIVE);
  auto fdAddonIt = can_receivers.find(CANFD_ADDON_MCP2518);

  if (fdNativeIt != can_receivers.end() || fdAddonIt != can_receivers.end()) {

    auto speed = (fdNativeIt != can_receivers.end()) ? fdNativeIt->second.speed : fdAddonIt->second.speed;

    auto cs_pin = esp32hal->MCP2517_CS();
    auto int_pin = esp32hal->MCP2517_INT();
    auto sck_pin = esp32hal->MCP2517_SCK();
    auto sdo_pin = esp32hal->MCP2517_SDO();
    auto sdi_pin = esp32hal->MCP2517_SDI();

    if (!esp32hal->alloc_pins("CAN", cs_pin, int_pin, sck_pin, sdo_pin, sdi_pin)) {
      return false;
    }

    canfd = new ACAN2517FD(cs_pin, SPI2517, int_pin);

    logging.println("CAN FD add-on (ESP32+MCP2517) selected");
    SPI2517.begin(sck_pin, sdo_pin, sdi_pin);
    auto bitRate = (int)speed * 1000UL;
    settings2517 = new ACAN2517FDSettings(
        CANFD_ADDON_CRYSTAL_FREQUENCY_MHZ, bitRate,
        DataBitRateFactor::x4);  // Arbitration bit rate: 250/500 kbit/s, data bit rate: 1/2 Mbit/s

    // ListenOnly / Normal20B / NormalFD
    settings2517->mRequestedMode = use_canfd_as_can ? ACAN2517FDSettings::Normal20B : ACAN2517FDSettings::NormalFD;

    const uint32_t errorCode2517 = canfd->begin(*settings2517, [] { canfd->isr(); });
    canfd->poll();
    if (errorCode2517 == 0) {
      logging.print("Bit Rate prescaler: ");
      logging.println(settings2517->mBitRatePrescaler);
      logging.print("Arbitration Phase segment 1: ");
      logging.print(settings2517->mArbitrationPhaseSegment1);
      logging.print(" segment 2: ");
      logging.print(settings2517->mArbitrationPhaseSegment2);
      logging.print(" SJW: ");
      logging.println(settings2517->mArbitrationSJW);
      logging.print("Actual Arbitration Bit Rate: ");
      logging.print(settings2517->actualArbitrationBitRate());
      logging.print(" bit/s");
      logging.print(" (Exact:");
      logging.println(settings2517->exactArbitrationBitRate() ? "yes)" : "no)");
      logging.print("Arbitration Sample point: ");
      logging.print(settings2517->arbitrationSamplePointFromBitStart());
      logging.println("%");
    } else {
      logging.print("CAN-FD Configuration error 0x");
      logging.println(errorCode2517, HEX);
      set_event(EVENT_CANMCP2517FD_INIT_FAILURE, (uint8_t)errorCode2517);
      return false;
    }
  }

  return true;
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, int interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, frameDirection(MSG_TX));

#ifdef ENABLE_SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    add_can_frame_to_buffer(*tx_frame, frameDirection(MSG_TX));
  }
#endif  // ENABLE_SDCARD

  switch (interface) {
    case CAN_NATIVE: {

      CANMessage frame;
      frame.id = tx_frame->ID;
      frame.ext = tx_frame->ext_ID;
      frame.len = tx_frame->DLC;
      for (uint8_t i = 0; i < frame.len; i++) {
        frame.data[i] = tx_frame->data.u8[i];
      }
      send_ok_native = ACAN_ESP32::can.tryToSend(frame);

      if (!send_ok_native) {
        datalayer.system.info.can_native_send_fail = true;
      }
    } break;
    case CAN_ADDON_MCP2515: {
      //Struct with ACAN2515 library format, needed to use the MCP2515 library for CAN2
      CANMessage MCP2515Frame;
      MCP2515Frame.id = tx_frame->ID;
      MCP2515Frame.ext = tx_frame->ext_ID;
      MCP2515Frame.len = tx_frame->DLC;
      MCP2515Frame.rtr = false;
      for (uint8_t i = 0; i < MCP2515Frame.len; i++) {
        MCP2515Frame.data[i] = tx_frame->data.u8[i];
      }

      send_ok_2515 = can2515->tryToSend(MCP2515Frame);
      if (!send_ok_2515) {
        datalayer.system.info.can_2515_send_fail = true;
      }
    } break;
    case CANFD_NATIVE:
    case CANFD_ADDON_MCP2518: {
      CANFDMessage MCP2518Frame;
      if (tx_frame->FD) {
        MCP2518Frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
      } else {  //Classic CAN message
        MCP2518Frame.type = CANFDMessage::CAN_DATA;
      }
      MCP2518Frame.id = tx_frame->ID;
      MCP2518Frame.ext = tx_frame->ext_ID;
      MCP2518Frame.len = tx_frame->DLC;
      for (uint8_t i = 0; i < MCP2518Frame.len; i++) {
        MCP2518Frame.data[i] = tx_frame->data.u8[i];
      }
      send_ok_2518 = canfd->tryToSend(MCP2518Frame);
      if (!send_ok_2518) {
        datalayer.system.info.can_2518_send_fail = true;
      }
    } break;
    default:
      // Invalid interface sent with function call. TODO: Raise event that coders messed up
      break;
  }
}

// Receive functions
void receive_can() {
  if (native_can_initialized) {
    receive_frame_can_native();  // Receive CAN messages from native CAN port
  }

  if (can2515) {
    receive_frame_can_addon();  // Receive CAN messages on add-on MCP2515 chip
  }

  if (canfd) {
    receive_frame_canfd_addon();  // Receive CAN-FD messages.
  }
}

void receive_frame_can_native() {  // This section checks if we have a complete CAN message incoming on native CAN port
  CANMessage frame;

  if (ACAN_ESP32::can.available()) {
    if (ACAN_ESP32::can.receive(frame)) {

      CAN_frame rx_frame;
      rx_frame.ID = frame.id;
      rx_frame.ext_ID = frame.ext;
      rx_frame.DLC = frame.len;
      for (uint8_t i = 0; i < frame.len && i < 8; i++) {
        rx_frame.data.u8[i] = frame.data[i];
      }

      //message incoming, pass it on to the handler
      map_can_frame_to_variable(&rx_frame, CAN_NATIVE);
    }
  }
}

void receive_frame_can_addon() {  // This section checks if we have a complete CAN message incoming on add-on CAN port
  CAN_frame rx_frame;             // Struct with our CAN format
  CANMessage MCP2515frame;        // Struct with ACAN2515 library format, needed to use the MCP2515 library

  if (can2515->available()) {
    can2515->receive(MCP2515frame);

    rx_frame.ID = MCP2515frame.id;
    rx_frame.ext_ID = MCP2515frame.ext;
    rx_frame.DLC = MCP2515frame.len;
    for (uint8_t i = 0; i < MCP2515frame.len && i < 8; i++) {
      rx_frame.data.u8[i] = MCP2515frame.data[i];
    }

    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CAN_ADDON_MCP2515);
  }
}

void receive_frame_canfd_addon() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage MCP2518frame;
  int count = 0;
  while (canfd->available() && count++ < 16) {
    canfd->receive(MCP2518frame);

    CAN_frame rx_frame;
    rx_frame.ID = MCP2518frame.id;
    rx_frame.ext_ID = MCP2518frame.ext;
    rx_frame.DLC = MCP2518frame.len;
    memcpy(rx_frame.data.u8, MCP2518frame.data, std::min(rx_frame.DLC, (uint8_t)64));
    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CANFD_ADDON_MCP2518);
    map_can_frame_to_variable(&rx_frame, CANFD_NATIVE);
  }
}

// Support functions
void print_can_frame(CAN_frame frame, frameDirection msgDir) {

  if (datalayer.system.info.CAN_usb_logging_active) {
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
  }

  if (datalayer.system.info.can_logging_active) {  // If user clicked on CAN Logging page in webserver, start recording
    dump_can_frame(frame, msgDir);
  }
}

void map_can_frame_to_variable(CAN_frame* rx_frame, CAN_Interface interface) {
  if (interface !=
      CANFD_NATIVE) {  //Avoid printing twice due to receive_frame_canfd_addon sending to both FD interfaces
    //TODO: This check can be removed later when refactored to use inline functions for logging
    print_can_frame(*rx_frame, frameDirection(MSG_RX));
  }

#ifdef ENABLE_SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    if (interface !=
        CANFD_NATIVE) {  //Avoid printing twice due to receive_frame_canfd_addon sending to both FD interfaces
      //TODO: This check can be removed later when refactored to use inline functions for logging
      add_can_frame_to_buffer(*rx_frame, frameDirection(MSG_RX));
    }
  }
#endif  // ENABLE_SDCARD

  // Send the frame to all the receivers registered for this interface.
  auto receivers = can_receivers.equal_range(interface);

  for (auto it = receivers.first; it != receivers.second; ++it) {
    auto& receiver = it->second;
    receiver.receiver->receive_can_frame(rx_frame);
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

void stop_can() {
  if (can_receivers.find(CAN_NATIVE) != can_receivers.end()) {
    ACAN_ESP32::can.end();
  }

  if (can2515) {
    can2515->end();
    SPI2515.end();
  }

  if (canfd) {
    canfd->end();
    SPI2517.end();
  }
}

void restart_can() {
  if (can_receivers.find(CAN_NATIVE) != can_receivers.end()) {
    ACAN_ESP32::can.begin(*settingsespcan);
  }

  if (can2515) {
    SPI2515.begin();
    can2515->begin(*settings2515, [] { can2515->isr(); });
  }

  if (canfd) {
    SPI2517.begin();
    canfd->begin(*settings2517, [] { can2515->isr(); });
  }
}

CAN_Speed change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  auto oldSpeed = (CAN_Speed)settingsespcan->mDesiredBitRate;
  if (interface == CAN_Interface::CAN_NATIVE) {
    settingsespcan->mDesiredBitRate = (int)speed;
    ACAN_ESP32::can.begin(*settingsespcan);
  }
  return speed;
}
