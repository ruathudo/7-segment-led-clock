# CLAUDE.md

This file provides guidance to Claude Code when working with this ESP32-C3 7-segment LED clock project.

## Project Overview

A high-brightness IoT-connected LED filament clock using ESP32-C3 with NTP time sync, ambient light auto-dimming, and temperature/humidity monitoring. The display uses 28 flexible LED filaments driven by 2x PCA9685 PWM controllers.

## Development Commands

```bash
# Build the project
pio run

# Monitor serial output (in a separate terminal)
pio monitor

# Upload firmware to device
pio run --target upload

# Clean build artifacts
pio run --target clean

# Run a single test (if testing framework is added)
pio test
```

## Hardware Architecture

**Controller:** ESP32-C3 with I2C bus (SDA=GPIO7, SCL=GPIO5)

**I2C Devices:**
- PCA9685 #1: Address 0x41 → Digits 1-2
- PCA9685 #2: Address 0x40 → Digits 3-4
- BH1750: Ambient light sensor
- AHT30: Temperature & humidity sensor

**Peripherals:**
- 2x WS2812B LEDs (RGB colon) on GPIO2: led[0]=top dot, led[1]=bottom dot
- Button1 (Effect): GPIO9, Button2 (Brightness): GPIO0

**Power System:**
- 5V rail powers ESP32 via VIN
- 3.3V 3A Buck Converter powers LED filaments (anodes directly)
- Max PWM constrained to 1000 to prevent filament burnout

## Core Logic & Data Flow

**Time Display (main mode):** Shows HH:MM with blinking orange colon

**Environment Display:**
- Temperature: Shown seconds 26-30 of each minute with °C symbol
- Humidity: Shown seconds 56-59 with % symbol (upper+lower circle segments)
- Decimal dot color indicates temperature level: Blue (<20°C) → Green (20-25°C) → Yellow (25-30°C) → Red (>30°C)

**Auto-dimming:** Ambient lux mapped to PWM range 10-1000, scaled by user alpha multiplier

**Auto night off:** Display blank during 21:00-06:00 unless ambient light present

**Effect modes:**
- Mode 0: Blink orange every second
- Mode 1: RGB color cycling
- Mode 2: Breathing orange (2s interval)

## Key Files

- `src/main.cpp`: Main application logic (setup, loop, display updates, sensors)
- `src/config.h`: Configuration including WiFi credentials, timezone, pin definitions, and segment pin mappings for each digit
- `platformio.ini`: PlatformIO build configuration with dependencies

## Development Notes

- **NTP Sync:** WiFi connects at midnight (00:00) daily, syncs time, then disconnects to save power
- **Settings Persistence:** Brightness multiplier (0-1.0 in 0.25 steps) and effect mode stored in NVS via Preferences.h
- **Segment Mapping:** Standard A-G segments mapped as [Bot, LL, UL, Top, UR, LR, Mid] (indices 0-6)
- **PWM:** 1000Hz frequency with Totem Pole output mode
- **Button Handling:** Non-blocking debouncing at 50ms

## Hardware Wiring

| Segment Index | Segment | Wire Position |
|---------------|---------|---------------|
| 0 | A (Bot)   | index 0       |
| 1 | B (LL)    | index 1       |
| 2 | C (UL)    | index 2       |
| 3 | D (Top)   | index 3       |
| 4 | E (UR)    | index 4       |
| 5 | F (LR)    | index 5       |
| 6 | G (Mid)   | index 6       |

Each digit's pins are explicitly defined in `config.h` for easy wiring modification.
