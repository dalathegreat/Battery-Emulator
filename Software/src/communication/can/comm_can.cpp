#include "comm_can.h"
#include "../../lib/mcp2515_lite/mcp2515_lite.h"
#include "../../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "../../lib/pierremolinaro-acan-esp32/ACAN_ESP32.h"
#include "CanReceiver.h"
#include "comm_can_dispatch.h"
#include "comm_can_device.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/hal/hal.h"
#include "src/devboard/safety/safety.h"
#include "src/devboard/sdcard/sdcard.h"
#include "src/devboard/utils/events.h"
#include "src/devboard/utils/logging.h"
#include "src/devboard/webserver/webserver_can_streaming.h"
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

static void dispatch_frame(CAN_frame* rx_frame, class CanDevice* dev);
static void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir);

// The native TWAI controller on the ESP32 itself.
class NativeTwaiDevice : public CanDevice {
 public:
  NativeTwaiDevice() {
    name = "Native CAN";
    log_interface = CAN_NATIVE;
    device_index = 0;
    buffer_full_event = EVENT_CAN_NATIVE_BUFFER_FULL;
    bus_error_event = EVENT_CAN_NATIVE_BUS_ERROR;
  }

  bool init(CAN_Speed speed) override {
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

    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;

    const uint32_t errorCode = begin_native(speed);
    if (errorCode == 0) {
      initialized = true;
      logging.println("Native Can ok");
      logging.print("Bit Rate prescaler: ");
      logging.println(settings_->mBitRatePrescaler);
      logging.print("Time Segment 1:     ");
      logging.println(settings_->mTimeSegment1);
      logging.print("Time Segment 2:     ");
      logging.println(settings_->mTimeSegment2);
      logging.print("RJW:                ");
      logging.println(settings_->mRJW);
      logging.print("Triple Sampling:    ");
      logging.println(settings_->mTripleSampling ? "yes" : "no");
      logging.print("Actual bit rate:    ");
      logging.print(settings_->actualBitRate());
      logging.println(" bit/s");
      logging.print("Exact bit rate ?    ");
      logging.println(settings_->exactBitRate() ? "yes" : "no");
      logging.print("Sample point:       ");
      logging.print(settings_->samplePointFromBitStart());
      logging.println("%");
      return true;
    }
    logging.print("Error Native Can: 0x");
    logging.println(errorCode, HEX);
    return false;
  }

  bool try_send(const CAN_frame& tx_frame) override {
    if (tx_frame.DLC > sizeof(CANMessage::data)) {
      // An FD-length frame cannot be sent on a classic CAN interface - a CAN-FD battery
      // configured on one produces these - and the copy below would overflow frame.data on
      // the stack. Refusing it raises the send-fail flag through the caller, exactly as the
      // hand-written branch did.
      return false;
    }
    CANMessage frame;
    frame.id = tx_frame.ID;
    frame.ext = tx_frame.ext_ID;
    frame.len = tx_frame.DLC;
    for (uint8_t i = 0; i < frame.len; i++) {
      frame.data[i] = tx_frame.data.u8[i];
    }
    return ACAN_ESP32::can.tryToSend(frame);
  }

