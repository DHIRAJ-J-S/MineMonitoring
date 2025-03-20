// Mine Monitoring System using ESP32 and Blynk with Buzzer for alerts
// Uses DHT22, Flame Sensor, MQ2 Gas Sensor, Vibration Sensor, and Buzzer

// Define Blynk template credentials from the image
#define BLYNK_TEMPLATE_ID "TMPL3DGTm3uaO"
#define BLYNK_TEMPLATE_NAME "mine monitor system"
#define BLYNK_AUTH_TOKEN "WuX87fztvwOFI11dUk6phsnsIah4-JDU"

// Define the Blynk debug output level
#define BLYNK_PRINT Serial


// Define pins for sensors
#define DHTPIN 4          // DHT22 data pin
#define DHTTYPE DHT22     // DHT sensor type
#define FLAME_PIN 34      // Flame sensor analog pin
#define MQ2_PIN 35        // MQ2 Gas sensor analog pin
#define VIBRATION_PIN 32  // Vibration sensor digital pin
#define BUZZER_PIN 25     // Buzzer pin

// Define threshold values for alerts
#define TEMP_THRESHOLD 40.0      // Temperature threshold in Celsius
#define HUMIDITY_THRESHOLD 85.0  // Humidity threshold in percentage
#define GAS_THRESHOLD 700        // MQ2 gas threshold value
#define FLAME_THRESHOLD 300      // Flame sensor threshold (lower value means flame detected)

// Define severe threshold values for buzzer activation
#define SEVERE_TEMP_THRESHOLD 45.0      // Severe temperature threshold
#define SEVERE_GAS_THRESHOLD 900        // Severe gas threshold
#define SEVERE_FLAME_THRESHOLD 100      // Severe flame threshold (lower = more intense)


// Include required libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// WiFi credentials - replace with your network details
char ssid[] = "AURA";
char pass[] = "qwertyuiop";

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables to store sensor values
float temperature = 0;
float humidity = 0;
int gasValue = 0;
int flameValue = 0;
bool vibrationDetected = false;
bool buzzerActive = false;

// Timer for sending data to Blynk
BlynkTimer timer;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Mine Monitoring System Starting...");

  // Initialize sensors
  pinMode(FLAME_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(VIBRATION_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off on startup
  dht.begin();

  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Setup a function to be called every 2 seconds
  timer.setInterval(2000L, sendSensorData);
  
  // Setup a function to handle buzzer alerts
  timer.setInterval(500L, buzzerControl);
}

void loop() {
  Blynk.run();
  timer.run();
}

// Function to read all sensors and send data to Blynk
void sendSensorData() {
  // Read temperature and humidity
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // Check if any reads failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Read gas sensor
  gasValue = analogRead(MQ2_PIN);
  
  // Read flame sensor
  flameValue = analogRead(FLAME_PIN);
  
  // Read vibration sensor
  vibrationDetected = digitalRead(VIBRATION_PIN);

  // Print values to Serial Monitor
  Serial.println("Sensor Readings:");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  
  Serial.print("Gas Value: ");
  Serial.println(gasValue);
  
  Serial.print("Flame Value: ");
  Serial.println(flameValue);
  
  Serial.print("Vibration: ");
  Serial.println(vibrationDetected ? "Detected" : "Not Detected");
  
  Serial.print("Buzzer: ");
  Serial.println(buzzerActive ? "ON" : "OFF");
  Serial.println();

  // Send data to Blynk virtual pins
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, gasValue);
  Blynk.virtualWrite(V3, flameValue);
  Blynk.virtualWrite(V4, vibrationDetected);
  Blynk.virtualWrite(V6, buzzerActive); // Send buzzer status to Blynk

  // Check for alert conditions
  checkAlerts();
}

// Function to check alert conditions and notify
void checkAlerts() {
  bool severeSituation = false;
  
  // Temperature alert
  if (temperature > TEMP_THRESHOLD) {
    Blynk.logEvent("temp_alert", String("High Temperature: ") + temperature + "°C");
    Serial.println("ALERT: High temperature detected!");
    
    // Check for severe condition
    if (temperature > SEVERE_TEMP_THRESHOLD) {
      severeSituation = true;
    }
  }
  
  // Humidity alert
  if (humidity > HUMIDITY_THRESHOLD) {
    Blynk.logEvent("humidity_alert", String("High Humidity: ") + humidity + "%");
    Serial.println("ALERT: High humidity detected!");
  }
  
  // Gas alert
  if (gasValue > GAS_THRESHOLD) {
    Blynk.logEvent("gas_alert", String("Gas Detected: ") + gasValue);
    Serial.println("ALERT: Gas detected!");
    
    // Check for severe condition
    if (gasValue > SEVERE_GAS_THRESHOLD) {
      severeSituation = true;
    }
  }
  
  // Flame alert
  if (flameValue < FLAME_THRESHOLD) {
    Blynk.logEvent("flame_alert", "Flame Detected!");
    Serial.println("ALERT: Flame detected!");
    
    // Check for severe condition
    if (flameValue < SEVERE_FLAME_THRESHOLD) {
      severeSituation = true;
    }
  }
  
  // Vibration alert - consider any vibration as potentially severe
  if (vibrationDetected) {
    Blynk.logEvent("vibration_alert", "Vibration Detected!");
    Serial.println("ALERT: Vibration detected!");
    severeSituation = true;
  }
  
  // Activate buzzer if situation is severe
  buzzerActive = severeSituation;
  
  // Send emergency alert for severe situations
  if (severeSituation && !buzzerActive) {
    // First time severe situation detected
    Blynk.logEvent("emergency_alert", "EMERGENCY: Critical conditions in mine!");
  }
}

// Function to control the buzzer
void buzzerControl() {
  static bool buzzerState = false;
  static unsigned long lastBuzzerToggle = 0;
  
  if (buzzerActive) {
    // Toggle buzzer state for alarm sound (beeping)
    unsigned long currentMillis = millis();
    if (currentMillis - lastBuzzerToggle > 300) {  // Toggle every 300ms for urgent beeping
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
      lastBuzzerToggle = currentMillis;
    }
  } else {
    // Make sure buzzer is off when not active
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }
}

// Blynk functions for notifications and control
BLYNK_WRITE(V5) {
  // This function will be called when App button on V5 is pressed
  int buttonState = param.asInt();
  if (buttonState == 1) {
    // Reset alerts and turn off buzzer
    buzzerActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Alert reset requested - Buzzer deactivated");
    Blynk.virtualWrite(V6, buzzerActive); // Update buzzer status on Blynk
  }
}