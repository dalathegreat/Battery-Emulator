#include "comm_can.h"
#include "../../lib/mcp2515_lite/mcp2515_lite.h"
#include "../../lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
#include "../../lib/pierremolinaro-acan-esp32/ACAN_ESP32.h"
#include "CanReceiver.h"
#include "comm_can_device.h"
#include "comm_can_dispatch.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/hal/hal.h"
#include "src/devboard/safety/safety.h"
#include "src/devboard/sdcard/sdcard.h"
#include "src/devboard/utils/events.h"
#include "src/devboard/utils/logging.h"
#include "utils.h"

#include <esp_private/periph_ctrl.h>

#include <algorithm>
#include <map>

volatile CAN_Configuration can_config = {
    .battery = CAN_NATIVE,
    .inverter = CAN_NATIVE,
    .battery_double = CAN_ADDON_MCP2515,
    .battery_triple = CAN_ADDON_MCP2515,
    .charger = CAN_NATIVE,
    .shunt = CAN_NATIVE,
};

static void dispatch_frame(CAN_frame* rx_frame, class CanDevice* dev);

// The native TWAI controller on the ESP32 itself.
class NativeTwaiDevice : public CanDevice {
 public:
  struct Config {
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    gpio_num_t se_pin;  // GPIO_NUM_NC on boards without a silent-enable pin
  };

  explicit NativeTwaiDevice(const Config& config) : config_(config) {
    name = "Native CAN";
    log_interface = CAN_NATIVE;
    buffer_full_event = EVENT_CAN_NATIVE_BUFFER_FULL;
    bus_error_event = EVENT_CAN_NATIVE_BUS_ERROR;
  }

  bool init(CAN_Speed speed) override {
    if (config_.se_pin != GPIO_NUM_NC) {
      pinMode(config_.se_pin, OUTPUT);
      digitalWrite(config_.se_pin, LOW);
    }

    tx_pin_ = config_.tx_pin;
    rx_pin_ = config_.rx_pin;

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
    for (uint8_t i = 0; i < frame.len; ++i) {
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
        rx_frame.FD = false;
        for (uint8_t i = 0; i < frame.len && i < 8; ++i) {
          rx_frame.data.u8[i] = frame.data[i];
        }

        //message incoming, pass it on to the handler
        dispatch_frame(&rx_frame, this);
      }
    }

    // The mirrored bit values must match the controller library's, or the
    // host-tested decision would not describe what runs on the target.
    static_assert(kTwaiBusOffStatusBit == TWAI_BUS_OFF_ST, "TWAI bus-off status bit moved");
    static_assert(kTwaiErrorStatusBit == TWAI_ERR_ST, "TWAI error status bit moved");

    const CanBusStatusAction action = evaluate_twai_status(ACAN_ESP32::can.statusRegister());
    if (action.reinitialise) {
      change_speed(speed_);  // Bus off: bring the controller back up
    }
    if (action.flag_bus_error) {
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
  Config config_;
  gpio_num_t tx_pin_;
  gpio_num_t rx_pin_;
};

// MCP2515 add-on controller (classic CAN over SPI).
class Mcp2515Device : public CanDevice {
 public:
  struct Config {
    gpio_num_t cs_pin;
    gpio_num_t int_pin;
    gpio_num_t rst_pin;  // GPIO_NUM_NC when the controller has no reset line
    SPIClass* spi;       // owned by the bus, not by this device
    uint32_t freq;       // 0 = autodetect the crystal
  };

  explicit Mcp2515Device(const Config& config) : config_(config) {
    name = "MCP2515";
    log_interface = CAN_ADDON_MCP2515;
    buffer_full_event = EVENT_CANMCP2515_BUFFER_FULL;
    bus_error_event = EVENT_CANMCP2515_BUS_ERROR;
  }

  bool init(CAN_Speed speed) override {
    logging.println("Dual CAN Bus (ESP32+MCP2515) selected");

    const gpio_num_t rst_pin = config_.rst_pin;
    if (rst_pin != GPIO_NUM_NC) {
      pinMode(rst_pin, OUTPUT);
      digitalWrite(rst_pin, HIGH);
      delay(100);
      digitalWrite(rst_pin, LOW);
      delay(100);
      digitalWrite(rst_pin, HIGH);
      delay(100);
    }

    can_ = new MCP2515_Lite(*config_.spi, config_.cs_pin, config_.int_pin);

    quartz_frequency_ = config_.freq;
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
  Config config_;
  MCP2515_Lite* can_ = nullptr;
  uint32_t quartz_frequency_ = 0;
};

class Mcp2518Device;

// The ACAN2517FD begin() callback must be a captureless function pointer, so
// the two FD instances are reachable through this array for their ISR
// trampolines.
static Mcp2518Device* fd_instances[MAX_CAN_FD_DEVICES] = {};  // value-initialized: all null

// Logical interface -> physical device. Entries may repeat: on boards where
// CANFD_NATIVE and CANFD_ADDON_MCP2518 are the same chip, both slots point at
// the same device, and dispatch_frame delivers each physical frame to the
// receivers of every logical interface mapped to it.
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
  };