  void poll_receive() override {
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
        dispatch_frame(&rx_frame, this);
      }
    }

    auto flags = ACAN_ESP32::can.statusRegister();
    if ((flags & TWAI_BUS_OFF_ST) != 0) {
      // Bus off, reset the CAN controller
      change_speed(speed_);
      datalayer.system.info.can_device[device_index].bus_error = true;
    }
    if ((flags & TWAI_ERR_ST) != 0) {
      datalayer.system.info.can_device[device_index].bus_error = true;
    }
  }

  void stop() override { ACAN_ESP32::can.end(); }

  void restart() override { ACAN_ESP32::can.begin(*settings_); }

  bool change_speed(CAN_Speed speed) override {
    if (settings_ == nullptr) {
      return false;
    }
    // Reinitialize the native CAN interface with the new speed
    const uint32_t errorCode = begin_native(speed);
    if (errorCode != 0) {
      logging.print("Error Native Can: 0x");
      logging.println(errorCode, HEX);
      return false;
    }
    return true;
  }

 private:
  // (Re)starts the controller with the given speed on the stored pins. This
  // can be called repeatedly to change the interface speed (as some batteries
  // require).
  uint32_t begin_native(CAN_Speed speed) {
    // TODO: check whether this is necessary? It seems to help with
    // reinitialization.
    periph_module_reset(PERIPH_TWAI_MODULE);

    if (settings_ != nullptr) {
      delete settings_;
    }

    speed_ = speed;

    // Create a new settings object (as it does the bitrate calcs in the constructor)
    settings_ = new ACAN_ESP32_Settings((int)speed * 1000UL);
    settings_->mRequestedCANMode = ACAN_ESP32_Settings::NormalMode;
    settings_->mTxPin = tx_pin_;
    settings_->mRxPin = rx_pin_;

    // (Re)start the CAN interface
    return ACAN_ESP32::can.begin(*settings_);
  }

  ACAN_ESP32_Settings* settings_ = nullptr;
  CAN_Speed speed_;
  gpio_num_t tx_pin_;
  gpio_num_t rx_pin_;
};

// MCP2515 add-on controller (classic CAN over SPI).
class Mcp2515Device : public CanDevice {
 public:
  Mcp2515Device() {
    name = "MCP2515";
    log_interface = CAN_ADDON_MCP2515;
    device_index = 1;
    buffer_full_event = EVENT_CANMCP2515_BUFFER_FULL;
    bus_error_event = EVENT_CANMCP2515_BUS_ERROR;
  }

  bool init(CAN_Speed speed) override {
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

    spi_ = new SPIClass(esp32hal->MCP2515_BUS());
    spi_->begin(sck_pin, miso_pin, mosi_pin);
    can_ = new MCP2515_Lite(*spi_, cs_pin, int_pin);

    quartz_frequency_ = esp32hal->MCP2515_FREQ();
    if (quartz_frequency_ == 0) {
      quartz_frequency_ = can_->autodetectOscillatorFrequency();
    }

    if (can_->begin({(int)speed * 1000UL, quartz_frequency_})) {
      logging.println("MCP2515 CAN ok");
      initialized = true;
      return true;
    }
    logging.println("MCP2515 CAN init failed");
    set_event(EVENT_CANMCP2515_INIT_FAILURE, 1);
    // This will leak, but we have failed and won't try to reinit.
    can_ = nullptr;
    return false;
  }

  bool try_send(const CAN_frame& tx_frame) override {
    if (tx_frame.DLC > sizeof(MCP2515_Lite_Frame::data)) {
      // Same as the native interface: an FD-length frame cannot travel over the MCP2515,
      // and refusing it here raises the send-fail flag through the caller.
      return false;
    }
    MCP2515_Lite_Frame mcp2515_frame;
    copy_can_frame_to_mcp2515_lite_frame(tx_frame, mcp2515_frame);
    return can_ != nullptr && can_->sendFrame(mcp2515_frame);
  }

  void poll_receive() override {
    MCP2515_Lite_Frame rx_frame;
    CAN_frame full_frame;

    int count = 0;
    while (count++ < 16 && can_->receiveFrame(rx_frame)) {
      copy_mcp2515_lite_frame_to_can_frame(rx_frame, full_frame);
      dispatch_frame(&full_frame, this);
    }

    if (can_->hasErrors()) {
      datalayer.system.info.can_device[device_index].bus_error = true;
    }
  }

  void stop() override { can_->pause(true); }

  void restart() override { can_->pause(false); }

  bool change_speed(CAN_Speed speed) override {
    can_->changeSpeed({(int)speed * 1000UL, quartz_frequency_});
    return true;
  }

 private:
  MCP2515_Lite* can_ = nullptr;
  SPIClass* spi_ = nullptr;
  uint32_t quartz_frequency_ = 0;
};

class Mcp2518Device;

