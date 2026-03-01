// Project Name: Vision-32, ESP32 TFT Image Processor 
// Date: 5th January, 2026
// Author: Mr. Fayyaz Nisar Shaikh
// Arduino IDE Code (.ino file)
// Email ID: fayyaz.nisar.shaikh@gmail.com
// LinkedIn: https://linkedin.com/in/fayyaz-shaikh-7646312a3/ 
// GitHub: https://github.com/fayyaz-27

/* 
  Contributors: Dr. Yerramreddy Srinivasa Rao
                Mr. Tushar Kashinath Patange
*/


#include <SPI.h>
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

const char* ssid = "esp_tft";
const char* password = "123456789";

using namespace websockets;
WebsocketsServer server;
WebsocketsClient client;

TFT_eSPI tft = TFT_eSPI();

// Current active mode
uint8_t currentMode = 5; // ENHANCE by default
int bwThreshold = 100;
float satFactor = 2.0f;
int brightnessOffset = -50;
float contrastEnh = 1.2f;
int brightnessEnh = 50;
int segThreshold = 120;

// Mode definitions
#define MODE_NORMAL     0
#define MODE_GRAYSCALE  1
#define MODE_BLACKWHITE 2
#define MODE_SATURATION 3
#define MODE_BRIGHTNESS 4
#define MODE_ENHANCE    5
#define MODE_SEGMENT    6
#define MODE_EDGE       7

