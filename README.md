# WolkGatewayModule C++
WolkAbout C++11 Connector library for connecting devices to WolkAbout Gateway.

Supported protocol(s):
* JSON_PROTOCOL

Prerequisite
------
Following tools/libraries are required in order to build WolkAbout C++ connector

* cmake - version 3.5 or later
* autotools
* autoconf
* m4
* zlib1g-dev

Former can be installed on Debian based system from terminal by invoking

`apt-get install autotools-dev autoconf m4 zlib1g-dev cmake`

Afterwards dependencies are built, and Makefile build system is generated by invoking
`./configure`

Generated build system is located inside 'out' directory


WolkAbout C++ Connector library, and example are built from 'out' directory by invoking
`make` in terminal

Example Usage
-------------
**Establishing connection with WolkAbout IoT platform:**
```cpp
wolkabout::Device device("DEVICE_KEY", "DEVICE_PASSWORD", {"ACTUATOR_REFERENCE_ONE", "ACTUATOR_REFERENCE_TWO"});

std::unique_ptr<wolkabout::Wolk> wolk =
  wolkabout::Wolk::newBuilder()
    .actuationHandler([](const std::string& deviceKey, const std::string& reference, const std::string& value) -> void {
        // TODO Invoke your code which activates actuator of specified device.

        std::cout << "Actuation request received - Key: " << deviceKey << " Reference: " << reference << " value: " << value << std::endl;
    })
    .actuatorStatusProvider([](const std::string& deviceKey, const std::string& reference) -> wolkabout::ActuatorStatus {
        // TODO Invoke code which reads the state of the actuator of specified device.

        if (deviceKey == "DEVICE_KEY" && reference == "SWITCH_ACTUATOR_REF") {
            return wolkabout::ActuatorStatus("true", wolkabout::ActuatorStatus::State::READY);

        return wolkabout::ActuatorStatus("", wolkabout::ActuatorStatus::State::READY);
    })
	.deviceStatusProvider([](const std::string& deviceKey) -> wolkabout::DeviceStatus {
        // TODO Invoke code which reads the status of specified device.

        if (deviceKey == "DEVICE_KEY")
        {
            return wolkabout::DeviceStatus::CONNECTED;
        }

        return wolkabout::DeviceStatus::OFFLINE;
    })
    .build();

    wolk->connect();
```

**Creating devices:**
```cpp
wolkabout::SensorManifest temperatureSensor{"Temperature",										// name
                                            "TEMPERATURE_REF",									// reference
                                            "Temperature sensor",								// description
                                            "℃",												// unit
                                            "TEMPERATURE",										// reading type
                                            wolkabout::SensorManifest::DataType::NUMERIC,		// data type
                                            1,													// percision
                                            -273.15,											// min value
                                            100000000};											// max value

wolkabout::SensorManifest pressureSensor{"Pressure",
                                         "PRESSURE_REF",
                                         "Pressure sensor",
                                         "mb",
                                         "PRESSURE",
                                         wolkabout::SensorManifest::DataType::NUMERIC,
                                         1,
                                         0,
                                         1100};

wolkabout::ActuatorManifest switchActuator{"Switch",
                                           "SWITCH_ACTUATOR_REF",
                                           "Switch actuator",
                                           "",
                                           "SW",
                                           wolkabout::ActuatorManifest::DataType::BOOLEAN,
                                           1,
                                           0,
                                           1};

wolkabout::AlarmManifest highHumidityAlarm{"High Humidity",										// name
                                           wolkabout::AlarmManifest::AlarmSeverity::ALERT,		// severity
                                           "HUMIDITY_ALARM_REF",								// reference
                                           "High Humidity",										// message
                                           "High Humidity alarm"};								// description

wolkabout::DeviceManifest deviceManifest1{"DEVICE_MANIFEST_NAME",
                                          "DEVICE_MANIFEST_DESCRIPTION",
                                          "JsonProtocol",
                                          "",
                                          {},
                                          {temperatureSensor, pressureSensor},
                                          {},
                                          {switchActuator}};

wolkabout::Device device1{"DEVICE_NAME", "DEVICE_KEY", deviceManifest};

wolk->addDevice(device);
```

**Publishing sensor readings:**
```cpp
wolk->addSensorReading("DEVICE_KEY", "TEMPERATURE_REF", 23.4);
wolk->addSensorReading("DEVICE_KEY", "PRESSURE_REF", 1080);
```

**Publishing actuator statuses:**
```cpp
wolk->publishActuatorStatus("DEVICE_KEY", "SWITCH_ACTUATOR_REF");
```
This will invoke the ActuationStatusProvider to read the actuator status,
and publish actuator status.

**Publishing alarms:**
```cpp
wolk->addAlarm("DEVICE_KEY", "HUMIDITY_ALARM_REF", "ALARM_VALUE");
```

**Data publish strategy:**

Sensor readings, and alarms are pushed to WolkAbout IoT platform on demand by calling
```cpp
wolk->publish();

wolk->publish("DEVICE_KEY");
```
Publishing without providing device key publishes all available data,
whereas publishing with device key only data for the specified device is published


Actuator statuses are published automatically by calling:

```cpp
wolk->publishActuatorStatus("DEVICE_KEY", "ACTUATOR_REFERENCE_ONE");
```

**Disconnecting from the platform:**
```cpp
wolk->disconnect();
```

**Data persistence:**

WolkAbout C++ Connector provides mechanism for persisting data in situations where readings can not be sent to WolkAbout Gateway.

Persisted readings are sent to WolkAbout Gateway once connection is established.
Data persistence mechanism used **by default** stores data in-memory.

In cases when provided in-memory persistence is suboptimal, one can use custom persistence by implementing wolkabout::Persistence interface,
and forwarding it to builder in following manner:

```cpp
std::unique_ptr<wolkabout::Wolk> wolk =
  wolkabout::Wolk::newBuilder()
    .actuationHandler([](const std::string& deviceKey, const std::string& reference, const std::string& value) -> void {
        // TODO Invoke your code which activates actuator of specified device.

        std::cout << "Actuation request received - Key: " << deviceKey << " Reference: " << reference << " value: " << value << std::endl;
    })
    .actuatorStatusProvider([](const std::string& deviceKey, const std::string& reference) -> wolkabout::ActuatorStatus {
        // TODO Invoke code which reads the state of the actuator of specified device.

        if (deviceKey == "DEVICE_KEY" && reference == "SWITCH_ACTUATOR_REF") {
            return wolkabout::ActuatorStatus("true", wolkabout::ActuatorStatus::State::READY);

        return wolkabout::ActuatorStatus("", wolkabout::ActuatorStatus::State::READY);
    })
	.deviceStatusProvider([](const std::string& deviceKey) -> wolkabout::DeviceStatus {
        // TODO Invoke code which reads the status of specified device.

        if (deviceKey == "DEVICE_KEY")
        {
            return wolkabout::DeviceStatus::CONNECTED;
        }

        return wolkabout::DeviceStatus::OFFLINE;
    })
    .withPersistence(std::make_shared<CustomPersistenceImpl>()) // Enable data persistance via custom persistence mechanism
    .build();

    wolk->connect();
```

For more info on persistence mechanism see wolkabout::Persistence and wolkabout::InMemoryPersistence classes