// The ACAN2517FD begin() callback must be a captureless function pointer, so
// the two FD instances are reachable through this array for their ISR
// trampolines.
static Mcp2518Device* fd_instances[2] = {nullptr, nullptr};

// Logical interface -> physical device. Entries may repeat: on boards where
// CANFD_NATIVE and CANFD_ADDON_MCP2518 are the same chip, both slots point at
// the same device, and dispatch_frame delivers each physical frame to the
// receivers of every logical interface mapped to it.
static CanDevice* device_for[NO_CAN_INTERFACE] = {};

static std::vector<CanDevice*> all_devices;

const std::vector<CanDevice*>& unique_can_devices() {
  return all_devices;
}

// MCP2518FD add-on controller (CAN-FD over SPI). Instantiated twice on boards
// with two FD chips; the SPI bus wiring (including bus sharing between the
// two instances) is board topology and stays in init_CAN().
class Mcp2518Device : public CanDevice {
 public:
  struct Config {
    uint8_t index;  // 0 or 1, selects the ISR trampoline
    gpio_num_t cs_pin;
    gpio_num_t int_pin;  // GPIO_NUM_NC on instance 1 means use the INT0/INT1 pair
    gpio_num_t int0_pin;
    gpio_num_t int1_pin;
    SPIClass* spi;
    uint32_t freq;  // 0 = autodetect
    bool set_clko;  // instance 1 only: program the clock output divider
    uint8_t clko_div;
    const char* selected_log;
    const char* error_log_prefix;
  };

  explicit Mcp2518Device(const Config& config) : cfg_(config) {
    name = (cfg_.index == 0) ? "CAN-FD" : "CAN-FD 2";
    log_interface = (cfg_.index == 0) ? CANFD_ADDON_MCP2518 : CANFD_ADDON_MCP2518_2;
    device_index = (cfg_.index == 0) ? 2 : 3;
    buffer_full_event = (cfg_.index == 0) ? EVENT_CANFD_BUFFER_FULL : EVENT_CANFD_2_BUFFER_FULL;
    bus_error_event = (cfg_.index == 0) ? EVENT_CANFD_BUS_ERROR : EVENT_CANFD_2_BUS_ERROR;
    init_fail_event_ = (cfg_.index == 0) ? EVENT_CANMCP2518FD_INIT_FAILURE : EVENT_CANMCP2518FD_2_INIT_FAILURE;
    fd_instances[cfg_.index] = this;
  }

  bool init(CAN_Speed speed) override {
    can_ = new ACAN2517FD(cfg_.cs_pin, *cfg_.spi, cfg_.int_pin != GPIO_NUM_NC ? cfg_.int_pin : 255,
                          cfg_.int0_pin != GPIO_NUM_NC ? cfg_.int0_pin : 255,
                          cfg_.int1_pin != GPIO_NUM_NC ? cfg_.int1_pin : 255);

    logging.println(cfg_.selected_log);

    ACAN2517FDSettings::Oscillator osc_freq =
        (cfg_.freq == 0 ? ACAN2517FDSettings::OSC_AUTODETECT
                        : (cfg_.freq == 20000000 ? ACAN2517FDSettings::OSC_20MHz : ACAN2517FDSettings::OSC_40MHz));
    auto bitRate = (int)speed * 1000UL;
    settings_ = new ACAN2517FDSettings(osc_freq, bitRate, DataBitRateFactor::x4);

    if (cfg_.set_clko) {
      // Set up clock output divider (some hardware uses this for the second CAN FD add-on)
      settings_->mCLKOPin = static_cast<ACAN2517FDSettings::CLKOpin>(cfg_.clko_div);
    }

    // ListenOnly / Normal20B / NormalFDs
    settings_->mRequestedMode =
        ACAN2517FDSettings::NormalFD;  //Startup in NormalFD mode, both for Classic CAN and CAN-FD messages

    if (!begin()) {
      return false;
    }
    initialized = true;
    return true;
  }

