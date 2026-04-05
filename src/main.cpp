#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_PWMServoDriver.h>
#include <FastLED.h>
#include <hp_BH1750.h>
#include <Adafruit_AHTX0.h>
#include <Preferences.h>
#include "time.h"

// --- CONFIGURATION included from config.h ---
#include "config.h"

const uint8_t charMap[10] = {
    0b00111111, // 0 (Bot, LL, UL, Top, UR, LR)
    0b00110000, // 1 (UR, LR)
    0b01011011, // 2 (Top, UR, Mid, LL, Bot)
    0b01111001, // 3 (Top, UR, Mid, LR, Bot)
    0b01110100, // 4 (UL, Mid, UR, LR)
    0b01101101, // 5 (Top, UL, Mid, LR, Bot)
    0b01101111, // 6 (Top, UL, LL, Bot, LR, Mid)
    0b00111000, // 7 (Top, UR, LR)
    0b01111111, // 8 (All)
    0b01111101  // 9 (Top, UL, UR, Mid, LR, Bot)
};

const uint8_t CHAR_BLANK  = 0b00000000;
const uint8_t CHAR_DEGREE = 0b01011100; // Top, UL, UR, Mid
const uint8_t CHAR_H      = 0b01110110; // LL, UL, UR, LR, Mid 

// --- Objects ---
Adafruit_PWMServoDriver pca1 = Adafruit_PWMServoDriver(0x41);
Adafruit_PWMServoDriver pca2 = Adafruit_PWMServoDriver(0x40);
CRGB leds[NUM_LEDS];
hp_BH1750 lightSensor;
Adafruit_AHTX0 aht;
Preferences prefs;

// --- STATE VARIABLES ---
enum DisplayState {
    SHOW_TIME,
    SHOW_TEMP,
    SHOW_HUMID
};
DisplayState currentState = SHOW_TIME;

// Persisted Preferences
float alpha = 0.25;
int effectMode = 0; // 0=Blink, 1=RGB, 2=Breathing

// Sensor Values
float currentTemp = 0; 
float currentHum = 0;
int ambientPwmMax = 500; // Calculated based on BH1750 (100 to MAX_SAFE_PWM)

struct Button {
    uint8_t pin;
    bool state;
    bool lastState;
    unsigned long lastDebounceTime;
    bool pressed;
};

Button btnBrightness = {PIN_BTN_BRIGHNESS, HIGH, HIGH, 0, false};
Button btnEffect = {PIN_BTN_EFFECT, HIGH, HIGH, 0, false};

// Function prototypes
void syncTime();
void updateDisplayForState(int hr, int min, int sec);
void updateColon();
void setDigit(uint8_t digit, uint8_t charCode, int pwmValue);
void updateButton(Button &btn);

void setup()
{
    Serial.begin(115200);
    Wire.begin(PIN_SDA, PIN_SCL); // ESP32-C3 SDA=7, SCL=5

    pca1.begin();
    pca1.setPWMFreq(1000);
    pca1.setOutputMode(true); 

    pca2.begin();
    pca2.setPWMFreq(1000);
    pca2.setOutputMode(true); 

    FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(255);

    pinMode(btnBrightness.pin, INPUT_PULLUP);
    pinMode(btnEffect.pin, INPUT_PULLUP);

    if (!lightSensor.begin(BH1750_TO_GROUND)) {
        Serial.println("Sensors: BH1750 missing");
    }
    if (!aht.begin()) {
        Serial.println("Sensors: AHT20 missing");
    }

    prefs.begin("clock", false);
    alpha = prefs.getFloat("alpha", 0.25f);
    effectMode = prefs.getInt("effect", 0);

    // Initial explicit full turn off for all segments
    for(int i=0; i<4; i++) {
        setDigit(i, CHAR_BLANK, 0);
    }

    syncTime();
    
    // Quick initial sensor read before starting loop
    sensors_event_t h_event, t_event;
    aht.getEvent(&h_event, &t_event);
    currentTemp = t_event.temperature;
    currentHum = h_event.relative_humidity;
    lightSensor.start(); 
}