  // Everything that differs between the FD chips, one row per instance,
  // indexed by Config::index.
  struct Identity {
    const char* name;
    CAN_Interface log_interface;
    // Init failure keeps a per-instance event: its payload is the controller's
    // error code, so the instance cannot ride there as it does for the
    // health events.
    EVENTS_ENUM_TYPE init_fail_event;
  };

  // The channel number is a parameter rather than part of the text, so these are one
  // message each instead of one per controller: another channel costs no strings, and
  // each message is a single unit to translate rather than several near-identical ones.
  static constexpr const char* SELECTED_LOG_FORMAT = "CAN FD %d add-on (ESP32+MCP2517) selected";
  static constexpr const char* CONFIG_ERROR_FORMAT = "CAN-FD %d Configuration error 0x";

  static constexpr Identity identity[MAX_CAN_FD_DEVICES] = {
      {
          .name = "CAN-FD",
          .log_interface = CANFD_ADDON_MCP2518,
          .init_fail_event = EVENT_CANMCP2518FD_INIT_FAILURE,
      },
      {
          .name = "CAN-FD 2",
          .log_interface = CANFD_ADDON_MCP2518_2,
          .init_fail_event = EVENT_CANMCP2518FD_2_INIT_FAILURE,
      },
  };
  static_assert(MAX_CAN_FD_DEVICES == 2, "add the new FD instance's identity row and its ISR trampoline");
  // Init failure stays per instance, so those must still differ.
  static_assert(identity[0].init_fail_event != identity[1].init_fail_event,
                "each FD instance needs its own init-failure event id");

  // 1-based, as the messages and the settings UI count channels.
  int channel_number() const { return cfg_.index + 1; }

  explicit Mcp2518Device(const Config& config) : cfg_(config) {
    name = identity[cfg_.index].name;
    log_interface = identity[cfg_.index].log_interface;
    init_fail_event_ = identity[cfg_.index].init_fail_event;
    // One pair for every FD controller: which of them is unhealthy rides the
    // device bitmask in the event payload, so adding a channel costs no events.
    buffer_full_event = EVENT_CANFD_BUFFER_FULL;
    bus_error_event = EVENT_CANFD_BUS_ERROR;
    fd_instances[cfg_.index] = this;
  }