  bool try_send(const CAN_frame& tx_frame) override {
    CANFDMessage MCP2518Frame;
    if (tx_frame.FD) {
      MCP2518Frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
    } else {  //Classic CAN message
      MCP2518Frame.type = CANFDMessage::CAN_DATA;
    }
    MCP2518Frame.id = tx_frame.ID;
    MCP2518Frame.ext = tx_frame.ext_ID;
    MCP2518Frame.len = tx_frame.DLC;
    memcpy(MCP2518Frame.data, tx_frame.data.u8, std::min(tx_frame.DLC, (uint8_t)sizeof(MCP2518Frame.data)));

    return can_ != nullptr && can_->tryToSend(MCP2518Frame);
  }

  void poll_receive() override {
    CANFDMessage MCP2518frame;
    int count = 0;
    while (can_->available() && count++ < 16) {
      can_->receive(MCP2518frame);

      CAN_frame rx_frame;
      rx_frame.ID = MCP2518frame.id;
      rx_frame.ext_ID = MCP2518frame.ext;
      rx_frame.DLC = MCP2518frame.len;
      rx_frame.FD = (MCP2518frame.type == CANFDMessage::CANFD_NO_BIT_RATE_SWITCH ||
                     MCP2518frame.type == CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH);
      memcpy(rx_frame.data.u8, MCP2518frame.data, std::min(rx_frame.DLC, (uint8_t)sizeof(rx_frame.data.u8)));
      //message incoming, pass it on to the handler
      dispatch_frame(&rx_frame, this);
    }

    if (can_->hasCanErrors()) {
      datalayer.system.info.can_device[device_index].bus_error = true;
    }
  }

  void stop() override { can_->end(); }

  void restart() override { begin(); }

  void run_isr() { can_->isr(); }

 private:
  // Starts the controller; shared by init() and restart(). On failure the
  // device marks itself dead (can_ nulled, initialized false) so the receive
  // and transmit paths skip it, matching the previous nulled-pointer state.
  bool begin() {
    const uint32_t errorCode =
        can_->begin(*settings_, cfg_.index == 0 ? +[] { fd_instances[0]->run_isr(); }
                                                : +[] { fd_instances[1]->run_isr(); });
    can_->poll();
    if (errorCode != 0) {
      logging.print(cfg_.error_log_prefix);
      logging.println(errorCode, HEX);
      set_event(init_fail_event_, (uint8_t)errorCode);
      // This will leak, but we have failed and won't try to reinit.
      can_ = nullptr;
      initialized = false;
      return false;
    }
    return true;
  }

  Config cfg_;
  ACAN2517FD* can_ = nullptr;
  ACAN2517FDSettings* settings_ = nullptr;
  EVENTS_ENUM_TYPE init_fail_event_ = EVENT_NOF_EVENTS;
};

//CAN logging filter settings
uint16_t user_selected_CAN_ID_cutoff_filter = 0;  //Messages below this ID will not be logged in webserver

