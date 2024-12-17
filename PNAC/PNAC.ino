#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"   // Disable brownout problems
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "Reborn";   
const char* password = "Godisgood11";

// Firebase credentials
const char* firebaseURL = "https://intrudercapture-default-rtdb.firebaseio.com/Image.json";

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

camera_config_t config;

// Base64 encoding characters
const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Function to encode data to Base64
String base64_encode(byte *data, unsigned int len) {
  String encoded_string = "";
  byte temp[3];
  int padding = 0;

  for (unsigned int i = 0; i < len; i += 3) {
    temp[0] = data[i];
    temp[1] = (i + 1 < len) ? data[i + 1] : 0;
    temp[2] = (i + 2 < len) ? data[i + 2] : 0;

    if (i + 1 >= len) padding = 2;
    else if (i + 2 >= len) padding = 1;

    encoded_string += base64_chars[(temp[0] >> 2) & 0x3F];
    encoded_string += base64_chars[((temp[0] & 0x03) << 4) | ((temp[1] >> 4) & 0x0F)];
    encoded_string += (padding >= 2) ? '=' : base64_chars[((temp[1] & 0x0F) << 2) | ((temp[2] >> 6) & 0x03)];
    encoded_string += (padding >= 1) ? '=' : base64_chars[temp[2] & 0x3F];
  }

  return encoded_string;
}

// Function to send string to Firebase
void sendStringToFirebase(const String& data) {
  // Connect to Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting...");
    }
    Serial.println("Wi-Fi connected!");
  }

  // Make an HTTP request
  HTTPClient http;
  http.begin(firebaseURL); // Initialize the HTTP client with Firebase URL
  http.addHeader("Content-Type", "application/json"); // Set header for JSON data

  // JSON payload
  String payload = "{\"SavedImage\":\"" + data + "\"}";

  // Send POST request
  int httpResponseCode = http.PUT(payload); // Use PUT to update the database entry
  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.printf("Error code: %d\n", httpResponseCode);
  }

  // End HTTP request
  http.end();
}

void setup() {
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector

  Serial.begin(115200);
  WiFi.begin(ssid, password);  // Connect to Wi-Fi network

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; 
    config.jpeg_quality = 15;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Turns on the ESP32-CAM on-board LED (flashlight) before taking a picture
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(500); // Flashlight delay to allow lighting

  // Take Picture with Camera
  camera_fb_t * fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(4, LOW); // Turn off flashlight
    return;
  }

  // Encode the captured image to Base64
  String base64_image = base64_encode(fb->buf, fb->len);
  //base64_image = "Amen";
  Serial.println("Base64 Encoded Image:");
  Serial.println(base64_image);

  // Update Firebase
  sendStringToFirebase(String(base64_image));
 
  esp_camera_fb_return(fb); 
  digitalWrite(4, LOW); // Turn off flashlight

  Serial.println("Going to sleep now");
  delay(2000);
  esp_deep_sleep_start(); 
}

void loop() {
  // Nothing in loop
}
