#pragma once

#include <Arduino.h>

// --- WIFI CONFIGURATION ---
const char *ssid = "[SSID]";
const char *password = "[PASSWORD]";

// Set your timezone using POSIX format. 
// For Europe/Helsinki, use: "EET-2EEST,M3.5.0/3,M10.5.0/4"
const char *tzInfo = "EET-2EEST,M3.5.0/3,M10.5.0/4";

// --- PIN DEFINITIONS ---
#define PIN_BTN_BRIGHNESS 0
#define PIN_BTN_EFFECT 9
#define PIN_WS2812 2
#define PIN_SDA 7
#define PIN_SCL 5

// --- DISPLAY CONFIGURATION ---
#define NUM_LEDS 2
#define MAX_SAFE_PWM 1000

// --- Hardware Mappings ---
// Pins matching exact segment [Bot, LL, UL, Top, UR, LR, Mid] array
const uint8_t digit1_pins[7] = {15, 14, 13, 12, 11, 10, 9};
const uint8_t digit2_pins[7] = {6, 5, 4, 3, 2, 1, 0};
const uint8_t digit3_pins[7] = {9, 10, 11, 12, 13, 14, 15};
const uint8_t digit4_pins[7] = {0, 1, 2, 3, 4, 5, 6};
