#include <Arduino.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "config.h"

int PSM = 05; // D1
int ZC = 04; // D2  
int RelayPin = 14; // D5  
int dimming = 128;  // Dimming level (0-128)  0 = ON, 128 = OFF

void ICACHE_RAM_ATTR zero_crosss_int(void);

// WiFi Parameters
const char* ssid = SSID;
const char* password = PASSWORD;

// Timers
unsigned long timer = 0;
unsigned long timer_prevent_failure = 0;

unsigned long time_limit = 1000;

void setup() {

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf("[SETUP] Connecting...\n");
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }
  digitalWrite(LED_BUILTIN, HIGH); 
  
  
  dimming = 128;
  pinMode(PSM, OUTPUT);// Set AC Load pin as output
  attachInterrupt(digitalPinToInterrupt(ZC), zero_crosss_int, RISING);  // Choose the zero cross interrupt

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
}

// the interrupt function must take no parameters and return nothing
void zero_crosss_int()  // function to be fired at the zero crossing to dim the light
{
  // Firing angle calculation : 1 full 50Hz wave =1/50=20ms 
  // Every zerocrossing thus: (50Hz)-> 10ms (1/2 Cycle) For 60Hz => 8.33ms (10.000/120)
  // 10ms=10000us
  // (10000us - 10us) / 128 = 75 (Approx) For 60Hz =>65

  int dimtime = (75*dimming);    // For 60Hz =>65    
  delayMicroseconds(dimtime);    // Off cycle
  digitalWrite(PSM, HIGH);   // triac firing
  delayMicroseconds(10);         // triac On propogation delay (for 60Hz use 8.33)
  digitalWrite(PSM, LOW);    // triac Off
  //Serial.print("Trig\n");
}

void update() {
    WiFiClient client;
    HTTPClient http;
    Serial.print("[HTTP] begin...\n");
    if(http.begin(client, "http://192.168.0.98:8889/api/device/id/1b9d8/advanced")) {
      int httpCode = http.GET();                                                   
      if (httpCode != 200 || httpCode != 304) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // Parsing
        StaticJsonDocument<200> root;
        
        DeserializationError error = deserializeJson(root, http.getString());

        // Test if parsing succeeds.
        if (error) {
          Serial.print("[JSON PARSER] deserializeJson() failed: ");
          Serial.println(error.f_str());
          Serial.print("[DECISION] Desactivating\n");
          dimming = 128;
          digitalWrite(relayPin, HIGH);
          return;
        }
        timer = millis();
        time_limit = root["time_limit"];
        dimming = root["alpha"]; 
        if (dimming < 3) {
          dimming = 3;
        }
        if (dimming == 128) {
          Serial.print("[DECISION] Desactivating\n");
          digitalWrite(relayPin, HIGH);
        } else {
          Serial.print("[DECISION] Activating\n");
          digitalWrite(relayPin, LOW);
        }
        Serial.print("[DECISION] Alpha = ");
        Serial.print(dimming);
        Serial.print("\n");

      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        Serial.print("[DECISION] Desactivating\n");
        dimming = 128;
        digitalWrite(relayPin, HIGH);
      }
      http.end();
    } else {
      Serial.printf("[HTTP] Unable to connect\n");
      Serial.print("[DECISION] Desactivating\n");
      dimming = 128;
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
  // stop each hour to prevent failure
  if(millis() > timer_prevent_failure + 3600000) {
    dimming = 128;
    digitalWrite(relayPin, HIGH);
    timer_prevent_failure = millis();
  }
}
