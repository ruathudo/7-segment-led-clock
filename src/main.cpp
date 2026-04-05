#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_PWMServoDriver.h>
#include <FastLED.h>
#include <hp_BH1750.h>
#include <Adafruit_AHTX0.h>
#include <Preferences.h>
#include "time.h"

// --- CONFIGURATION ---
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const long gmtOffset_sec = 25200; // Adjust for your timezone (e.g., +7h = 25200)
const int daylightOffset_sec = 0;

#define PIN_BTN_BRIGHNESS 0
#define PIN_BTN_EFFECT 9
#define PIN_WS2812 2
#define NUM_LEDS 2
#define MAX_SAFE_PWM 1500
#define PIN_SDA 7
#define PIN_SCL 5

// --- Objects ---
Adafruit_PWMServoDriver pca1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pca2 = Adafruit_PWMServoDriver(0x41);
CRGB leds[NUM_LEDS];
hp_BH1750 lightSensor;
Adafruit_AHTX0 aht;

// --- STATE VARIABLES ---
enum DisplayState
{
    SHOW_TIME,
    SHOW_TEMP,
    SHOW_HUMID
};
DisplayState currentState = SHOW_TIME;
float alpha = 1.0;
float currentTemp = 0; 
float currentHum = 0;
unsigned long previousMillis = 0;
bool ledState = false;

// 7-Segment Mapping (Standard A-G)
const byte segMap[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
const byte charDeg = 0b01100011; // Degree symbol
const byte percL = 0b01100011;   // % Top
const byte percR = 0b01011100;   // % Bottom

void setup()
{
    Serial.begin(115200);
    Wire.begin(PIN_SDA, PIN_SCL); // ESP32-C3 SDA=7, SCL=5

    pca1.begin();
    pca1.setPWMFreq(1000);
    pca1.setOutputMode(true); // totem pole output, not open drain

    pca2.begin();
    pca2.setPWMFreq(1000);
    pca2.setOutputMode(true); // totem pole output, not open drain

    FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(255);
}

void loop()
{
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
        previousMillis = currentMillis;
        if (ledState) {
            fill_solid(leds, NUM_LEDS, CRGB::Orange);
        } else {
            fill_solid(leds, NUM_LEDS, CRGB::Black);
        }
        ledState = !ledState;
        FastLED.show();
    }

    // Fade UP
    for (int brightness = 0; brightness <= 1000; brightness += 20)
    {
        // Note: We NO LONGER subtract from 4095 because the
        // transistor handles the inversion logic.
        for (int i = 0; i < 16; i++)
        {
            pca1.setPin(i, brightness);
            pca2.setPin(i, brightness);
        }
        delay(10);
    }

    // Hold at max for a second to measure current
    Serial.println("Max Brightness reached. Check your multimeter!");
    delay(1000);

    // Fade DOWN
    for (int brightness = 1000; brightness >= 0; brightness -= 20)
    {
        for (int i = 0; i < 16; i++)
        {
            pca1.setPin(i, brightness);
            pca2.setPin(i, brightness);
        }

        delay(10);
    }

    delay(1000);
}