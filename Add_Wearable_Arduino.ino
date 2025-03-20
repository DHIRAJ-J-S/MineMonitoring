// Arduino UNO R4 WiFi code for MQ135 air quality sensor and MAX30102 pulse oximeter
// with Blynk interface display

// Blynk template information
#define BLYNK_TEMPLATE_ID "TMPL3DGTm3uaO"
#define BLYNK_TEMPLATE_NAME "mine monitor system"
#define BLYNK_AUTH_TOKEN "WuX87fztvwOFI11dUk6phsnsIah4-JDU"
#define BLYNK_PRINT Serial
// Pin definitions
#define MQ135_PIN A0  // Analog pin for MQ135 on Arduino UNO

// Include necessary libraries for Arduino UNO R4 WiFi
#include <WiFiS3.h>
#include <BlynkSimpleWiFiS3.h>

// Libraries for MAX30102 sensor
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// MQ135 library
#include "MQ135.h"

// WiFi credentials
const char ssid[] = "AURA";
const char pass[] = "qwertyuiop";



// Initialize MAX30102 sensor
MAX30105 particleSensor;

// Variables for MAX30102 data
uint32_t irBuffer[100];    // IR LED sensor data
uint32_t redBuffer[100];   // Red LED sensor data
int32_t bufferLength = 100; // Buffer length of 100 for SpO2 calculation
int32_t spo2;              // SPO2 value
int8_t validSPO2;          // Indicator to show if the SPO2 calculation is valid
int32_t heartRate;         // Heart rate value
int8_t validHeartRate;     // Indicator to show if the heart rate calculation is valid

// Timer for updating data
BlynkTimer timer;

// Initialize MQ135 sensor
MQ135 gasSensor = MQ135(MQ135_PIN);

void setup() {
  // Start serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("Initializing...");
  
  // Check WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("Connected!");
  
  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.macAddress(), ssid, pass);
  
  // Initialize I2C communication
  Wire.begin();
  
  // Initialize MAX30102 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    while (1);
  }
  
  // Configure MAX30102 sensor
  byte ledBrightness = 60;  // Options: 0=Off to 255=50mA
  byte sampleAverage = 4;   // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2;         // Options: 1=Red only, 2=Red+IR, 3=Red+IR+Green
  byte sampleRate = 100;    // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;     // Options: 69, 118, 215, 411
  int adcRange = 4096;      // Options: 2048, 4096, 8192, 16384
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  
  // Set timer to update data every 2 seconds
  timer.setInterval(2000L, sendSensorData);
  
  Serial.println("Initialization complete");
}

void loop() {
  Blynk.run();
  timer.run();
}

void sendSensorData() {
  // Read MQ135 data
  float ppm = gasSensor.getPPM();
  float correctedPPM = gasSensor.getCorrectedPPM(25.0, 50.0);  // Correct for temperature (25°C) and humidity (50%)
  float airQuality = map(correctedPPM, 0, 1000, 0, 100);  // Map PPM to a 0-100 scale for easier visualization
  
  // Read MAX30102 data
  readMAX30102();
  
  // Print data to serial monitor
  Serial.print("Air Quality (PPM): ");
  Serial.print(ppm);
  Serial.print(", Corrected PPM: ");
  Serial.print(correctedPPM);
  Serial.print(", Air Quality Index: ");
  Serial.println(airQuality);
  
  if (validHeartRate) {
    Serial.print("Heart Rate: ");
    Serial.print(heartRate);
    Serial.println(" BPM");
  } else {
    Serial.println("Heart Rate calculation not valid");
  }
  
  if (validSPO2) {
    Serial.print("SpO2: ");
    Serial.print(spo2);
    Serial.println("%");
  } else {
    Serial.println("SpO2 calculation not valid");
  }
  
  // Send data to Blynk
  Blynk.virtualWrite(V0, ppm);           // Raw PPM on virtual pin V0
  Blynk.virtualWrite(V1, correctedPPM);  // Corrected PPM on virtual pin V1
  Blynk.virtualWrite(V2, airQuality);    // Air quality index on virtual pin V2
  
  if (validHeartRate) {
    Blynk.virtualWrite(V3, heartRate);   // Heart rate on virtual pin V3
  }
  
  if (validSPO2) {
    Blynk.virtualWrite(V4, spo2);        // SpO2 on virtual pin V4
  }
}

void readMAX30102() {
  // Reset buffer counters
  bufferLength = 100;
  
  // Read the first 100 samples
  for (byte i = 0; i < bufferLength; i++) {
    while (!particleSensor.available())
      particleSensor.check(); // Check if new data is available
      
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); // Move to next sample
    
    // Check if finger is on the sensor
    if (irBuffer[i] < 50000) {
      Serial.println("Finger not detected");
      Blynk.virtualWrite(V5, "Place finger on sensor");
      return;
    }
  }
  
  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  
  if (validHeartRate && validSPO2) {
    Blynk.virtualWrite(V5, "Reading valid");
  }
}

// Blynk widget handlers for potential future interaction
BLYNK_WRITE(V6) {
  // Example of handling input from Blynk app
  int buttonState = param.asInt();
  if (buttonState == 1) {
    // Do something when button is pressed
    Serial.println("Button pressed in Blynk app");
  }
}