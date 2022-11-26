/*
Connect RobotDyn AC Light Dimmer as follows:
Dimmer pin  --  Arduino pin
    VCC     --    5V
    GND     --    GND
    Z-C     --    D2
    PWM     --    D3

Glow sketch for AC Voltage dimmer with Zero cross detection
Based on the sketch by Charith Fernanado 
Adapted by RobotDyn: http://www.robotdyn.com
License: Creative Commons Attribution Share-Alike 3.0 License.
Attach the Zero cross pin of the module to Arduino External Interrupt pin
Select the correct Interrupt # from the below table (the Pin numbers are digital pins, NOT physical pins: digital pin 2 [INT0]=physical pin 4 and digital pin 3 [INT1]= physical pin 5)

Pin    |  Interrrupt # | Arduino Platform
---------------------------------------
2      |  0            |  All
3      |  1            |  All
18     |  5            |  Arduino Mega Only
19     |  4            |  Arduino Mega Only
20     |  3            |  Arduino Mega Only
21     |  2            |  Arduino Mega Only

In the program pin 2 is chosen
*/
int PSM = 05; // D1
int ZC = 04; // D2  
int dimming = 128;  // Dimming level (0-128)  0 = ON, 128 = OFF

void  ICACHE_RAM_ATTR zero_crosss_int(void);

void setup()
{
  pinMode(PSM, OUTPUT);// Set AC Load pin as output
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(ZC), zero_crosss_int, FALLING);  // Choose the zero cross interrupt # from the table above
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
  Serial.print("Trig\n");
}

void loop()  {
  for (int i=5; i <= 120; i++){  // We use 120 as the lowest and 5 as the highest brightness setting. May be adjusted depending on the lamp used.
    dimming=i;
    delay(20);
   }
  for (int i=120; i >= 5; i--){  // Same as above
    dimming=i;
    delay(20);
   }
   Serial.print("Loop\n");
}
