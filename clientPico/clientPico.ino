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

String httpPostRequest(HTTPClient& http, String url, const uint8_t* body, size_t size, String contentType = "application/octet-stream", uint16_t timeout=15000) {

    http.begin(url);
    http.setTimeout(timeout);
    http.addHeader("Content-Type", contentType);
    int httpCode = http.POST(body, size);

    if (httpCode > 0) {
        String response = http.getString();
        http.end();
        return response;
    } else {
        String errorString = http.errorToString(httpCode);

        Serial.printf("Request failed: %s\n", errorString.c_str());
        http.end();
        return errorString;
    }

}

#define FONT_WIDTH 5
#define FONT_HEIGHT 7

#define WRITE_CHAR(disp, ch, x, y) disp.setTextCursor(x, y); disp.printChar(ch);

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

            if (columnIndex*FONT_WIDTH > (_display.width()-FONT_WIDTH)) {
                
                rowIndex++;
                columnIndex = 0;

                WRITE_CHAR(_display, text.c_str()[i], 0, rowIndex*FONT_HEIGHT);

            } else {

                WRITE_CHAR(_display, text.c_str()[i], columnIndex*FONT_WIDTH, rowIndex*FONT_HEIGHT);
                columnIndex++;

            }
            
        }
        
    }
    
}

#define GREEN_BG_BLACK_FG(disp) disp.setBackground(RGB_COLOR16(0, 255, 0)); disp.setColor(RGB_COLOR16(0, 0, 0));
#define WHITE_FG(disp) disp.setBackground(RGB_COLOR16(0, 0, 0)); disp.setColor(RGB_COLOR16(255, 255, 255));

#define AUDIO_CLEANUP() \
rp2040.fifo.push(Off); \
bufferIndex = 0; \
memset(audioData, 0, BUFFER_SIZE);

#define HANDLE_TIMEOUT_ERROR(respString) \
if(respString == http.errorToString(HTTPC_ERROR_READ_TIMEOUT)) { \
    WHITE_FG(display) \
    display.clear(); \
    display.printFixed(0, 0, "Timeout try again."); \
    AUDIO_CLEANUP() \
    return; \
}

void handleAudio() {

    HTTPClient http;

    recording = false;
    rp2040.fifo.push(Flashing);

    display.clear();
    display.printFixed(0, 0, "Processing...");

    String httpResponse = httpPostRequest(http, apiSpeechRec, audioData, bufferIndex);
    HANDLE_TIMEOUT_ERROR(httpResponse)
    
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, httpResponse.c_str());

    String userMessage = jsonDoc["text"].as<String>();

    display.clear();
    GREEN_BG_BLACK_FG(display)
    display.printFixed(0, 0, "You:", STYLE_BOLD);
    WHITE_FG(display)
    printMultiLineString(userMessage, display, 1);
    Serial.println(userMessage);

    String llmForm = "question=" + userMessage + "&";
    httpResponse = httpPostRequest(http, apiAskQuestion, (const uint8_t*) llmForm.c_str(), llmForm.length(), "application/x-www-form-urlencoded");
    HANDLE_TIMEOUT_ERROR(httpResponse)

    jsonDoc.clear();
    deserializeJson(jsonDoc, httpResponse.c_str());

    String llmMessage = jsonDoc["llmResponse"].as<String>();
    Serial.println(llmMessage);

    GREEN_BG_BLACK_FG(display)
    display.printFixed(0, 14, "Fiend:", STYLE_BOLD);
    WHITE_FG(display)
    printMultiLineString(llmMessage, display, 3);

    // Cleanup
    AUDIO_CLEANUP()
    jsonDoc.clear();

}

void setup() {

    analogReadResolution(12);

    display.begin();
    display.setFixedFont(ssd1306xled_font5x7);
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