bool init_CAN() {
  // Native CAN (onboard the ESP32)

  if (can_receiver_registered(CAN_NATIVE)) {
    NativeTwaiDevice* native_dev = new NativeTwaiDevice();
    device_for[CAN_NATIVE] = native_dev;
    all_devices.push_back(native_dev);
    if (!native_dev->init(can_receiver_speed(CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS))) {
      return false;
    }
  }

  // Add-on CAN interface (via MCP2515)

  if (can_receiver_registered(CAN_ADDON_MCP2515)) {
    Mcp2515Device* mcp2515_dev = new Mcp2515Device();
    device_for[CAN_ADDON_MCP2515] = mcp2515_dev;
    all_devices.push_back(mcp2515_dev);
    if (!mcp2515_dev->init(can_receiver_speed(CAN_ADDON_MCP2515, CAN_Speed::CAN_SPEED_500KBPS))) {
      return false;
    }
  }

  // FD interface(s) (via MCP2518FD)

  const bool fdNativeWanted = can_receiver_registered(CANFD_NATIVE);
  const bool fdAddonWanted = can_receiver_registered(CANFD_ADDON_MCP2518);
  const bool fdAddon2Wanted = can_receiver_registered(CANFD_ADDON_MCP2518_2);

  SPIClass* spi2517 = nullptr;

  if (fdNativeWanted || fdAddonWanted || fdAddon2Wanted) {
    // Initialise SPI bus first
    auto sck_pin = esp32hal->MCP2517_SCK();
    auto sdo_pin = esp32hal->MCP2517_SDO();
    auto sdi_pin = esp32hal->MCP2517_SDI();

    if (!esp32hal->alloc_pins("CANFD", sck_pin, sdo_pin, sdi_pin)) {
      return false;
    }

    spi2517 = new SPIClass(esp32hal->MCP2517_BUS());
    spi2517->begin(sck_pin, sdo_pin, sdi_pin);
  }

  if (fdNativeWanted || fdAddonWanted) {

    auto speed = fdNativeWanted ? can_receiver_speed(CANFD_NATIVE, CAN_Speed::CAN_SPEED_500KBPS) : can_receiver_speed(CANFD_ADDON_MCP2518, CAN_Speed::CAN_SPEED_500KBPS);

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

    Mcp2518Device* fd_dev = new Mcp2518Device({.index = 0,
                                               .cs_pin = cs_pin,
                                               .int_pin = int_pin,
                                               .int0_pin = int0_pin,
                                               .int1_pin = int1_pin,
                                               .spi = spi2517,
                                               .freq = esp32hal->MCP2517_FREQ(),
                                               .set_clko = true,
                                               .clko_div = static_cast<uint8_t>(esp32hal->MCP2517_CLKODIV()),
                                               .selected_log = "CAN FD add-on (ESP32+MCP2517) selected",
                                               .error_log_prefix = "CAN-FD Configuration error 0x"});
    // One physical chip serves both logical FD names on boards that expose both.
    device_for[CANFD_NATIVE] = fd_dev;
    device_for[CANFD_ADDON_MCP2518] = fd_dev;
    all_devices.push_back(fd_dev);
    if (!fd_dev->init(speed)) {
      return false;
    }
  }

  if (fdAddon2Wanted) {

    auto cs_pin = esp32hal->MCP2517_CS2();
    auto int_pin = esp32hal->MCP2517_INT2();

    if (!esp32hal->alloc_pins("CANFD2", cs_pin, int_pin)) {
      return false;
    }

    SPIClass* spi2517_2 = nullptr;

    if (esp32hal->MCP2517_BUS() == esp32hal->MCP2517_BUS2()) {
      // Use the same bus for both CAN FD chips
      spi2517_2 = spi2517;
    } else {
      spi2517_2 = new SPIClass(esp32hal->MCP2517_BUS2());

      auto sck_pin = esp32hal->MCP2517_SCK2();
      auto sdo_pin = esp32hal->MCP2517_SDO2();
      auto sdi_pin = esp32hal->MCP2517_SDI2();

      if (!esp32hal->alloc_pins("CANFD2", sck_pin, sdo_pin, sdi_pin)) {
        return false;
      }

      spi2517_2->begin(sck_pin, sdo_pin, sdi_pin);
    }

    Mcp2518Device* fd_dev_2 = new Mcp2518Device({.index = 1,
                                                 .cs_pin = cs_pin,
                                                 .int_pin = int_pin,
                                                 .int0_pin = GPIO_NUM_NC,
                                                 .int1_pin = GPIO_NUM_NC,
                                                 .spi = spi2517_2,
                                                 .freq = esp32hal->MCP2517_FREQ2(),
                                                 .set_clko = false,
                                                 .clko_div = 0,
                                                 .selected_log = "CAN FD add-on 2 (ESP32+MCP2517) selected",
                                                 .error_log_prefix = "CAN-FD 2 Configuration error 0x"});
    device_for[CANFD_ADDON_MCP2518_2] = fd_dev_2;
    all_devices.push_back(fd_dev_2);
    if (!fd_dev_2->init(can_receiver_speed(CANFD_ADDON_MCP2518_2, CAN_Speed::CAN_SPEED_500KBPS))) {
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

#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    add_can_frame_to_buffer(*tx_frame, interface, frameDirection(MSG_TX));
  }
#endif

  CanDevice* dev = (interface < NO_CAN_INTERFACE) ? device_for[interface] : nullptr;
  if (dev == nullptr) {
    // Invalid interface sent with function call. TODO: Raise event that coders messed up
    return;
  }

  if (!dev->try_send(*tx_frame)) {
    datalayer.system.info.can_device[dev->device_index].send_fail = true;
  }
}

// Receive functions
void receive_can() {
  for (CanDevice* dev : all_devices) {
    if (dev->initialized) {
      dev->poll_receive();
    }
  }
}

// Support functions
static void print_can_frame(CAN_frame frame, CAN_Interface interface, frameDirection msgDir) {

  if (datalayer.system.info.CAN_usb_logging_active) {
    // Build the whole line first, then write it in one go - and only if the TX
    // buffer has room. This path runs in the core task: a blocked/slow USB host
    // must never stall it (EVENT_TASK_OVERRUN). Frames that don't fit are
    // counted and reported as a gap marker once the port drains.
    static char usb_line[288];  // header + up to 64 CAN-FD data bytes at 3 chars each
    static uint32_t usb_frames_dropped = 0;
    unsigned long currentTime = millis();
    size_t size = snprintf(usb_line, sizeof(usb_line), "(%lu.%02lu) %s%d %lX [%u] ", currentTime / 1000,
                           (currentTime % 1000) / 10, (msgDir == MSG_RX) ? "RX" : "TX",
                           (msgDir == MSG_RX) ? (int)(interface * 2) : (int)(interface * 2) + 1, frame.ID, frame.DLC);
    for (uint8_t i = 0; i < frame.DLC; i++) {
      size += snprintf(usb_line + size, sizeof(usb_line) - size, (i < frame.DLC - 1) ? "%02X " : "%02X\r\n",
                       frame.data.u8[i]);
    }
    if (frame.DLC == 0) {
      size += snprintf(usb_line + size, sizeof(usb_line) - size, "\r\n");
    }

    if ((size_t)Serial.availableForWrite() >= size) {
      if (usb_frames_dropped > 0) {
        char marker[48];
        int marker_len =
            snprintf(marker, sizeof(marker), "[%lu CAN frames not printed]\r\n", (unsigned long)usb_frames_dropped);
        if ((size_t)Serial.availableForWrite() >= size + (size_t)marker_len) {
          Serial.write((const uint8_t*)marker, marker_len);
          usb_frames_dropped = 0;
        }
      }
      Serial.write((const uint8_t*)usb_line, size);
    } else {
      usb_frames_dropped++;
    }
  }

  if (datalayer.system.info.can_logging_active) {  // If user clicked on CAN Logging page in webserver, start recording
    if (frame.ID > user_selected_CAN_ID_cutoff_filter) {  //Only log the message if CAN ID is higher than user set value
      dump_can_frame(frame, interface, msgDir);
    }
  }
  if (datalayer.system.info.can_streaming_active) {
    stream_can_frame(frame, interface, msgDir);
  }
}

// Called by each device's poll_receive() once per received physical frame:
// log it once, then deliver it to the receivers of every logical interface
// mapped to the device.
static void dispatch_frame(CAN_frame* rx_frame, CanDevice* dev) {
  print_can_frame(*rx_frame, dev->log_interface, frameDirection(MSG_RX));

#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    add_can_frame_to_buffer(*rx_frame, dev->log_interface, frameDirection(MSG_RX));
  }
#endif

  for (int i = 0; i < NO_CAN_INTERFACE; i++) {
    if (device_for[i] != dev) {
      continue;
    }
    deliver_to_receivers(rx_frame, static_cast<CAN_Interface>(i));
  }
}

