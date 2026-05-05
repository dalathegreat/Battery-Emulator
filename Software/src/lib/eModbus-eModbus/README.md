
<img src=https://github.com/eModbus/eModbus/blob/master/eModbusLogo.png width="33%" alt="eModbus">

**Read the docs at http://emodbus.github.io!**

![eModbus](https://github.com/eModbus/eModbus/workflows/Building/badge.svg)

This is a library to provide Modbus client (formerly known as master), server (formerly slave) and bridge/gateway functionalities for Modbus RTU, ASCII and TCP protocols.

For Modbus protocol specifications, please refer to the [Modbus.org site](https://www.modbus.org/specs.php)!

Modbus communication is done in separate tasks, so Modbus requests and responses are non-blocking. Callbacks are provided to prepare or receive the responses asynchronously.

Key features:
- for use in the Arduino framework
- designed for ESP32, various interfaces supported; async versions run also on ESP8266
- non blocking / asynchronous API
- server, client and bridge modes
- TCP (Ethernet, WiFi and Async), ASCII and RTU interfaces
- all common and user-defined Modbus standard function codes 

This has been developed by enthusiasts. While we do our utmost best to make robust software, do not expect any bullet-proof, industry deployable, guaranteed software. [**See the license**](https://github.com/eModbus/eModbus/blob/master/license.md) to learn about liabilities etc.

We do welcome any ideas, suggestions, bug reports or questions. Please use the "[Issues](https://github.com/eModbus/eModbus/issues)" tab to report bugs and request new features and visit the "[Discussions](https://github.com/eModbus/eModbus/discussions)" tab for all else.

Have fun!
