#include "WiFi.h"
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include "lcdgfx.h"

#ifndef WIFISSID
#include "creds.h"
#endif

const String serverAddr = FIEND_SERVER_ADDR;
const String apiSpeechRec = serverAddr + "/api/speechToText";
const String apiAskQuestion = serverAddr + "/api/askQuestion";

#define BUTTON_PIN 14

#define MIC_IN A1 //27

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
#define SAMPLE_DELAY 1000000/AUDIO_SAMPLE_RATE

uint8_t audioData[BUFFER_SIZE];
uint32_t bufferIndex = 0;

bool recording = false;
long startTime;
PinStatus buttonState;

enum LedState : uint32_t {
    Off,
    Flashing,
    Static,
};

// 0 uses default SPI frequency
DisplaySSD1331_96x64x16_SPI display(OLED_RES, {-1, OLED_CS, OLED_DC, 0, OLED_SCL_SCK, OLED_SDA_MOSI});

WiFiClass wifi;

String httpPostRequest(String url, const uint8_t* body, size_t size, String contentType = "application/octet-stream", uint16_t timeout=10000) {

    HTTPClient http;
    http.begin(url);
    http.setTimeout(timeout);
    http.addHeader("Content-Type", contentType);
    int httpCode = http.POST(body, size);

    if (httpCode > 0) {
        return http.getString();
    } else {
        Serial.printf("Request failed: %s\n", http.errorToString(httpCode).c_str());
        return http.errorToString(httpCode);
    }

}

#define FONT_WIDTH 6
#define FONT_HEIGHT 8

void printMultiLineString(const String& text, DisplaySSD1331_96x64x16_SPI& _display, uint16_t startRow) {

    uint16_t rowIndex = startRow;
    uint16_t columnIndex;

    for (size_t i = 0; i < text.length() && (rowIndex*FONT_HEIGHT < _display.height()); i++) {

        if (text.c_str()[i] == '\n') {
            rowIndex++;
            columnIndex = 0;
        } else if (text.c_str()[i] == ' ' && columnIndex == 0) {
            continue;
        } else {

            char characterString[] = {text.c_str()[i], '\0'};

            if (columnIndex*FONT_WIDTH >= _display.width()) {
                
                rowIndex++;
                columnIndex = 0;

                _display.printFixed(0, rowIndex*FONT_HEIGHT, characterString);

            } else {

                _display.printFixed(columnIndex*FONT_WIDTH, rowIndex*FONT_HEIGHT, characterString);
                columnIndex++;

            }
            
        }
        
    }
    
}

void handleAudio() {

    recording = false;
    rp2040.fifo.push(Flashing);

    display.clear();
    display.printFixed(0, 0, "Processing...");

    String textResponse = httpPostRequest(apiSpeechRec, audioData, bufferIndex);
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, textResponse.c_str());

    String userMessage = jsonDoc["text"].as<String>();

    display.clear();
    display.printFixed(0, 0, "You:");
    printMultiLineString(userMessage, display, 1);
    Serial.println(userMessage);

    String llmForm = "question=" + userMessage + "&";
    String llmResponse = httpPostRequest(apiAskQuestion, (const uint8_t*) llmForm.c_str(), llmForm.length(), "application/x-www-form-urlencoded");
    jsonDoc.clear();
    deserializeJson(jsonDoc, llmResponse.c_str());

    String llmMessage = jsonDoc["llmResponse"].as<String>();
    Serial.println(llmMessage);

    display.printFixed(0, 16, "Fiend:");
    printMultiLineString(llmMessage, display, 3);

    // Cleanup
    rp2040.fifo.push(Off);
    bufferIndex = 0;
    jsonDoc.clear();
    memset(audioData, 0, BUFFER_SIZE);

}

void setup() {

    analogReadResolution(12);

    display.begin();
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.printFixed(0, 0, "Starting up...", STYLE_NORMAL);

    Serial.begin();

    wifi.begin(WIFISSID, WIFIPASSWORD);

    if (wifi.status() == WL_CONNECTED) {
        Serial.printf("Connected to WiFi %s\n", WIFISSID);
    } else {
        Serial.printf("Failed to connect to WiFi\n");
    }
    
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

                audioData[bufferIndex] = static_cast<uint8_t>(map(analogRead(MIC_IN), 0, 4095, 0, 255));
                bufferIndex++;
                delayMicroseconds(SAMPLE_DELAY);

            }
        
        } else {
            
            handleAudio();
        
        }
        

    } else {

        if (buttonState == HIGH) {
            startTime = millis();
            recording = true;
            rp2040.fifo.push(Static);
        }
        
    }
    
}

// Core 1 setup & code
// Core 1 controls the LED

#define LED_PIN 15

LedState currentLedState = Off;
LedState previousLedState = currentLedState;
PinStatus ledPinState = LOW;

#define FLASHING_DELAY 250
long flashingDelta = 0;

void setup1() {

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

}

void loop1() {
    
    if (rp2040.fifo.available() >= 1) {
    
        currentLedState = static_cast<LedState>(rp2040.fifo.pop());

    }
    
    switch (currentLedState) {
    case Off:
        if (currentLedState != previousLedState) {
            previousLedState = currentLedState;
            digitalWrite(LED_PIN, LOW);
            ledPinState = LOW;
        }
        break;
    case Static:
        if (currentLedState != previousLedState) {
            previousLedState = currentLedState;
            digitalWrite(LED_PIN, HIGH);
            ledPinState = HIGH;
        }
        break;
    case Flashing:
        if (currentLedState != previousLedState) {
            previousLedState = currentLedState;
        }
        
        if (millis()-flashingDelta > FLASHING_DELAY) {
            
            if (ledPinState) {
                digitalWrite(LED_PIN, LOW);
                ledPinState = LOW;
            } else {
                digitalWrite(LED_PIN, HIGH);
                ledPinState = HIGH;
            }

            flashingDelta = millis();
            
        }
        break;
    default:
        break;
    }

}