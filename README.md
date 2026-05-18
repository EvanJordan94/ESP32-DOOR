# ESP32 Smart Door Lock System

## Overview

This project is an ESP32-based smart door lock system that integrates multiple authentication methods and IoT components.

The system includes an ESP32 DevKit controller, an ESP32-CAM module, RFID/fingerprint-based access control, and server-side communication for monitoring and management. The project demonstrates embedded C++ firmware development, IoT device integration, camera module usage, and communication between hardware and server components.

## Key Features

- ESP32-based door lock controller
- ESP32-CAM integration for image/camera functionality
- RFID-based authentication
- Fingerprint-based authentication
- Door lock control using relay/servo
- Server communication for data monitoring and management
- Modular firmware structure using PlatformIO
- Embedded C++ development for IoT devices

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
