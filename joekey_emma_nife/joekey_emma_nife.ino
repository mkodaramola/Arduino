#define BLYNK_TEMPLATE_ID "TMPL2pPI__Cv3"
#define BLYNK_TEMPLATE_NAME "Irrigation System"
#define BLYNK_AUTH_TOKEN "fxHJgSBoSP63GKHbKDxM0e2zzhQYEt1-"  

#define BLYNK_PRINT Serial  // Enable debug prints

#include <WiFi.h>
#include <WiFiClient.h>

#include <BlynkSimpleEsp32.h>  // Correct Blynk library for ESP32
#include <Wire.h>
#include <RtcDS3231.h>
#include "DHT.h"

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials
char ssid[] = "Beware";
char pass[] = "Gbenga10";  

// Define DHT sensor pin and type
#define DHTPIN 18
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

RtcDS3231<TwoWire> Rtc(Wire);
int relay = 5;
String printDateTime();

unsigned long lastSentTime = 0;   // Tracks the last time data was sent
unsigned long interval = 2000;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Initialize Blynk

  // RTC Setup
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  if (!Rtc.IsDateTimeValid()) {
      if (!wasError()) {
          Rtc.SetDateTime(compiled);
      }
  }
  if (!Rtc.GetIsRunning()) {
      if (!wasError()) {
          Rtc.SetIsRunning(true);
      }
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (!wasError()) {
      if (now < compiled) {
          Rtc.SetDateTime(compiled);
      }
  }
  Rtc.Enable32kHzPin(false);
  wasError();
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  wasError();

  pinMode(relay,OUTPUT);

  dht.begin();  // Initialize DHT sensor
}

void loop() {
  Blynk.run();  // Run Blynk library
  
  // Send sensor data every interval
  if (millis() - lastSentTime >= interval) {
    float moist = analogRead(35);
    moist = map(moist,0,4095,100,0);
    moist *= 2;
//    moist = abs(moist);
    int threshold = analogRead(34);
    threshold = map(threshold,0,4095,40,80);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // Serial print of all variables
    Serial.print("Temperature: ");
    Serial.println(t);
    Serial.print("Humidity: ");
    Serial.println(h);
    Serial.print("Soil Moisture: ");
    Serial.println(moist);
    Serial.print("Threshold: ");
    Serial.println(threshold);
    

    // Send data to Blynk
    Blynk.virtualWrite(V1, t);         // Send temperature to Virtual Pin V1
    Blynk.virtualWrite(V2, h);         // Send humidity to Virtual Pin V2
    Blynk.virtualWrite(V3, moist);     // Send moisture to Virtual Pin V3
    Blynk.virtualWrite(V4, threshold); // Send threshold to Virtual Pin V4

    Serial.println("Data sent to Blynk");

     if (t >= 16 && t <= 32 && moist < threshold)
        digitalWrite(relay,HIGH);
   else 
        digitalWrite(relay,LOW);

    lastSentTime = millis();  // Update the last sent time
  }

 
    
}

// Check for RTC errors
bool wasError() {
  uint8_t error = Rtc.LastError();
  return error != 0;
}

// Print DateTime
String printDateTime() {
  RtcDateTime now = Rtc.GetDateTime();
  if (!wasError()) {
    char datestring[20];
    snprintf_P(datestring, sizeof(datestring), PSTR("%02u:%02u:%02u"), now.Hour(), now.Minute(), now.Second());
    return datestring;
  }
  return "";
}
