#include "comm_can.h"
#include "../../lib/mcp2515_lite/mcp2515_lite.h"
#include "../../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "../../lib/pierremolinaro-acan-esp32/ACAN_ESP32.h"
#include "CanReceiver.h"
#include "comm_can.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/safety/safety.h"
#include "src/devboard/sdcard/sdcard.h"
#include "src/devboard/utils/logging.h"
#include "utils.h"

#include <esp_private/periph_ctrl.h>

#include <algorithm>
#include <map>

volatile CAN_Configuration can_config = {.battery = CAN_NATIVE,
                                         .inverter = CAN_NATIVE,
                                         .battery_double = CAN_ADDON_MCP2515,
                                         .battery_triple = CAN_ADDON_MCP2515,
                                         .charger = CAN_NATIVE,
                                         .shunt = CAN_NATIVE};

struct CanReceiverRegistration {
  CanReceiver* receiver;
  CAN_Speed speed;
};

static std::multimap<CAN_Interface, CanReceiverRegistration> can_receivers;

static void receive_frame_can_native();
static void receive_frame_can_addon();
static void receive_frame_canfd_addon();
static void receive_frame_canfd_addon_2();
static void map_can_frame_to_variable(CAN_frame* rx_frame, CAN_Interface interface);
static void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir);
static uint32_t init_native_can(CAN_Speed speed, gpio_num_t tx_pin, gpio_num_t rx_pin);

void register_can_receiver(CanReceiver* receiver, CAN_Interface interface, CAN_Speed speed) {
  can_receivers.insert({interface, {receiver, speed}});
  DEBUG_PRINTF("CAN receiver registered, total: %d\n", can_receivers.size());
}

static ACAN_ESP32_Settings* settingsespcan = nullptr;

static uint32_t quartz_frequency;

static MCP2515_Lite* can2515;
static SPIClass* SPI2515;

static SPIClass* SPI2517;
static ACAN2517FD* canfd;
static ACAN2517FDSettings* settings2517;
static SPIClass* SPI2517_2;
static ACAN2517FD* canfd_2;
static ACAN2517FDSettings* settings2517_2;
bool use_canfd_as_can = false;
bool use_canfd2_as_can = false;
static bool native_can_initialized = false;
//CAN logging filter settings
uint16_t user_selected_CAN_ID_cutoff_filter = 0;  //Messages below this ID will not be logged in webserver