void loop()
{
    unsigned long currentMillis = millis();

    updateButton(btnBrightness);
    updateButton(btnEffect);

    if (btnBrightness.pressed) {
        btnBrightness.pressed = false;
        alpha += 0.25f;
        if (alpha > 1.05f) alpha = 0.0f; // loop back to 0 (off)
        prefs.putFloat("alpha", alpha);
    }
    
    if (btnEffect.pressed) {
        btnEffect.pressed = false;
        effectMode = (effectMode + 1) % 3;
        prefs.putInt("effect", effectMode);
    }

    // Read Sensors periodically (every 2 seconds)
    static unsigned long lastSensorRead = 0;
    if (currentMillis - lastSensorRead > 2000) {
        lastSensorRead = currentMillis;
        
        sensors_event_t h_event, t_event;
        aht.getEvent(&h_event, &t_event);
        currentTemp = t_event.temperature;
        currentHum = h_event.relative_humidity;

        if (lightSensor.hasValue()) {
            float lux = lightSensor.getLux();
            // Map 0-500 lux to 100-1000 max pwm
            int pwm_c = int((lux / 500.0) * (MAX_SAFE_PWM - 100)) + 100;
            if (pwm_c > MAX_SAFE_PWM) pwm_c = MAX_SAFE_PWM;
            if (pwm_c < 100) pwm_c = 100;
            ambientPwmMax = pwm_c;
            lightSensor.start();
        }
    }

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) { 
        int sec = timeinfo.tm_sec;
        int min = timeinfo.tm_min;
        int hr = timeinfo.tm_hour;

        // Daily sync check
        static bool syncedToday = true; // Started with a sync in setup
        if (hr == 0 && min == 0 && sec == 0) {
            if (!syncedToday) {
                // Dim screen briefly to indicate sync? (or skip)
                syncTime();
                syncedToday = true;
            }
        } else {
            syncedToday = false;
        }

        if (sec >= 26 && sec <= 30) {
            currentState = SHOW_TEMP;
        } else if (sec >= 56 && sec <= 59) {
            currentState = SHOW_HUMID;
        } else {
            currentState = SHOW_TIME;
        }

        updateDisplayForState(hr, min, sec);
    }
    updateColon();
}

Adafruit_PWMServoDriver* getPCAForDigit(uint8_t digit) {
    if (digit == 0 || digit == 1) return &pca1;
    return &pca2;
}

const uint8_t* getPinsForDigit(uint8_t digit) {
    if (digit == 0) return digit1_pins;
    if (digit == 1) return digit2_pins;
    if (digit == 2) return digit3_pins;
    return digit4_pins;
}

void setDigit(uint8_t digit, uint8_t charCode, int pwmValue) {
    Adafruit_PWMServoDriver* pca = getPCAForDigit(digit);
    const uint8_t* pins = getPinsForDigit(digit);
    
    // Full OFF to ensure no ticks
    int _offPWMVal = 4096;
    int _val = pwmValue;
    
    if (_val <= 0) {
        _val = 0; 
    } else if (_val > MAX_SAFE_PWM) {
        _val = MAX_SAFE_PWM;
    }

    for (int i=0; i<7; i++) {
        if (charCode & (1 << i)) {
            if (_val <= 0) {
                pca->setPin(pins[i], 0); // Hardware library full off
            } else {
                pca->setPWM(pins[i], 0, _val);
            }
        } else {
            pca->setPin(pins[i], 0); 
        }
    }
}