inline uint8_t get_r(uint16_t c) { return ((c >> 11) & 0x1F) * 255 / 31; }
inline uint8_t get_g(uint16_t c) { return ((c >> 5) & 0x3F) * 255 / 63; }
inline uint8_t get_b(uint16_t c) { return (c & 0x1F) * 255 / 31; }
inline uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void process_block(uint16_t* bitmap, uint16_t w, uint16_t h) {
  switch(currentMode) {
    case MODE_NORMAL:
      return;
      
    case MODE_GRAYSCALE:
      for (uint32_t i = 0; i < w * h; i++) {
        uint8_t r = get_r(bitmap[i]);
        uint8_t g = get_g(bitmap[i]);
        uint8_t b = get_b(bitmap[i]);
        uint8_t gray = (r * 3 + g * 6 + b * 1) / 10;
        bitmap[i] = rgb_to_565(gray, gray, gray);
      }
      break;
      
    case MODE_BLACKWHITE:
      for (uint32_t i = 0; i < w * h; i++) {
        uint8_t r = get_r(bitmap[i]);
        uint8_t g = get_g(bitmap[i]);
        uint8_t b = get_b(bitmap[i]);
        uint8_t gray = (r * 3 + g * 6 + b * 1) / 10;
        uint8_t val = (gray > bwThreshold) ? 255 : 0;
        bitmap[i] = rgb_to_565(val, val, val);
      }
      break;
      
    case MODE_SATURATION:
      for (uint32_t i = 0; i < w * h; i++) {
        uint8_t r = get_r(bitmap[i]);
        uint8_t g = get_g(bitmap[i]);
        uint8_t b = get_b(bitmap[i]);
        float gray = (r * 0.3f + g * 0.59f + b * 0.11f);
        int new_r = gray + (r - gray) * satFactor;
        int new_g = gray + (g - gray) * satFactor;
        int new_b = gray + (b - gray) * satFactor;
        if (new_r < 0) new_r = 0; else if (new_r > 255) new_r = 255;
        if (new_g < 0) new_g = 0; else if (new_g > 255) new_g = 255;
        if (new_b < 0) new_b = 0; else if (new_b > 255) new_b = 255;
        bitmap[i] = rgb_to_565(new_r, new_g, new_b);
      }
      break;
      
    case MODE_BRIGHTNESS:
      for (uint32_t i = 0; i < w * h; i++) {
        int r = get_r(bitmap[i]) + brightnessOffset;
        int g = get_g(bitmap[i]) + brightnessOffset;
        int b = get_b(bitmap[i]) + brightnessOffset;
        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;
        bitmap[i] = rgb_to_565(r, g, b);
      }
      break;
      
    case MODE_ENHANCE:
      for (uint32_t i = 0; i < w * h; i++) {
        int r = ((get_r(bitmap[i]) - 128) * contrastEnh) + 128 + brightnessEnh;
        int g = ((get_g(bitmap[i]) - 128) * contrastEnh) + 128 + brightnessEnh;
        int b = ((get_b(bitmap[i]) - 128) * contrastEnh) + 128 + brightnessEnh;
        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;
        bitmap[i] = rgb_to_565(r, g, b);
      }
      break;
      
    case MODE_SEGMENT:
      for (uint32_t i = 0; i < w * h; i++) {
        uint8_t r = get_r(bitmap[i]);
        uint8_t g = get_g(bitmap[i]);
        uint8_t b = get_b(bitmap[i]);
        uint8_t gray = (r * 3 + g * 6 + b * 1) / 10;
        if (gray > segThreshold)
          bitmap[i] = rgb_to_565(255, 255, 255);
        else
          bitmap[i] = rgb_to_565(0, 0, 0);
      }
      break;
      
    case MODE_EDGE:
      for (uint16_t y = 0; y < h - 1; y++) {
        for (uint16_t x = 0; x < w - 1; x++) {
          uint32_t i = y * w + x;
          uint32_t j = y * w + (x + 1);
          uint32_t k = (y + 1) * w + x;
          uint8_t gray_i = (get_r(bitmap[i]) * 3 + get_g(bitmap[i]) * 6 + get_b(bitmap[i])) / 10;
          uint8_t gray_j = (get_r(bitmap[j]) * 3 + get_g(bitmap[j]) * 6 + get_b(bitmap[j])) / 10;
          uint8_t gray_k = (get_r(bitmap[k]) * 3 + get_g(bitmap[k]) * 6 + get_b(bitmap[k])) / 10;
          int gx = gray_i - gray_j;
          int gy = gray_i - gray_k;
          int mag = abs(gx) + abs(gy);
          if (mag > 255) mag = 255;
          bitmap[i] = rgb_to_565(mag, mag, mag);
        }
      }
      break;
  }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  process_block(bitmap, w, h);
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void handleSerialCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("MODE:")) {
      int mode = cmd.substring(5).toInt();
      if (mode >= 0 && mode <= 7) {
        currentMode = mode;
        Serial.print("OK:MODE:");
        Serial.println(mode);
      }
    }
    else if (cmd.startsWith("PARAM:")) {
      int firstColon = cmd.indexOf(':', 6);
      if (firstColon > 0) {
        String paramName = cmd.substring(6, firstColon);
        String paramValue = cmd.substring(firstColon + 1);
        
        if (paramName == "bwThreshold") {
          bwThreshold = paramValue.toInt();
          Serial.print("OK:bwThreshold:");
          Serial.println(bwThreshold);
        }
        else if (paramName == "satFactor") {
          satFactor = paramValue.toFloat();
          Serial.print("OK:satFactor:");
          Serial.println(satFactor);
        }
        else if (paramName == "brightnessOffset") {
          brightnessOffset = paramValue.toInt();
          Serial.print("OK:brightnessOffset:");
          Serial.println(brightnessOffset);
        }
        else if (paramName == "contrastEnh") {
          contrastEnh = paramValue.toFloat();
          Serial.print("OK:contrastEnh:");
          Serial.println(contrastEnh);
        }
        else if (paramName == "brightnessEnh") {
          brightnessEnh = paramValue.toInt();
          Serial.print("OK:brightnessEnh:");
          Serial.println(brightnessEnh);
        }
        else if (paramName == "segThreshold") {
          segThreshold = paramValue.toInt();
          Serial.print("OK:segThreshold:");
          Serial.println(segThreshold);
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  tft.begin();
  tft.setRotation(3);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  
  Serial.println("\n=== ESP32 Image Processor Ready ===");
  Serial.println("Waiting for serial commands...");
  Serial.print("Current mode: ");
  Serial.println(currentMode);
  
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  
  server.listen(80);

  pinMode(13, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(32, OUTPUT);

  digitalWrite(13, HIGH);
  digitalWrite(27, HIGH);
  digitalWrite(33, HIGH);
  digitalWrite(32, HIGH);



}

void loop() {


  handleSerialCommand();
  
  if(server.poll()) {
    client = server.accept();
  }
  
  if(client.available()) {
    client.poll();
    WebsocketsMessage msg = client.readBlocking();
    
    uint32_t t = millis();
    uint16_t w = 0, h = 0;
    
    TJpgDec.getJpgSize(&w, &h, (const uint8_t*)msg.c_str(), msg.length());
    TJpgDec.drawJpg(0, 0, (const uint8_t*)msg.c_str(), msg.length());
    
    t = millis() - t;
    Serial.print(t);
    Serial.println(" ms");

  }


}