  bool init(CAN_Speed speed) override {
    can_ = new ACAN2517FD(cfg_.cs_pin, *cfg_.spi, cfg_.int_pin != GPIO_NUM_NC ? cfg_.int_pin : 255,
                          cfg_.int0_pin != GPIO_NUM_NC ? cfg_.int0_pin : 255,
                          cfg_.int1_pin != GPIO_NUM_NC ? cfg_.int1_pin : 255);

    logging.printf(SELECTED_LOG_FORMAT, channel_number());

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
    // One captureless trampoline per instance: ACAN2517FD::begin takes a
    // plain void(*)(), so these are per-index code, not data.
    static_assert(MAX_CAN_FD_DEVICES == 2, "add the new FD instance's ISR trampoline");
    const uint32_t errorCode =
        can_->begin(*settings_, cfg_.index == 0 ? +[] { fd_instances[0]->run_isr(); }
                                                : +[] { fd_instances[1]->run_isr(); });
    can_->poll();
    if (errorCode != 0) {
      logging.printf(CONFIG_ERROR_FORMAT, channel_number());
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

bool init_CAN() {
  // Native CAN (onboard the ESP32)

  if (can_receiver_registered(CAN_NATIVE)) {
    const NativeTwaiDevice::Config native_config = {
        .tx_pin = esp32hal->CAN_TX_PIN(),
        .rx_pin = esp32hal->CAN_RX_PIN(),
        .se_pin = esp32hal->CAN_SE_PIN(),
    };

    if (native_config.se_pin != GPIO_NUM_NC && !esp32hal->alloc_pins("CAN", native_config.se_pin)) {
      return false;
    }
    if (!esp32hal->alloc_pins("CAN", native_config.tx_pin, native_config.rx_pin)) {
      return false;
    }

    NativeTwaiDevice* native_dev = new NativeTwaiDevice(native_config);
    map_interface_to_device(CAN_NATIVE, native_dev);
    if (!register_device(native_dev)) {
      return false;
    }
    if (!native_dev->init(can_receiver_speed(CAN_NATIVE, CAN_Speed::CAN_SPEED_500KBPS))) {
      return false;
    }
  }

  // Add-on CAN interface (via MCP2515)

  if (can_receiver_registered(CAN_ADDON_MCP2515)) {
    /* SCK/MISO/MOSI belong to the bus, not to the device hanging off it, so
       they are claimed once here and the device never sees them. The only
       lines it owns are its chip select and its interrupt. */
    const auto sck_pin = esp32hal->MCP2515_SCK();
    const auto miso_pin = esp32hal->MCP2515_MISO();
    const auto mosi_pin = esp32hal->MCP2515_MOSI();

    if (!esp32hal->alloc_pins("CAN SPI bus", sck_pin, miso_pin, mosi_pin)) {
      return false;
    }

    SPIClass* spi2515 = new SPIClass(esp32hal->MCP2515_BUS());
    spi2515->begin(sck_pin, miso_pin, mosi_pin);

    const Mcp2515Device::Config mcp2515_config = {
        .cs_pin = esp32hal->MCP2515_CS(),
        .int_pin = esp32hal->MCP2515_INT(),
        .rst_pin = esp32hal->MCP2515_RST(),
        .spi = spi2515,
        .freq = esp32hal->MCP2515_FREQ(),
    };

    if (!esp32hal->alloc_pins("CAN", mcp2515_config.cs_pin, mcp2515_config.int_pin)) {
      return false;
    }

    Mcp2515Device* mcp2515_dev = new Mcp2515Device(mcp2515_config);
    map_interface_to_device(CAN_ADDON_MCP2515, mcp2515_dev);
    if (!register_device(mcp2515_dev)) {
      return false;
    }
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

    auto speed = fdNativeWanted ? can_receiver_speed(CANFD_NATIVE, CAN_Speed::CAN_SPEED_500KBPS)
                                : can_receiver_speed(CANFD_ADDON_MCP2518, CAN_Speed::CAN_SPEED_500KBPS);

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

    static_assert(MAX_CAN_FD_DEVICES == 2, "a third FD controller needs its own construction block here");
    Mcp2518Device* fd_dev = new Mcp2518Device({
        .index = 0,
        .cs_pin = cs_pin,
        .int_pin = int_pin,
        .int0_pin = int0_pin,
        .int1_pin = int1_pin,
        .spi = spi2517,
        .freq = esp32hal->MCP2517_FREQ(),
        .set_clko = true,
        .clko_div = static_cast<uint8_t>(esp32hal->MCP2517_CLKODIV()),
    });
    // One physical chip serves both logical FD names on boards that expose both.
    map_interface_to_device(CANFD_NATIVE, fd_dev);
    map_interface_to_device(CANFD_ADDON_MCP2518, fd_dev);
    if (!register_device(fd_dev)) {
      return false;
    }
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

    Mcp2518Device* fd_dev_2 = new Mcp2518Device({
        .index = 1,
        .cs_pin = cs_pin,
        .int_pin = int_pin,
        .int0_pin = GPIO_NUM_NC,
        .int1_pin = GPIO_NUM_NC,
        .spi = spi2517_2,
        .freq = esp32hal->MCP2517_FREQ2(),
        .set_clko = false,
        .clko_div = 0,
    });
    map_interface_to_device(CANFD_ADDON_MCP2518_2, fd_dev_2);
    if (!register_device(fd_dev_2)) {
      return false;
    }
    if (!fd_dev_2->init(can_receiver_speed(CANFD_ADDON_MCP2518_2, CAN_Speed::CAN_SPEED_500KBPS))) {
      return false;
    }
  }

  return true;
}

// Receive functions
void receive_can() {
  for (CanDevice* dev : unique_can_devices()) {
    if (dev->initialized) {
      dev->poll_receive();
    }
  }
}

// Support functions
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

  for (int i = 0; i < NO_CAN_INTERFACE; ++i) {
    if (can_device_index_for(static_cast<CAN_Interface>(i)) != dev->device_index) {
      continue;
    }
    deliver_to_receivers(rx_frame, static_cast<CAN_Interface>(i));
  }
}

void stop_can() {
  for (CanDevice* dev : unique_can_devices()) {
    if (dev->initialized) {
      dev->stop();
    }
  }
}

void restart_can() {
  for (CanDevice* dev : unique_can_devices()) {
    if (dev->initialized) {
      dev->restart();
    }
  }
}

// Change the speed of the given CAN interface. Returns true if successful.
// Devices without runtime speed change (the FD controllers) inherit the base
// class refusal.
bool change_can_speed(CAN_Interface interface, CAN_Speed speed) {
  CanDevice* dev = device_for_interface(interface);
  if (dev == nullptr || !dev->initialized) {
    return false;
  }
  return dev->change_speed(speed);
}
