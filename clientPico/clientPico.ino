#include "WiFi.h"
#include "lcdgfx.h"

#ifndef WIFISSID
#include "creds.h"
#endif

#define LED_PIN 15
#define BUTTON_PIN 14

#define MIC_IN 26

#define OLED_SCL_SCK 18
#define OLED_SDA_MOSI 19
#define OLED_RES 6
#define OLED_DC 7
#define OLED_CS 8

// Audio variables

#define AUDIO_SAMPLE_RATE 4000
#define AUDIO_BITS 8
#define MAX_RECORDING_LENGTH 15

#define BUFFER_SIZE (AUDIO_SAMPLE_RATE*AUDIO_BITS*MAX_RECORDING_LENGTH)/8

// 0 uses default SPI frequency
DisplaySSD1331_96x64x16_SPI display(OLED_RES, {-1, OLED_CS, OLED_DC, 0, OLED_SCL_SCK, OLED_SDA_MOSI});

WiFiClass wifi;

void setup() {

    Serial.begin(9600);

    wifi.begin(WIFISSID, WIFIPASSWORD);

    if (wifi.status() == WL_CONNECTED)
    {
        Serial.printf("Connected to WiFi %s\n", WIFISSID);
    } else {
        Serial.printf("Failed to connect to WiFi\n");
    }
    

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(MIC_IN, INPUT);

    digitalWrite(LED_PIN, HIGH);

    Serial.println("Starting display");

    display.begin();
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.printFixed(0, 0, "Fiend", STYLE_NORMAL);

}

void loop() {

    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_PIN, HIGH);

    Serial.println("test");

    delay(1000);

}