# 7 segment led clock

This project uses esp32 and filament leds to create 7-segment display for clock and temperature / humidity monitoring. 
The idea is to build a high-brightness, IoT-connected LED filament clock. It utilizes an ESP32-C3 for logic and PCA9685 PWM controllers to drive 28 individual LED filaments via AO3400 N-Channel MOSFETs.

## 1. Project Overview
* Goal: Build a 4-digit, 7-segment digital clock using flexible LED filaments.
* Core Hardware: ESP32-C3, 2x PCA9685, 28x AO3400 MOSFETs.
* Power System: Dual-rail (5V for ESP32; 3.3V 3A Buck Converter for LED filaments).
* Logic: NTP time sync, light-based auto-dimming
* Extra features: temperature and humidity monitoring, RGB colon, settings via buttons.


## 2. Hardware Components & GPIO Mapping

Controller & I2C Bus:
* Microcontroller: ESP32-C3
* I2C SDA: GPIO 7
* I2C SCL: GPIO 5

Bus Devices:
* PCA9685 #1: Address 0x40 (Digits 1 & 2)
* PCA9685 #2: Address 0x41 (Digits 3 & 4)
* BH1750: Ambient Light Sensor
* AHT30: Temperature & Humidity Sensor

External Peripherals:
* Two WS2812B leds (RGB Colon): GPIO 2, top dot is led 0, bottom dot is led 1.
* Button 1 (Effect): GPIO 9
* Button 2 (Brightness): GPIO 0

Power Rails:
* Primary Source: 5V DC (powers ESP32 via VIN).
* Filament Rail: 3.3V 3A Buck Converter (powers LED Anodes directly).
* Ground: Common GND between all modules.

## 3. Features
* Time display: 4-digit, 7-segment display with NTP time sync.
    * The time is displayed in HH:MM format.
    * Fetch time over WiFi, store in RTC, then disconnect WiFi to save power.
    * Cycle display between Time (HH:MM), Temp (°C), and Humidity (%).

* Colon: RGB LED colon with 2 modes (time, environment).
    * Time mode: Colon blinks every second.
    * Environment mode: The 2nd dot (bottom) is always on and the 1st dot (top) is off of colon (for decimal point).

* Temperature and humidity monitoring: Display temperature and humidity with AHT30 sensor.
    * The temperature and humidity are displayed for 5 seconds each, show at the 26-30 and 56-59 of the minute.
    * The temperature is displayed in Celsius. With the decimal point. (e.g. 25.5). The last digit is showing the Celsius symbol by using the top-right segment (square).
    * The humidity is displayed in percentage. With the decimal point. (e.g. 65.3). The last digit is showing the humidity symbol by using letter H.
    * The decimal point is displayed via the 2nd dot (bottom) of colon.
    * The color of temperature and humidity is decimal dot is based on the level of temperature and humidity.
        * Hot > 30°C: Red
        * Warm 25°C - 30°C: Yellow
        * Normal 20°C - 25°C: Green
        * Cold < 20°C: Blue
        * Humidity color is always white.

* Auto-dimming: Adjust brightness based on ambient light with BH1750 sensor.
    * The brightness of the 7-segment display is adjusted that lower ambient light, lower the brightness.
    * The brightness is never exceed 1000 (out of 4095) PWM value to prevent the burnout.
    * Check the brightness sensor every 2 seconds.

* Settings via buttons: Change display effects, and brightness with buttons.
    * The brightness is adjusted circularly in 5 steps: 0%, 25%, 50%, 75%, 100%. When the brightness is 0%, the 7-segment display is turned off. RGB colon is off.
    * The display effect is adjusted circularly in 3 modes:
        * 1st mode: Blink colon every second with orange color.
        * 2nd mode: RGB colon changes color every second (always on).
        * 3rd mode: Breathing colon effect with 2 seconds interval, the color is orange.
    * The settings are saved in NVS (Non-Volatile Storage).
    * Note that, for the effect mode, the colon effect is not applied when the brightness is 0% and in the environment mode.
         
## 4. Key Logic & Parameters
* Time sync: Fetch time over WiFi everyday at 00:00, store in RTC, then disconnect WiFi to save power.
* Segment Mapping (Standard A-G):

| Segment | Index |
| :--- | :--- |
| A (Bot) | 0 |
| B (LL) | 1 |
| C (UL) | 2 |
| D (Top) | 3 |
| E (UR) | 4 |
| F (LR) | 5 |
| G (Mid) | 6 |

Mapping is wired consistently across all 4 digits. As listed below. Please implement the mapping in the code that explicitly shows the wiring. The purpose is to make it easy to understand the wiring, possiblly to change the wiring and modify the code in the future.

* 1st digit (leftmost) is driven by PCA1, pins 15-9.
* 2nd digit is driven by PCA1, pins 6-0.
* 3rd digit is driven by PCA2, pins 9-15.
* 4th digit (rightmost) is driven by PCA2, pins 0-6.

PWM Settings:
* PCA1: 0x41
* PCA2: 0x40
* PWM Frequency: 1000Hz
* PWM Output Mode: Totem Pole
* Max Safe PWM Value: Constrain to 1000 to prevent the filament burnout

Persistent Storage:
* Use ESP32 Preferences.h to store the:
    * Brightness multiplier alpha (0, 0.25, 0.5, 0.75, 1.0). Default is 0.25.
    * Display effect mode (Blink, RGB, Breathing). Default is Blink.