bool init_CAN() {
  // Native CAN (onboard the ESP32)

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

    const uint32_t errorCode = init_native_can(nativeIt->second.speed, tx_pin, rx_pin);
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

  // Add-on CAN interface (via MCP2515)

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

    if (rst_pin != GPIO_NUM_NC) {
      pinMode(rst_pin, OUTPUT);
      digitalWrite(rst_pin, HIGH);
      delay(100);
      digitalWrite(rst_pin, LOW);
      delay(100);
      digitalWrite(rst_pin, HIGH);
      delay(100);
    }

    SPI2515 = new SPIClass(esp32hal->MCP2515_BUS());
    SPI2515->begin(sck_pin, miso_pin, mosi_pin);
    can2515 = new MCP2515_Lite(*SPI2515, cs_pin, int_pin);

    quartz_frequency = esp32hal->MCP2515_FREQ();
    if (quartz_frequency == 0) {
      quartz_frequency = can2515->autodetectOscillatorFrequency();
    }

    if (can2515->begin({(int)addonIt->second.speed * 1000UL, quartz_frequency})) {
      logging.println("MCP2515 CAN ok");
    } else {
      logging.println("MCP2515 CAN init failed");
      set_event(EVENT_CANMCP2515_INIT_FAILURE, 1);
      return false;
    }
  }

  // FD interface(s) (via MCP2518FD)

  auto fdNativeIt = can_receivers.find(CANFD_NATIVE);
  auto fdAddonIt = can_receivers.find(CANFD_ADDON_MCP2518);
  auto fdAddonIt_2 = can_receivers.find(CANFD_ADDON_MCP2518_2);

  if (fdNativeIt != can_receivers.end() || fdAddonIt != can_receivers.end() || fdAddonIt_2 != can_receivers.end()) {
    // Initialise SPI bus first
    auto sck_pin = esp32hal->MCP2517_SCK();
    auto sdo_pin = esp32hal->MCP2517_SDO();
    auto sdi_pin = esp32hal->MCP2517_SDI();

    if (!esp32hal->alloc_pins("CANFD", sck_pin, sdo_pin, sdi_pin)) {
      return false;
    }

    SPI2517 = new SPIClass(esp32hal->MCP2517_BUS());
    SPI2517->begin(sck_pin, sdo_pin, sdi_pin);
  }

  if (fdNativeIt != can_receivers.end() || fdAddonIt != can_receivers.end()) {

    auto speed = (fdNativeIt != can_receivers.end()) ? fdNativeIt->second.speed : fdAddonIt->second.speed;

    auto cs_pin = esp32hal->MCP2517_CS();
    auto int_pin = esp32hal->MCP2517_INT();
    auto int0_pin = esp32hal->MCP2517_INT0();
    auto int1_pin = esp32hal->MCP2517_INT1();

    if (!esp32hal->alloc_pins("CANFD", cs_pin)) {
      return false;
    }
    if (int_pin != GPIO_NUM_NC) {
      if (!esp32hal->alloc_pins("CANFD", int_pin)) {
        return false;
      }
    } else {
      if (!esp32hal->alloc_pins("CANFD", int0_pin, int1_pin)) {
        return false;
      }
    }

    canfd = new ACAN2517FD(cs_pin, *SPI2517, int_pin != GPIO_NUM_NC ? int_pin : 255,
                           int0_pin != GPIO_NUM_NC ? int0_pin : 255, int1_pin != GPIO_NUM_NC ? int1_pin : 255);

    logging.println("CAN FD add-on (ESP32+MCP2517) selected");

    const uint32_t freq = esp32hal->MCP2517_FREQ();
    ACAN2517FDSettings::Oscillator osc_freq =
        (freq == 0 ? ACAN2517FDSettings::OSC_AUTODETECT
                   : (freq == 20000000 ? ACAN2517FDSettings::OSC_20MHz : ACAN2517FDSettings::OSC_40MHz));
    auto bitRate = (int)speed * 1000UL;
    settings2517 = new ACAN2517FDSettings(osc_freq, bitRate, DataBitRateFactor::x4);

    // Set up clock output divider (some hardware uses this for the second CAN FD add-on)
    settings2517->mCLKOPin = static_cast<ACAN2517FDSettings::CLKOpin>(esp32hal->MCP2517_CLKODIV());

    // ListenOnly / Normal20B / NormalFDs
    settings2517->mRequestedMode = use_canfd_as_can ? ACAN2517FDSettings::Normal20B : ACAN2517FDSettings::NormalFD;

    const uint32_t errorCode2517 = canfd->begin(*settings2517, [] { canfd->isr(); });
    canfd->poll();
    if (errorCode2517 != 0) {
      logging.print("CAN-FD Configuration error 0x");
      logging.println(errorCode2517, HEX);
      set_event(EVENT_CANMCP2518FD_INIT_FAILURE, (uint8_t)errorCode2517);
      return false;
    }
  }

  if (fdAddonIt_2 != can_receivers.end()) {

    auto cs_pin = esp32hal->MCP2517_CS2();
    auto int_pin = esp32hal->MCP2517_INT2();

    if (!esp32hal->alloc_pins("CANFD2", cs_pin, int_pin)) {
      return false;
    }

    if (esp32hal->MCP2517_BUS() == esp32hal->MCP2517_BUS2()) {
      // Use the same bus for both CAN FD chips
      SPI2517_2 = SPI2517;
    } else {
      SPI2517_2 = new SPIClass(esp32hal->MCP2517_BUS2());

      auto sck_pin = esp32hal->MCP2517_SCK2();
      auto sdo_pin = esp32hal->MCP2517_SDO2();
      auto sdi_pin = esp32hal->MCP2517_SDI2();

      if (!esp32hal->alloc_pins("CANFD2", sck_pin, sdo_pin, sdi_pin)) {
        return false;
      }

      SPI2517_2->begin(sck_pin, sdo_pin, sdi_pin);
    }

    canfd_2 = new ACAN2517FD(cs_pin, *SPI2517_2, int_pin);

    logging.println("CAN FD add-on 2 (ESP32+MCP2517) selected");

    const uint32_t freq = esp32hal->MCP2517_FREQ2();
    ACAN2517FDSettings::Oscillator osc_freq =
        (freq == 0 ? ACAN2517FDSettings::OSC_AUTODETECT
                   : (freq == 20000000 ? ACAN2517FDSettings::OSC_20MHz : ACAN2517FDSettings::OSC_40MHz));

    auto speed = fdAddonIt_2->second.speed;
    auto bitRate = (int)speed * 1000UL;
    // Crystal setting is ignored (library now autodetects)
    settings2517_2 = new ACAN2517FDSettings(osc_freq, bitRate, DataBitRateFactor::x4);
    // Arbitration bit rate: 250/500 kbit/s, data bit rate: 1/2 Mbit/s

    settings2517_2->mRequestedMode = use_canfd2_as_can ? ACAN2517FDSettings::Normal20B : ACAN2517FDSettings::NormalFD;

    const uint32_t errorCode2517_2 = canfd_2->begin(*settings2517_2, [] { canfd_2->isr(); });
    canfd_2->poll();
    if (errorCode2517_2 != 0) {
      logging.print("CAN-FD 2 Configuration error 0x");
      logging.println(errorCode2517_2, HEX);
      set_event(EVENT_CANMCP2518FD_INIT_FAILURE, (uint8_t)errorCode2517_2);
      return false;
    }
  }

  return true;
}

