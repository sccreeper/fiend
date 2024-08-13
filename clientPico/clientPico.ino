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

#define STRING_PROCESSING_AUDIO "Processing audio of length %d"
#define SAMPLE_DELAY 125

uint8_t audioData[BUFFER_SIZE];
uint32_t bufferIndex = 0;

bool recording = false;
long startTime;
PinStatus buttonState;

// 0 uses default SPI frequency
DisplaySSD1331_96x64x16_SPI display(OLED_RES, {-1, OLED_CS, OLED_DC, 0, OLED_SCL_SCK, OLED_SDA_MOSI});

WiFiClass wifi;

void handleAudio() {

    recording = false;

    digitalWrite(LED_PIN, LOW);

    display.clear();
    display.printFixed(0, 0, "Processing...");
    Serial.printf(STRING_PROCESSING_AUDIO, bufferIndex);

    // Cleanup

    display.clear();
    display.printFixed(0, 0, "Fiend");
    bufferIndex = 0;
    memset(&audioData, 0, BUFFER_SIZE);

}

void setup() {

    display.begin();
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.printFixed(0, 0, "Starting up...", STYLE_NORMAL);

    Serial.begin(9600);

    wifi.begin(WIFISSID, WIFIPASSWORD);

    if (wifi.status() == WL_CONNECTED) {
        Serial.printf("Connected to WiFi %s\n", WIFISSID);
    } else {
        Serial.printf("Failed to connect to WiFi\n");
    }
    

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(MIC_IN, INPUT);

    Serial.println("Starting display");

    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.printFixed(0, 0, "Fiend", STYLE_BOLD);

}

void loop() {

    buttonState = digitalRead(BUTTON_PIN);

    if (recording) {
    
        if (buttonState == HIGH) {

            if (millis() - startTime >= (MAX_RECORDING_LENGTH*1000) || bufferIndex >= BUFFER_SIZE) {

                handleAudio();
            
            } else {

                int sample = analogRead(MIC_IN);
                audioData[bufferIndex] = (uint8_t)(sample >> 2);

                bufferIndex++;

            }

            delayMicroseconds(SAMPLE_DELAY);
        
        } else {
            
            handleAudio();
        
        }
        

    } else {

        if (buttonState == HIGH) {
            startTime = millis();
            recording = true;
            digitalWrite(LED_PIN, HIGH);
        }
        
    }
    
}