#include <Adafruit_MPU6050.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <SPI.h>

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;  
HardwareSerial hardwareSerial(1);  // GPS Serial

// Pin Configurations
const int MPUIndicator = 12, WIFIIndicator = 13, GPSIndicator = 33, PowerIndicator = 32, buzzer = 27;
const byte VIBPin = 5, GPSRX = 16, GPSTX = 17, SD_CS = 14;


// WiFi and Server Details
 const char* ssid = "MTN_4G_325845"; //choose your wireless ssid
const char* password =  "CF9675A7"; //put your wireless password here.
const char* serverURL = "https://anomaly-detection-9fml.onrender.com/log";
const char* rootCACertificate= \
  "-----BEGIN CERTIFICATE-----\n" \
"MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n" \
"VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n" \
"A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n" \
"WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n" \
"IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n" \
"AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n" \
"QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n" \
"HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n" \
"BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n" \
"9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n" \
"p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n" \
"-----END CERTIFICATE-----\n";

// Objects for secure HTTP and SD Client
WiFiClientSecure client;
HTTPClient https;

void setup() {
  Serial.begin(115200);
  hardwareSerial.begin(9600, SERIAL_8N1, GPSRX, GPSTX);
  pinMode(MPUIndicator, OUTPUT); 
  pinMode(GPSIndicator, OUTPUT);
  pinMode(WIFIIndicator, OUTPUT);
  pinMode(PowerIndicator, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(VIBPin, INPUT);

  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);

  connectWiFi();
  setupMPU();
//  if (!SD.begin(SD_CS)) {
//    Serial.println("SD card initialization failed!");
//    while (1);
//  }

  
  Serial.println("Setup complete");
  digitalWrite(PowerIndicator, HIGH);
}

void loop() {
  digitalWrite(PowerIndicator, HIGH);
  String data = getData();
  if (!sendData(data)) {
    //saveToSD(data);
    Serial.println("No SD Card");
  }
  delay(500);
}

// Gather data from sensors
String getData() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  String data = String(a.acceleration.x) + "," + String(a.acceleration.y) + "," + String(a.acceleration.z) + ",";
  data += String(g.gyro.x) + "," + String(g.gyro.y) + "," + String(g.gyro.z) + ",";
  data += String(temp.temperature) + "," + String(!digitalRead(VIBPin)) + ",";

  if (!digitalRead(VIBPin) == 0)Serial.println("Vibration Detected");

  if (hardwareSerial.available() > 0) gps.encode(hardwareSerial.read());

  if (gps.location.isValid()) {
    data += String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6) + ",";
    digitalWrite(GPSIndicator, HIGH);
  } else {
    data += "N/A,N/A,";
    digitalWrite(GPSIndicator, LOW);
  }

  if (gps.date.isValid() && gps.time.isValid()) {
    data += String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day()) + " ";
    data += String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
    digitalWrite(GPSIndicator, HIGH);
  } else {
    data += "N/A";
    digitalWrite(GPSIndicator, LOW);
  }

  Serial.println("Data: " + data);
  return data;
}

// Send data via HTTPS
bool sendData(String data) {
  if (WiFi.status() != WL_CONNECTED) return false;
  digitalWrite(WIFIIndicator, HIGH);

  client.setCACert(rootCACertificate);
  https.begin(client, serverURL);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int responseCode = https.POST("data=" + data);

  if (responseCode > 0) {
    Serial.println("Response Received");
    https.end();
    return true;
  } else {
    Serial.println("Failed to send, response: " + String(responseCode));
    https.end();
    return false;
  }
}

// Backup data to SD card if unable to send
void saveToSD(String data) {
  File file = SD.open("/data_backup.csv", FILE_APPEND);
  if (file) {
    file.println(data);
    file.close();
    Serial.println("Data saved to SD card.");
  } else {
    Serial.println("Error writing to SD card.");
  }
}

// WiFi Connection Setup
void connectWiFi() {
  WiFi.begin(ssid, password);
  int retryCount = 0; 

  while (WiFi.status() != WL_CONNECTED && retryCount < 5) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(WIFIIndicator, HIGH);
  } else {
    Serial.println("WiFi connection failed");
    digitalWrite(WIFIIndicator, LOW);
  }
}

// MPU6050 Sensor Initialization
void setupMPU() {
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    digitalWrite(MPUIndicator, LOW);
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  Serial.println("MPU6050 initialized");
  digitalWrite(MPUIndicator, HIGH);
}
 
