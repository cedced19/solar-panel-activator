#include <Arduino.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "config.h"

int PSM = 05; // D1
int ZC = 04; // D2  
int relayPin = 12; // D6 (14 corresponding to D5 on some devices) 
int dimming = 128;  // Dimming level (0-128)  0 = ON, 128 = OFF

unsigned long relay_transition_time = 0;
bool relay_transitioning = false;
int target_dimming = 128;
bool relay_target_state = HIGH;


void ICACHE_RAM_ATTR zero_crosss_int(void);

// WiFi Parameters
const char* ssid = SSID;
const char* password = PASSWORD;

// Timers
unsigned long timer = 0;
unsigned long timer_prevent_failure = 0;
unsigned long timer_temperature = 0;
unsigned long timer_wifi = 0;

unsigned long time_limit = 1000;

// T law
// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    

void initWiFi() {
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf("[WIFI] Connecting...\n");
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }
  Serial.printf("[WIFI] Connected\n");
  timer_wifi = millis();
}

void setup() {

  Serial.begin(115200);
  initWiFi();
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
  delayMicroseconds(100);         // triac On propogation delay (for 60Hz use 8.33)
  digitalWrite(PSM, LOW);    // triac Off
  //Serial.print("Trig\n");
}

void update() {
    WiFiClient client;
    HTTPClient http;
    Serial.print("[HTTP] begin...\n");
    if(http.begin(client, "http://192.168.0.87:8889/api/device/id/QQqR9/advanced/")) {
      int httpCode = http.GET();                                                   
      if (httpCode == 200 || httpCode == 304) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // Parsing
        StaticJsonDocument<200> root;
        
        DeserializationError error = deserializeJson(root, http.getString());

        // Test if parsing succeeds.
        if (error) {
          Serial.print("[JSON PARSER] deserializeJson() failed: ");
          Serial.println(error.f_str());
          Serial.print("[DECISION] Desactivating\n");
          safeDeactivate();
          return;
        }
        timer = millis();
        time_limit = root["time_limit"];
        int new_dimming = root["alpha"]; 
        if (new_dimming < 3 && new_dimming != 128) {
          new_dimming = 3;
        }
        
        // Handle state transitions with relay protection
        if (new_dimming == 128) {
          // Turning OFF
          Serial.print("[DECISION] Desactivating\n");
          safeDeactivate();
        } else {
          // Turning ON or changing brightness
          Serial.print("[DECISION] Activating\n");
          safeActivate(new_dimming);
        }
        Serial.print("[DECISION] Alpha = ");
        Serial.print(new_dimming);
        Serial.print("\n");

      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        Serial.print("[DECISION] Desactivating\n");
        safeDeactivate();
      }
      http.end();
    } else {
      Serial.printf("[HTTP] Unable to connect\n");
      Serial.print("[DECISION] Desactivating\n");
      safeDeactivate();
    }
}

// Safe deactivation: dimmer to 0 first, then relay off
void safeDeactivate() {
  if (dimming != 128) {
    // First reduce dimming to minimum
    dimming = 128;
    target_dimming = 128;
    
    // Start transition timer for relay
    relay_transitioning = true;
    relay_transition_time = millis();
    relay_target_state = HIGH;
    
    Serial.print("[RELAY PROTECTION] Dimmer set to 0, waiting before opening relay\n");
  } else {
    // Already off, just ensure relay is open
    digitalWrite(relayPin, HIGH);
    relay_transitioning = false;
  }
}

// Safe activation: relay on first, then ramp up dimmer
void safeActivate(int new_dimming) {
  target_dimming = new_dimming;
  
  if (digitalRead(relayPin) == HIGH) {
    // Relay is currently open, need to close it first
    Serial.print("[RELAY PROTECTION] Closing relay first\n");
    

    // Keep dimming at safe level until relay settles
    dimming = 128; // Minimum safe value

    digitalWrite(relayPin, LOW);
    
    // Start transition timer for dimmer
    relay_transitioning = true;
    relay_transition_time = millis();
    relay_target_state = LOW;
    

  } else {
    // Relay already closed, safe to change dimming directly
    dimming = new_dimming;
    relay_transitioning = false;
  }
}

void handleRelayTransition() {
  if (relay_transitioning) {
    unsigned long elapsed = millis() - relay_transition_time;
    
    // Wait 100ms for relay contacts to settle
    if (elapsed >= 100) {
      if (relay_target_state == HIGH) {
        // Complete deactivation sequence
        digitalWrite(relayPin, HIGH);
        Serial.print("[RELAY PROTECTION] Relay opened safely\n");
      } else {
        // Complete activation sequence
        dimming = target_dimming;
        Serial.print("[RELAY PROTECTION] Dimmer ramped up safely to ");
        Serial.print(dimming);
        Serial.print("\n");
      }
      relay_transitioning = false;
    }
  }
}


void getTemperature() {
  char params[100];
  char temperatureString[6];
  WiFiClient clientTemp;
  HTTPClient httpTemp;

  float measure;
  float temperature;

  measure = analogRead(THERMISTORPIN);

  //Serial.print("measure analog reading "); 
  //Serial.println(measure);
  
  // convert the value to resistance
  measure = 1023 / measure - 1;
  measure = SERIESRESISTOR / measure;
  //Serial.print("Thermistor resistance "); 
  //Serial.println(measure);
  
  float steinhart;
  steinhart = measure / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert

  temperature = steinhart - 273.15;  // Temperature in degree celsius
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" degree celsius");
  dtostrf(temperature, 4, 2, temperatureString);
  snprintf(params, 100, "var=%s", temperatureString);
  httpTemp.begin(clientTemp, "http://192.168.0.87:8889/api/device/id/QQqR9/control-variable/"); // specifie the address
  httpTemp.addHeader("Content-Type", "application/x-www-form-urlencoded");
  httpTemp.POST(params);    
  httpTemp.end();
  timer_temperature = millis();
}

void loop() {
  handleRelayTransition();
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    // Update alpha
    if(millis() > timer + time_limit) {
      update();
    }
    // Update temperature
    if(millis() > timer_temperature + 120000) { // each 2min = 120000ms
      getTemperature();
    }
  } else {
    // check WiFi Status and disconnect if necessary
    if (millis() > timer_wifi + 300000) { // each 5min if it fails
      Serial.println("[WIFI] Reconnecting to WiFi...");
      WiFi.disconnect();
      initWiFi();
    }
  }
  // stop each hour to prevent failure
  if(millis() > timer_prevent_failure + 3600000) {
    dimming = 128;
    digitalWrite(relayPin, HIGH);
    timer_prevent_failure = millis();
  }
}
