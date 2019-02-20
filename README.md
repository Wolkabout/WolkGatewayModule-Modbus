# WolkGateway Module Modbus
WolkGateway module for connecting Modbus devices to WolkAbout IoT Platform by communicating with [WolkGateway](https://github.com/Wolkabout/WolkGateway).

Supported protocol(s):
* JSON_PROTOCOL

Installing from source
----------------------

This repository must be cloned from the command line using:
```sh
git clone --recurse-submodules https://github.com/Wolkabout/WolkGatewayModule-Modbus.git
```

Prerequisite
------------
Following tools/libraries are required in order to build WolkGateway Module Modbus

* cmake - version 3.5 or later
* autotools
* automake
* autoconf
* m4
* zlib1g-dev
* libtool
* python
* python pip
* conan

Former can be installed on Debian based system from terminal by invoking:
```sh
sudo apt-get install automake autotools-dev autoconf m4 zlib1g-dev cmake libtool python python-pip && python -m pip install conan
```
Afterwards dependencies are built, and Makefile build system is generated by invoking:
```sh
./configure
```

Generated build system is located inside `out` directory

WolkGateway Module Modbus library, is built from `out` directory by 
invoking `make` in terminal

Module Configuration
--------------------
Module configuration consists of 3 configurations files

* Device configuration
* Modbus configuration
* Modbus register mapping

These files are located in `out` directory, and are passed to Modbus module executable in following manner:
```sh
./modbusModule deviceConfiguration.json modbusConfiguration.json modbusRegisterMapping.json
```
Below are sections describing each of these configuration files.

Device configuration
--------------------
Device configuration file contains settings that relate to communication with WolkGateway Module.

```javascript
{
    "name": "WolkGateway Modbus Bridge",   // Device name
    "key": "modbus_bridge",                // Device key
    "protocol": "JsonProtocol",            // Protocol used on WolkGateway
    "localMqttUri": "tcp://localhost:1883" // WolkGateway Bus
}

```

Modbus configuration
--------------------
Modbus configuration file contains setting that relate to Modbus communication protocol settings,
such as slave IP address, port, response timeout etc.

Here one can select between two modes of modbus communication:

* TCP/IP modbus - supports only one device, use additional WolkGateway-ModbusModules for more devices.

```javascript
{
    // TCP/IP Configuration
    "ip": "192.168.100.1",          // Slave IP address
    "port": 502,                    // Slave port
```

* Serial RTU - supports communication with multiple slaves

```javascript
    // SERIAL/RTU Configuration
    "serialPort": "/dev/ttyS",      // Serial port
    "baudRate": 115200,             // Baud rate
    "dataBits": 8,                  // Number of data bits
    "stopBits": 1,                  // Number of stop bits
    "bitParity": "NONE",            // Bit parity - "NONE" or "EVEN" or "ODD"
    "slaveAddress" : 1,             // Initial slave address
```

Select preferred connection type and register read parameters

```javascript

    // Modbus connection type configuration
    "connectionType": "SERIAL/RTU", // Connection type - "SERIAL/RTU" or "TCP/IP"

    // Modbus register read parameters
    "responseTimeoutMs": 200,       // Register read/write timeout
    "registerReadPeriodMs": 500     // Register pool period
}

```

Modbus register mapping
-----------------------
Modbus register mapping file contains settings that map modbus registers to WolkAbout IoT Platform sensors/actuators.

* Actuators (read & write):
    - `COIL`
    - `HOLDING_REGISTER_ACTUATOR`
* Sensors (read only):
    - `HOLDING_REGISTER_SENSOR`
    - `INPUT_REGISTER`
    - `INPUT_CONTACT`

```javascript
{
   "modbusRegisterMapping":[
      {
         "name":"mappingName",              // Register name
         "reference": "mappingReference",   // Unique reference used to differ register on WolkAbout IoT Platform

         "minimum": -32768,                 // Minimum value that can be held in register. Required for visualization on WolkAbout IoT Platform
         "maximum": 32767,                  // Maximum value that can be held in register. Required for visualization on WolkAbout IoT Platform

         "address": 0,                      // Register address
         "registerType": "INPUT_REGISTER",  // Register type - "INPUT_REGISTER" or "HOLDING_REGISTER_ACTUATOR" or "HOLDING_REGISTER_SENSOR" or "INPUT_CONTACT" or "COIL"
         "dataType": "INT16",               // Data type stored in register - "INT16" or "UINT16" or "REAL32" for "INPUT_REGISTER"/"HOLDING_REGISTER_ACTUATOR"/"HOLDING_REGISTER_SENSOR" register type, and "BOOL" for "COIL"/"INPUT_CONTACT"
         "slaveAddress": 1                  // Slave address where the register is located - Ignored for TCP/IP
      },
      {
         "name":"mappingName2",
         "reference": "mappingReference2",

         "minimum": -4000,
         "maximum": 4000,

         "address": 1,
         "registerType": "HOLDING_REGISTER_ACTUATOR",
         "dataType": "REAL32",
         "slaveAddress" : 1
      },
      {
         "name":"mappingName3",
         "reference": "mappingReference3",

         "address": 2,
         "registerType": "INPUT_CONTACT",
         "dataType": "BOOL",
         "slaveAddress" : 2
      },
      {
         "name":"mappingName4",
         "reference": "mappingReference4",

         "address": 3,
         "registerType": "COIL",
         "dataType": "BOOL",
         "slaveAddress" : 2
      }
   ]
}
```