void transmit_can_frame_to_interface(const CAN_frame* tx_frame, CAN_Interface interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, interface, frameDirection(MSG_TX));

  if (datalayer.system.info.CAN_SD_logging_active) {
    add_can_frame_to_buffer(*tx_frame, frameDirection(MSG_TX));
  }

  switch (interface) {
    case CAN_NATIVE: {

      if (tx_frame->FD) {
        //Native does not support CAN-FD, ignore
        break;
      }

      CANMessage frame;
      frame.id = tx_frame->ID;
      frame.ext = tx_frame->ext_ID;
      frame.len = tx_frame->DLC;
      for (uint8_t i = 0; i < frame.len; i++) {
        frame.data[i] = tx_frame->data.u8[i];
      }

      if (!ACAN_ESP32::can.tryToSend(frame)) {
        datalayer.system.info.can_native_send_fail = true;
      }
    } break;
    case CAN_ADDON_MCP2515: {
      MCP2515_Lite_Frame mcp2515_frame;
      copy_can_frame_to_mcp2515_lite_frame(*tx_frame, mcp2515_frame);

      if (!can2515->sendFrame(mcp2515_frame)) {
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
      memcpy(MCP2518Frame.data, tx_frame->data.u8, std::min(tx_frame->DLC, (uint8_t)sizeof(MCP2518Frame.data)));

      if (!canfd->tryToSend(MCP2518Frame)) {
        datalayer.system.info.can_2518_send_fail = true;
      }
    } break;
    case CANFD_ADDON_MCP2518_2: {
      CANFDMessage MCP2518Frame;
      if (tx_frame->FD) {
        MCP2518Frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
      } else {  //Classic CAN message
        MCP2518Frame.type = CANFDMessage::CAN_DATA;
      }
      MCP2518Frame.id = tx_frame->ID;
      MCP2518Frame.ext = tx_frame->ext_ID;
      MCP2518Frame.len = tx_frame->DLC;
      memcpy(MCP2518Frame.data, tx_frame->data.u8, std::min(tx_frame->DLC, (uint8_t)sizeof(MCP2518Frame.data)));

      if (!canfd_2->tryToSend(MCP2518Frame)) {
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

  if (canfd_2) {
    receive_frame_canfd_addon_2();  // Receive CAN-FD messages on 2nd CAN-FD add-on.
  }
}

static void
receive_frame_can_native() {  // This section checks if we have a complete CAN message incoming on native CAN port
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

static void
receive_frame_can_addon() {  // This section checks if we have a complete CAN message incoming on add-on CAN port
  MCP2515_Lite_Frame rx_frame;
  CAN_frame full_frame;

  int count = 0;
  while (count++ < 16 && can2515->receiveFrame(rx_frame)) {
    copy_mcp2515_lite_frame_to_can_frame(rx_frame, full_frame);
    map_can_frame_to_variable(&full_frame, CAN_ADDON_MCP2515);
  }
}

static void receive_frame_canfd_addon() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage MCP2518frame;
  int count = 0;
  while (canfd->available() && count++ < 16) {
    canfd->receive(MCP2518frame);

    CAN_frame rx_frame;
    rx_frame.ID = MCP2518frame.id;
    rx_frame.ext_ID = MCP2518frame.ext;
    rx_frame.DLC = MCP2518frame.len;
    memcpy(rx_frame.data.u8, MCP2518frame.data, std::min(rx_frame.DLC, (uint8_t)sizeof(rx_frame.data.u8)));
    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CANFD_ADDON_MCP2518);
    map_can_frame_to_variable(&rx_frame, CANFD_NATIVE);
  }
}

static void
receive_frame_canfd_addon_2() {  // This section checks if we have a complete CAN-FD message incoming on 2nd CAN-FD add-on
  CANFDMessage MCP2518frame;
  int count = 0;
  while (canfd_2->available() && count++ < 16) {
    canfd_2->receive(MCP2518frame);

    CAN_frame rx_frame;
    rx_frame.ID = MCP2518frame.id;
    rx_frame.ext_ID = MCP2518frame.ext;
    rx_frame.DLC = MCP2518frame.len;
    memcpy(rx_frame.data.u8, MCP2518frame.data, std::min(rx_frame.DLC, (uint8_t)sizeof(rx_frame.data.u8)));
    //message incoming, pass it on to the handler
    map_can_frame_to_variable(&rx_frame, CANFD_ADDON_MCP2518_2);
  }
}

// Support functions
static void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir) {

  if (datalayer.system.info.CAN_usb_logging_active) {
    uint8_t i = 0;
    Serial.print("(");
    Serial.print(millis() / 1000.0);
    if (msgDir == MSG_RX) {
      Serial.print(") RX");
      Serial.print((int)(interface * 2));
    } else {
      Serial.print(") TX");
      Serial.print((int)(interface * 2) + 1);
    }
    Serial.print(" ");
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
    if (frame.ID > user_selected_CAN_ID_cutoff_filter) {  //Only log the message if CAN ID is higher than user set value
      dump_can_frame(frame, interface, msgDir);
    }
  }
}

static void map_can_frame_to_variable(CAN_frame* rx_frame, CAN_Interface interface) {
  if (interface !=
      CANFD_NATIVE) {  //Avoid printing twice due to receive_frame_canfd_addon sending to both FD interfaces
    //TODO: This check can be removed later when refactored to use inline functions for logging
    print_can_frame(*rx_frame, interface, frameDirection(MSG_RX));
  }

  if (datalayer.system.info.CAN_SD_logging_active) {
    if (interface !=
        CANFD_NATIVE) {  //Avoid printing twice due to receive_frame_canfd_addon sending to both FD interfaces
      //TODO: This check can be removed later when refactored to use inline functions for logging
      add_can_frame_to_buffer(*rx_frame, frameDirection(MSG_RX));
    }
  }

  // Send the frame to all the receivers registered for this interface.
  auto receivers = can_receivers.equal_range(interface);

  for (auto it = receivers.first; it != receivers.second; ++it) {
    auto& receiver = it->second;
    receiver.receiver->receive_can_frame(rx_frame);
  }
}

void dump_can_frame(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {
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

  // Add direction. Multiplying the interface by two ensures that SavvyCAN puts TX and RX in a different bus.
  offset += snprintf(message_string + offset, message_string_size - offset, "%s%d ", (msgDir == MSG_RX) ? "RX" : "TX",
                     (int)(interface * 2) + (msgDir == MSG_RX ? 0 : 1));

  // Add ID and DLC
  offset += snprintf(message_string + offset, message_string_size - offset, "%lX [%u] ", frame.ID, frame.DLC);

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
    can2515->pause(true);
  }

  if (canfd) {
    canfd->end();
  }
}

void restart_can() {
  if (can_receivers.find(CAN_NATIVE) != can_receivers.end()) {
    ACAN_ESP32::can.begin(*settingsespcan);
  }

  if (can2515) {
    can2515->pause(false);
  }

  if (canfd) {
    canfd->begin(*settings2517, [] { canfd->isr(); });
    canfd->poll();
  }
}

// Initialize the native CAN interface with the given speed and pins.
// This can be called repeatedly to change the interface speed (as some
// batteries require).
static uint32_t init_native_can(CAN_Speed speed, gpio_num_t tx_pin, gpio_num_t rx_pin) {

  // TODO: check whether this is necessary? It seems to help with
  // reinitialization.
  periph_module_reset(PERIPH_TWAI_MODULE);

  if (settingsespcan != nullptr) {
    delete settingsespcan;
  }

  // Create a new settings object (as it does the bitrate calcs in the constructor)
  settingsespcan = new ACAN_ESP32_Settings((int)speed * 1000UL);
  settingsespcan->mRequestedCANMode = ACAN_ESP32_Settings::NormalMode;
  settingsespcan->mTxPin = tx_pin;
  settingsespcan->mRxPin = rx_pin;

  // (Re)start the CAN interface
  return ACAN_ESP32::can.begin(*settingsespcan);
}

// Change the speed of the given CAN interface. Returns true if successful.
bool change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  if (interface == CAN_Interface::CAN_NATIVE && settingsespcan != nullptr) {
    // Reinitialize the native CAN interface with the new speed
    const uint32_t errorCode = init_native_can(speed, settingsespcan->mTxPin, settingsespcan->mRxPin);
    if (errorCode != 0) {
      logging.print("Error Native Can: 0x");
      logging.println(errorCode, HEX);
      return false;
    }
    return true;
  } else if (interface == CAN_Interface::CAN_ADDON_MCP2515 && can2515) {
    can2515->changeSpeed({(int)speed * 1000UL, quartz_frequency});
    return true;
  }

  return false;
}