// For formatting CAN frames considerably faster than using snprintf
static const char* hex = "0123456789abcdef";

static char* put_hex(char* ptr, uint32_t value, uint8_t digits) {
  for (int i = digits - 1; i >= 0; i--) {
    *ptr++ = hex[(value >> (i * 4)) & 0x0f];
  }
  return ptr;
}

static char* put_time(char* ptr, unsigned long time) {
  // Wrap around after 100000 seconds (about 27.7 hours)
  if (time >= 100000000)
    time = time % 100000000;

  char buf[8];
  int i = 0;
  do {
    buf[i++] = (time % 10) + '0';
    time /= 10;
  } while (time > 0);
  while (i > 0) {
    *ptr++ = buf[--i];
    if (i == 3) {
      *ptr++ = '.';
    }
  }
  return ptr;
}

// CAN log formatter: "(12345.678) RX0 123 [8] 01 02 03 ... 0A\n".
size_t format_can_frame(char* buffer, size_t len, const CAN_frame& frame, CAN_Interface interface,
                        frameDirection msgDir) {
  // Worst-case line length: '(' + up-to-8-digit time + optional '.' + ')' + ' '
  // + "RX"/"TX" + channel digit + ' ' + 8-hex ID + ' ' + '[' + 2-digit DLC + ']'
  // + 3 bytes per data byte + '\n'.
  const size_t needed = 1 + 9 + 1 + 1 + 3 + 1 + 8 + 1 + 1 + 2 + 1 + (size_t)frame.DLC * 3 + 1;
  if (needed > len) {
    if (len > 0) {
      buffer[0] = '\0';
    }
    return 0;
  }

  char* ptr = buffer;
  const unsigned long currentTime = millis();
  *ptr++ = '(';
  ptr = put_time(ptr, currentTime);
  *ptr++ = ')';
  *ptr++ = ' ';
  if (msgDir == MSG_RX) {
    *ptr++ = frame.FD ? 'R' : 'r';
    *ptr++ = frame.FD ? 'X' : 'x';
    *ptr++ = '0' + ((int)interface * 2);
  } else {
    *ptr++ = frame.FD ? 'T' : 't';
    *ptr++ = frame.FD ? 'X' : 'x';
    *ptr++ = '1' + ((int)interface * 2);
  }
  *ptr++ = ' ';
  if (frame.ext_ID)
    ptr = put_hex(ptr, frame.ID, 8);
  else
    ptr = put_hex(ptr, frame.ID, 3);
  *ptr++ = ' ';
  *ptr++ = '[';
  if (frame.DLC > 9) {
    *ptr++ = '0' + (frame.DLC / 10);
    *ptr++ = '0' + (frame.DLC % 10);
  } else
    *ptr++ = '0' + (frame.DLC);
  *ptr++ = ']';
  for (int i = 0; i < frame.DLC; i++) {
    *ptr++ = ' ';
    ptr = put_hex(ptr, frame.data.u8[i], 2);
  }
  *ptr++ = '\n';
  *ptr = '\0';
  return (size_t)(ptr - buffer);
}

