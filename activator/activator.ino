#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


// WiFi Parameters
const char* ssid = "ssid";
const char* password = "password";

// Timers
unsigned long timer = 0;

void update() {
    HTTPClient http;  
    http.begin("http://192.168.0.40:8889/api/device/chauffe-eau");
    int httpCode = http.GET();                                                        
    if (httpCode > 0) {
      // Parsing
      const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      if (root["toggle"] == "true") {

      } else {

      }
    } else {
      
    }
    http.end();
    
}

void setup() {
  WiFi.begin(ssid, password);
  // Check for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    digitalWrite(2, LOW);
    delay(1000);
  }
  digitalWrite(2, HIGH); 
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  // Load data
  Serial.begin(115200);
}

void loop() {
   if(millis() > timer + 120000){
        timer = millis();
        update();
   }
}