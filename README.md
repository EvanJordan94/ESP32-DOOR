# ESP32 Smart Door Lock System

## Overview

This project is an ESP32-based smart door lock system that integrates multiple authentication methods and IoT components.

The system includes an ESP32 DevKit controller, an ESP32-CAM module, RFID/fingerprint-based access control, and server-side communication for monitoring and management. The project demonstrates embedded C++ firmware development, IoT device integration, camera module usage, hardware design, and communication between hardware and server components.

## Key Features

- ESP32-based door lock controller
- ESP32-CAM integration for camera functionality
- RFID-based authentication
- Fingerprint-based authentication
- Door lock control using relay/servo
- Server communication for data monitoring and management
- Modular firmware structure using PlatformIO
- Embedded C++ development for IoT devices
- Schematic and PCB design using Altium Designer
- Gerber files for PCB manufacturing

## Technologies Used

- ESP32 DevKit
- ESP32-CAM
- C/C++
- PlatformIO
- Arduino framework for ESP32
- RFID module
- Fingerprint sensor
- Relay / Servo motor
- Python server
- Wi-Fi communication
- Altium Designer
- PCB Design

## System Architecture

```text
User
 |
 |-- RFID Card
 |-- Fingerprint
 |-- Camera
 |
 v
ESP32 Door Controller  <---->  ESP32-CAM
 |
 v
Door Lock / Relay / Servo
 |
 v
Server / Database / Monitoring
```

## Hardware Design

The hardware design was created using Altium Designer for the ESP32-based smart door lock system.

Main hardware files:

- `hardware/altium/ESP32_Project.PrjPcb` - Altium PCB project file
- `hardware/altium/Sheet2.SchDoc` - Schematic design
- `hardware/altium/PCB2.PcbDoc` - PCB layout design
- `docs/images/schematic.pdf` - Exported schematic PDF
- `hardware/gerber/` - PCB manufacturing files

## Project Structure

```text
ESP32-DOOR/
│
├── code esp32 devkitv1/
│   └── door/                 # ESP32 DevKit firmware
│
├── code_32cam/
│   └── code/                 # ESP32-CAM firmware
│
├── code_server/              # Server-side code
├── lib/                      # Custom libraries
│
├── hardware/
│   ├── altium/               # Altium schematic and PCB design files
│   └── gerber/               # PCB manufacturing files
│
├── docs/
│   └── images/               # Schematic PDF and project images
│
├── README.md
└── .gitignore
```

## My Responsibilities

- Developed embedded C++ firmware for the ESP32-based door controller
- Integrated RFID/fingerprint authentication modules
- Integrated ESP32-CAM for camera-related functionality
- Implemented door lock control using relay/servo mechanism
- Built server-side communication for monitoring and management
- Designed schematic and PCB layout using Altium Designer
- Generated Gerber files for PCB manufacturing
- Tested and debugged hardware/software interaction

## Demo

Demo video: Updating...

## Future Improvements

- Add real hardware images and demo video
- Add system wiring diagram
- Improve error handling and access log management
- Add OTA update support
- Improve code structure and documentation