void dump_can_frame(CAN_frame& frame, CAN_Interface interface, frameDirection msgDir) {
  char* message_string = datalayer.system.info.logged_can_messages;
  size_t offset =
      datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

  size_t written = format_can_frame(message_string + offset, message_string_size - offset, frame, interface, msgDir);
  if (written == 0 && offset != 0) {
    // Not enough space left at the tail - wrap around and start from the beginning
    offset = 0;
    written = format_can_frame(message_string, message_string_size, frame, interface, msgDir);
  }
  if (written > 0) {
    datalayer.system.info.logged_can_messages_offset = offset + written;  // Update offset in buffer
  }
}

void stop_can() {
  for (CanDevice* dev : all_devices) {
    if (dev->initialized) {
      dev->stop();
    }
  }
}

void restart_can() {
  for (CanDevice* dev : all_devices) {
    if (dev->initialized) {
      dev->restart();
    }
  }
}

// Change the speed of the given CAN interface. Returns true if successful.
// Devices without runtime speed change (the FD controllers) inherit the base
// class refusal.
bool change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  CanDevice* dev = (interface < NO_CAN_INTERFACE) ? device_for[interface] : nullptr;
  if (dev == nullptr || !dev->initialized) {
    return false;
  }
  return dev->change_speed(speed);
}
