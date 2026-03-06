#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pca2 = Adafruit_PWMServoDriver(0x41);

void setup()
{
    Serial.begin(115200);
    Wire.begin(7, 5); // ESP32-C3 SDA=7, SCL=5

    pca1.begin();
    pca1.setPWMFreq(1000);
    pca1.setOutputMode(true); // totem pole output, not open drain

    pca2.begin();
    pca2.setPWMFreq(1000);
    pca2.setOutputMode(true); // totem pole output, not open drain

    Serial.println("Breathing test started on Pin 0...");

    // pca.setPWM(15, 0, 0);
}

void loop()
{
    // Fade UP
    for (int brightness = 0; brightness <= 4000; brightness += 20)
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
    for (int brightness = 4000; brightness >= 0; brightness -= 20)
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