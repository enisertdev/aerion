# AERION

**AERION** is an experimental, open-source RC fixed-wing UAV platform built around an **ESP32 flight controller**, **LoRa telemetry/control**, and a custom **C#/.NET Ground Control Station (GCS)**.

The project is designed to combine embedded systems, wireless communication, flight control, telemetry, and ground-control software into a single modular UAV platform.

> ⚠️ **Status:** Active development / Experimental
> AERION is currently being developed and tested on the ground. It is not intended for autonomous flight yet.

---

## ✈️ What is AERION?

AERION is a custom-built fixed-wing UAV system consisting of two main software components:

* **ESP32 Firmware** — Flight-side software responsible for receiving commands, controlling the ESC/servos, reading sensors, handling failsafe logic, and sending telemetry.
* **Ground Control Station** — A C#/.NET application used to control the aircraft, read telemetry, and communicate with the flight controller through a LoRa link.

The goal is to eventually build a complete flight-control system without relying on a commercial flight controller.

---

## 🚀 Current Features

### Flight Controller

* ESP32-based flight controller
* LoRa command reception
* LoRa telemetry transmission
* GPS data processing
* IMU data processing
* QMC5883P magnetometer support
* ESC PWM control
* Servo control architecture
* Configurable flight modes
* Automatic failsafe
* Wi-Fi debugging
* Modular firmware structure

### Communication

* Long-range LoRa communication
* Custom binary packet protocol
* Packet checksum validation
* Command packet validation
* Telemetry packet validation
* Connection timeout detection
* Automatic throttle failsafe

### Ground Control Station

* C# / .NET
* Custom joystick support
* SharpDX DirectInput
* Real-time joystick control
* Joystick hot-plug detection
* Automatic joystick reinitialization
* Serial communication with LoRa module
* Real-time telemetry display
* SignalR-based UI communication
* IMU calibration command
* Serial connection watchdog

### Safety

AERION currently implements a basic communication failsafe.

If valid control packets are no longer received within the configured timeout:

```text
Throttle → 0
Pitch    → Neutral
Roll     → Neutral
Yaw      → Neutral
```

The flight controller then enters failsafe mode.

---

# 🧩 System Architecture

```text
                    ┌─────────────────────┐
                    │    Ground Control   │
                    │       Station       │
                    │      C# / .NET      │
                    └──────────┬──────────┘
                               │
                         USB / Serial
                               │
                    ┌──────────▼──────────┐
                    │     LoRa Module     │
                    └──────────┬──────────┘
                               │
                            LoRa RF
                               │
                    ┌──────────▼──────────┐
                    │     LoRa Module     │
                    └──────────┬──────────┘
                               │ UART
                               │
                    ┌──────────▼──────────┐
                    │        ESP32        │
                    │   Flight Controller │
                    └─────┬────┬────┬─────┘
                          │    │    │
                     ┌────┘    │    └─────┐
                     │         │          │
                   GPS         IMU       ESC
                                         │
                                      Motor
                                         
                              ┌──────────┐
                              │ Servos   │
                              └──────────┘
```

---

# 🔧 Hardware

## Flight Controller

| Component    | Model                |
| ------------ | -------------------- |
| MCU          | ESP32 DevKit         |
| GPS          | TBS M10Q             |
| IMU          | MPU9250              |
| Magnetometer | QMC5883P             |
| Barometer    | BMP280               |
| LoRa         | Ebyte E22900T30D     |
| ESC          | 40A                  |
| Motor        | A2212 1400KV         |
| Propeller    | 1045                 |
| Battery      | 3S 1300mAh LiPo      |
| PDB/BEC      | Matek Mini Hub       |
| Airframe     | ~400–500g fixed-wing |

> Some components listed above are currently planned or partially integrated.

---

# 🔌 Current Pin Configuration

| Function | ESP32 Pin |
| -------- | --------: |
| GPS RX   |   GPIO 16 |
| GPS TX   |   GPIO 17 |
| LoRa RX  |   GPIO 25 |
| LoRa TX  |   GPIO 26 |
| I2C SDA  |   GPIO 21 |
| I2C SCL  |   GPIO 22 |
| ESC PWM  |   GPIO 27 |

Servo pins will be documented once the final flight-control channel layout is established.

---

# 📡 Communication Protocol

AERION uses custom binary packets between the GCS and the flight controller.

## Joystick Command Packet

```text
[HEADER1]
[HEADER2]
[PITCH]
[ROLL]
[YAW]
[THROTTLE]
[FLIGHT_MODE]
[CHECKSUM]
```

