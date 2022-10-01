#include <Arduino.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "config.h"

#define relayPin 05 // D1

// WiFi Parameters
const char* ssid = SSID;
const char* password = PASSWORD;

// Timers
unsigned long timer = 0;

unsigned long time_limit = 1000;

void setup() {

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf("[SETUP] Connecting...\n");
    digitalWrite(2, LOW);
    delay(1000);
  }
  digitalWrite(2, HIGH); 

  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
}

void update() {
    WiFiClient client;
    HTTPClient http;
    Serial.print("[HTTP] begin...\n");
    if(http.begin(client, "http://192.168.0.62:8001/test.json")) { // http://192.168.0.40:8889/api/device/chauffe-eau
      int httpCode = http.GET();                                                   
      if (httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // Parsing
        StaticJsonDocument<200> root;
        
        DeserializationError error = deserializeJson(root, http.getString());

        // Test if parsing succeeds.
        if (error) {
          Serial.print("[JSON PARSER] deserializeJson() failed: ");
          Serial.println(error.f_str());
          Serial.print("[DECISION] Desactivating\n");
          digitalWrite(relayPin, HIGH);
          return;
        }
        timer = millis();
        time_limit = root["time_limit"];
        if (root["toggle"] == true) {
          Serial.print("[DECISION] Activating\n");
          digitalWrite(relayPin, LOW);
        } else {
          Serial.print("[DECISION] Desactivating\n");
          digitalWrite(relayPin, HIGH);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        Serial.print("[DECISION] Desactivating\n");
        digitalWrite(relayPin, HIGH);
      }
      http.end();
    } else {
      Serial.printf("[HTTP] Unable to connect\n");
      Serial.print("[DECISION] Desactivating\n");
      digitalWrite(relayPin, HIGH);
    }
}

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    if(millis() > timer + time_limit) {
          update();
    }
  }

}