void updateDisplayForState(int hr, int min, int sec) {
    int pwm = (int)(ambientPwmMax * alpha);

    if (alpha <= 0) {
        for (int i=0; i<4; i++) setDigit(i, CHAR_BLANK, 0);
        return;
    }

    if (currentState == SHOW_TIME) {
        int hp = hr;         // allow 24 format
        int m1 = min / 10;
        int m2 = min % 10;
        int h1 = hp / 10;
        int h2 = hp % 10;
        
        if (h1 == 0) setDigit(0, CHAR_BLANK, pwm);
        else setDigit(0, charMap[h1], pwm);
        setDigit(1, charMap[h2], pwm);
        setDigit(2, charMap[m1], pwm);
        setDigit(3, charMap[m2], pwm);
    } 
    else if (currentState == SHOW_TEMP) {
        int tInt = (int)(currentTemp * 10); 
        int d1 = (tInt / 100) % 10;
        int d2 = (tInt / 10) % 10;
        int d3 = tInt % 10;

        if (tInt < 100) setDigit(0, CHAR_BLANK, pwm);
        else setDigit(0, charMap[d1], pwm);
        setDigit(1, charMap[d2], pwm);
        setDigit(2, charMap[d3], pwm);
        setDigit(3, CHAR_DEGREE, pwm);
    } 
    else if (currentState == SHOW_HUMID) {
        int hInt = (int)(currentHum * 10);
        int d1 = (hInt / 100) % 10;
        int d2 = (hInt / 10) % 10;
        int d3 = hInt % 10;

        if (hInt < 100) setDigit(0, CHAR_BLANK, pwm);
        else setDigit(0, charMap[d1], pwm);
        setDigit(1, charMap[d2], pwm);
        setDigit(2, charMap[d3], pwm);
        setDigit(3, CHAR_H, pwm);
    }
}

void updateColon() {
    if (alpha <= 0) {
        leds[0] = CRGB::Black;
        leds[1] = CRGB::Black;
        FastLED.show();
        return;
    }

    // Scale ambient and alpha for ws2812 multiplier (out of 255)
    float ambientScale = (float)ambientPwmMax / MAX_SAFE_PWM;
    uint8_t displayBrightness = (uint8_t)(255 * alpha * ambientScale);
    FastLED.setBrightness(displayBrightness);

    if (currentState != SHOW_TIME) {
        // Environment Mode - Bottom dot on
        leds[0] = CRGB::Black;
        CRGB col = CRGB::White;
        
        if (currentState == SHOW_TEMP) {
            float t = currentTemp;
            if (t > 30) col = CRGB::Red;
            else if (t >= 25) col = CRGB::Yellow;
            else if (t >= 20) col = CRGB::Green;
            else col = CRGB::Blue;
        }
        leds[1] = col;
    } else {
        // Time Mode - effects
        unsigned long ms = millis();
        switch(effectMode) {
            case 0: // Blink orange every second
                if ((ms % 1000) < 500) {
                    leds[0] = CRGB::OrangeRed;
                    leds[1] = CRGB::OrangeRed;
                } else {
                    leds[0] = CRGB::Black;
                    leds[1] = CRGB::Black;
                }
                break;
            case 1: { // RGB cycle
                uint8_t hue = (ms / 1000) * 32; 
                leds[0] = CHSV(hue, 255, 255);
                leds[1] = CHSV(hue, 255, 255);
                break;
            }
            case 2: { // Breathing colon, 2s interval, orange
                uint8_t wave = triwave8((ms * 255) / 2000); 
                CRGB col = CRGB::OrangeRed;
                col.nscale8(wave);
                leds[0] = col;
                leds[1] = col;
                break;
            }
        }
    }
    FastLED.show();
}

void updateButton(Button &btn) {
    bool reading = digitalRead(btn.pin);
    if (reading != btn.lastState) {
        btn.lastDebounceTime = millis();
    }
    if ((millis() - btn.lastDebounceTime) > 50) {
        if (reading != btn.state) {
            btn.state = reading;
            if (btn.state == LOW) {
                btn.pressed = true;
            }
        }
    }
    btn.lastState = reading;
}


void syncTime() {
    Serial.println("Connecting to WiFi for NTP Sync...");
    WiFi.mode(WIFI_STA);
    
    // Sometimes lowering TX power prevents sudden current spikes (brownouts) 
    // that crash the radio transmitter while connecting
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    
    WiFi.disconnect(true);
    delay(1000);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        configTzTime(tzInfo, "pool.ntp.org");
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 10000)) {
            Serial.println("Time synchronized.");
        } else {
            Serial.println("Failed to get time.");
        }
    } else {
        Serial.println("WiFi connection failed.");
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}