Current header:

```text
0x25 0x28
```

Checksum:

```text
PITCH ^ ROLL ^ YAW ^ THROTTLE ^ FLIGHT_MODE
```

## Telemetry Packet

Telemetry contains information such as:

* GPS satellites
* Latitude
* Longitude
* IMU pitch
* IMU roll

Telemetry packets use a separate header and checksum for validation.

---

# 🖥️ Ground Control Station

The GCS is being developed using:

* C#
* .NET / ASP.NET Core
* SignalR
* SharpDX DirectInput
* SerialPort
* Leaflet for mapping

The GCS is responsible for translating joystick input into AERION command packets and displaying flight telemetry.

---

# 📁 Project Structure

```text
AERION/
│
├── firmware/
│   └── ESP32/
│       ├── src/
│       ├── include/
│       └── platformio.ini
│
├── gcs/
│   └── UAV-GroundControl/
│       ├── Hubs/
│       ├── Services/
│       ├── Controllers/
│       └── ...
│
├── hardware/
│   └── ...
│
├── docs/
│   └── ...
│
├── .gitignore
├── README.md
└── LICENSE
```

---

# 🛠️ Required Software

### Firmware

* PlatformIO
* VS Code
* ESP32 PlatformIO framework

### Ground Control Station

* .NET SDK
* Visual Studio / VS Code
* Windows
* Compatible USB joystick/controller
* USB-to-Serial LoRa interface

---

# 📦 Required Hardware

To reproduce the current system, the following hardware is required:

### Flight Side

* ESP32 development board
* LoRa module
* GPS module
* IMU
* Magnetometer
* ESC
* Brushless motor
* Servos
* LiPo battery
* PDB/BEC
* RC fixed-wing airframe

### Ground Side

* PC
* USB joystick/game controller
* LoRa module
* USB-to-Serial connection

---

# 🗺️ Roadmap

## 🔄 In Development

* [x] ESP32 flight controller base
* [x] LoRa command communication
* [x] LoRa telemetry
* [x] GPS integration
* [x] IMU integration
* [x] ESC control
* [x] GCS joystick control
* [x] Communication failsafe
* [x] Joystick disconnect detection
* [x] Joystick automatic reconnection
* [x] Serial connection watchdog
* [ ] Servo integration
* [ ] Final flight-control channel mapping
* [ ] Flight-mode implementation

## 🚧 Planned

* [ ] MPU9250 full integration
* [ ] BMP280 altitude estimation
* [ ] Magnetometer heading
* [ ] Sensor fusion
* [ ] Attitude estimation
* [ ] PID stabilization
* [ ] Roll stabilization
* [ ] Pitch stabilization
* [ ] Yaw control
* [ ] Altitude estimation
* [ ] Airspeed measurement
* [ ] Battery voltage/current telemetry
* [ ] RSSI monitoring
* [ ] Better telemetry protocol
* [ ] Flight-data logging
* [ ] GCS flight map
* [ ] Waypoint system
* [ ] Return-to-home
* [ ] Autonomous flight
* [ ] Mission planning

---

# ⚠️ Safety

AERION is an experimental UAV project.

**Never test the motor with a propeller installed while developing or debugging the flight controller.**

The current failsafe system is not a replacement for a certified RC/UAV flight-control system.

Do not fly the aircraft until:

* Control surfaces have been verified
* Servo directions have been verified
* Failsafe has been tested
* Motor/ESC behavior has been verified
* Sensor readings have been validated
* Radio link reliability has been tested
* The aircraft has been mechanically inspected

---

# 🎯 Project Goals

The long-term goal of AERION is to develop a complete custom UAV ecosystem consisting of:

```text
             AERION
                │
      ┌─────────┴─────────┐
      │                   │
 Flight Controller      GCS
      │                   │
 ┌────┼────┐         ┌────┼────┐
 GPS  IMU  ESC       Map  Joystick
      │    │              │
   Sensors Servos      Telemetry
      │                   │
      └─────────┬─────────┘
                │
             LoRa Link
```

The project is primarily a learning and engineering project focused on:

* Embedded C/C++
* ESP32 development
* RF communication
* UART/I2C
* Flight-control systems
* Sensor processing
* Real-time systems
* C#/.NET
* Networking
* Ground-control software
* Hardware/software integration

---

# 📜 License

This project is open-source. See the `LICENSE` file for details.

---

## 👨‍💻 Author

Developed by **enisertdev**.

AERION is an independent experimental UAV project built from the ground up.
