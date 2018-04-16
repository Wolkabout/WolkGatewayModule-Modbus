# WolkGateway Modbus Module
WolkGateway module for connecting Modbus devices to WolkAbout IoT Platform.

Supported protocol(s):
* JSON_PROTOCOL

Prerequisite
------------
Following tools/libraries are required in order to build WolkAbout C++ connector

* cmake - version 3.5 or later
* autotools
* automake
* autoconf
* m4
* zlib1g-dev

Former can be installed on Debian based system from terminal by invoking

`apt-get install automake autotools-dev autoconf m4 zlib1g-dev cmake`

Afterwards dependencies are built, and Makefile build system is generated by invoking
`./configure`

Generated build system is located inside 'out' directory

WolkGateway Modbus Module library, is built from 'out' directory by invoking `make` in terminal

Module Configuration
--------------------
Module configuration consists of 3 configurations files

* Device configuration
* Modbus configuration
* Modbus register mapping

These files are located in `out` directory, and are passed to Modbus module executable in following manner
`./modbusModule deviceConfiguration.json modbusConfiguration.json modbusRegisterMapping.json`

Below are sections describing each of these configuration files.

Device configuration
--------------------
Device configuration file contains settings that relate to communication with WolkGateway Module.

```
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
such as slace IP address, port, response timeout etc.

Here one can select between Serial RTU or TCP/IP modbus.

```
{
    // TCP/IP Configuration
    "ip": "192.168.100.1",          // Slave IP address
    "port": 502,                    // Slave port

    // SERIAL/RTU Configuration
    "serialPort": "/dev/ttyS",      // Serial port
    "baudRate": 115200,             // Baud rate
    "dataBits": 8,                  // Number of data bits
    "stopBits": 1,                  // Number of stop bits
    "bitParity": "NONE",            // Bit parity - "NONE" or "EVEN" or "ODD"

    // Modbus connection type configuration
    "connectionType": "SERIAL/RTU", // Connection type - "SERIAL/RTU" or "TCP/IP"

    // Modbus register read parameters
    "responseTimeoutMs": 200,       // Register read/write timeout
    "registerReadPeriodMs": 500     // Register pool period
}

```

Modbus register mapping
-----------------------
Modbus register mapping file is heart and soul of WolkGateway Module, this file contains settings that map modbus registers to WolkAbout IoT Platform sensors/actuators.

```
{
   "modbusRegisterMapping":[
      {
         "name":"mappingName",              // Register name
         "reference": "mappingReference",   // Unique reference used to differ register on WolkAbout IoT Platform
         "unit": "mappingReferenceUnit",    // Unit of type stored in register

         "minimum": -32768,                 // Minimum value that can be held in register. Required for visalization on WolkAbout IoT Platform
         "maximum": 32767,                  // Maximum value that can be held in register. Required for visalization on WolkAbout IoT Platform

         "address": 0,                      // Register address
         "registerType": "INPUT_REGISTER",  // Register type - "INPUT_REGISTER" or "HOLDING_REGISTER" or "INPUT_BIT" or "COIL"
         "dataType": "INT16"                // Data type stored in register - "INT16" or "UINT16" or "REAL32" for "INPUT_REGISTER"/"HOLDING_REGISTER"/"INPUT_BIT" register type, and "BOOL" for "COIL"
      },
      {
         "name":"mappingName2",
         "reference": "mappingReference2",
         "unit": "mappingReferenceUniti2",

         "minimum": -4000,
         "maximum": 4000,

         "address": 1,
         "registerType": "HOLDING_REGISTER",
         "dataType": "REAL32"
      },
      {
         "name":"mappingName3",
         "reference": "mappingReference3",
         "unit": "mappingReferenceUnit3",

         "address": 2,
         "registerType": "INPUT_BIT",
         "dataType": "BOOL"
      },
      {
         "name":"mappingName4",
         "reference": "mappingReference4",
         "unit": "mappingReferenceUnit4",

         "address": 3,
         "registerType": "COIL",
         "dataType": "BOOL"
      }
   ]
}


```
