## MCP2517FD and MCP2518FD CAN Controller Library for Arduino (in CAN FD mode)


### Compatibility with the other ACAN libraries

This library is fully compatible with the Teensy 3.x ACAN library https://github.com/pierremolinaro/acan, ACAN2515 library https://github.com/pierremolinaro/acan2515, and the ACAN2517 library https://github.com/pierremolinaro/acan2517, it uses a very similar API.

### ACAN2517FD library description
ACAN2517FD is a driver for the MCP2517FD and MCP2518FD CAN Controllers, in CAN FD mode. It runs on any Arduino compatible board.

> The ACAN2517 library handles the MCP2517FD and MCP2518FD CAN Controller, in CAN 2.0B mode.


The library supports the 4MHz, 20 MHz and 40 MHz oscillator clock.

The driver supports many bit rates: the CAN bit timing calculator finds settings for standard 62.5 kbit/s, 125 kbit/s, 250 kbit/s, 500 kbit/s, 1 Mbit/s, but also for an exotic bit rate as 727 kbit/s. If the desired bit rate cannot be achieved, the `begin` method does not configure the hardware and returns an error code.

> Driver API is fully described by the PDF file in the `extras` directory.

### Demo Sketch

> The demo sketch is in the `examples/LoopBackDemo` directory.

Configuration is a four-step operation.

1. Instanciation of the `settings` object : the constructor has one parameter: the wished CAN bit rate. The `settings` is fully initialized.
2. You can override default settings. Here, we set the `mRequestedMode` property to true, enabling to run demo code without any additional hardware (no CAN transceiver needed). We can also for example change the receive buffer size by setting the `mReceiveBufferSize` property.
3. Calling the `begin` method configures the driver and starts CAN bus participation. Any message can be sent, any frame on the bus is received. No default filter to provide.
4. You check the `errorCode` value to detect configuration error(s).

```cpp
static const byte MCP2517_CS  = 20 ; // CS input of MCP2517FD, adapt to your design
static const byte MCP2517_INT = 37 ; // INT output of MCP2517FD, adapt to your design

ACAN2517FD can (MCP2517_CS, SPI, MCP2517_INT) ; // You can use SPI2, SPI3, if provided by your microcontroller

void setup () {
  Serial.begin (9600) ;
  while (!Serial) {}
  Serial.println ("Hello") ;
 // Arbitration bit rate: 125 kbit/s, data bit rate: 500 kbit/s
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL,
                               125 * 1000, ACAN2517FDSettings::DATA_BITRATE_x4) ;
  settings.mRequestedMode = ACAN2517FDSettings::InternalLoopBack ; // Select loopback mode
  const uint32_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
  if (0 == errorCode) {
    Serial.println ("Can ok") ;
  }else{
    Serial.print ("Error Can: 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}
```

Now, an example of the `loop` function. As we have selected loop back mode, every sent frame is received.

```cpp
static unsigned gSendDate = 0 ;
static unsigned gSentCount = 0 ;
static unsigned gReceivedCount = 0 ;

void loop () {
  CANFDMessage message ;
  if (gSendDate < millis ()) {
    message.id = 0x542 ;
    const bool ok = can.tryToSend (message) ;
    if (ok) {
      gSendDate += 2000 ;
      gSentCount += 1 ;
      Serial.print ("Sent: ") ;
      Serial.println (gSentCount) ;
    }
  }
  if (can.receive (message)) {
    gReceivedCount += 1 ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedCount) ;
  }
}
```
`CANFDMessage` is the class that defines a CAN FD message. The `message` object is fully initialized by the default constructor. Here, we set the `id` to `0x542` for sending a standard data frame, without data, with this identifier.

The `can.tryToSend` tries to send the message. It returns `true` if the message has been sucessfully added to the driver transmit buffer.

The `gSendDate` variable handles sending a CAN message every 2000 ms.

`can.receive` returns `true` if a message has been received, and assigned to the `message`argument.

### Use of Optional Reception Filtering

The MCP2517 CAN Controller implements 32 acceptance masks and 32 acceptance filters. The driver API enables you to fully manage these registers.

For example (`LoopBackDemoTeensy3xWithFilters` sketch):

```cpp
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000) ;
  settings.mRequestedMode = ACAN2517FDSettings::InternalLoopBack ; // Select loopback mode
  ACAN2517Filters filters ;
// Filter #0: receive standard frame with identifier 0x123
  filters.appendFrameFilter (kStandard, 0x123, receiveFromFilter0) ;
// Filter #1: receive extended frame with identifier 0x12345678
  filters.appendFrameFilter (kExtended, 0x12345678, receiveFromFilter1) ; 
// Filter #2: receive standard frame with identifier 0x3n4
  filters.appendFilter (kStandard, 0x70F, 0x304, receiveFromFilter2) ;
  const uint32_t errorCode = can.begin (settings, [] { can.isr () ; }, filters) ;
```

These settings enable the acceptance of standard frames whose identifier is 0x123, extended frames whose identifier is 0x12345678, and data frames whose identifier is 0x304, 0x314, ..., 0x3E4, 0x3F4.

The `receiveFromFilter0`, `receiveFromFilter1`, `receiveFromFilter2` functions are call back functions, handled by the `can.dispatchReceivedMessage` function:


```cpp
void loop () {
  can.dispatchReceivedMessage () ; // Do not use can.receive any more
  ...
}
```
