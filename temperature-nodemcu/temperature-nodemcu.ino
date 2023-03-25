// https://www.electronicwings.com/nodemcu/thermistor-interfacing-with-nodemcu

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

void setup() {
  Serial.begin(9600);  /* Define baud rate for serial communication */
}

void loop() {
  float measure;
  float temperature;

  measure =  analogRead(THERMISTORPIN);

  Serial.print("measure analog reading "); 
  Serial.println(measure);
  
  // convert the value to resistance
  measure = 1023 / measure - 1;
  measure = SERIESRESISTOR / measure;
  Serial.print("Thermistor resistance "); 
  Serial.println(measure);
  
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
  delay(500);
}
