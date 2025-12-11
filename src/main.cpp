#include <Arduino.h>

#include <HX711.h>
#include <WiFi.h>
#include <WiFiManager.h>

#define TRIG_PIN 2
#define ECHO_PIN 4

// HX711 pins
#define HX_DOUT 18   // DT pin
#define HX_SCK 5     // SCK pin

HX711 scale;

//-----------------------------------------
// DISTANCE MEASUREMENT
//-----------------------------------------
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.0343 / 2;

  return distance;
}

void foodAmount() {
  if (getDistance() >= 35) {
    Serial.println("Running out of food!");
  } else {
    Serial.println("OKAY");
  }
}


//-----------------------------------------
// WEIGHT MEASUREMENT (GRAMS)
//-----------------------------------------

long zeroOffset = 0;
float calibrationFactor = 0.42;

float targetPortionGrams = 0;  // User-defined full amount

// For smoothing
float smoothedWeight = 0;  
float alpha = 0.2;          // smoothing factor (0 < alpha <= 1)

// -------------------- Raw Readings --------------------
long readRaw() {
  return scale.read();
}

float getWeightGrams() {
  long raw = readRaw();
  long adjusted = raw - zeroOffset;
  float grams = adjusted / calibrationFactor;
  return grams;
}

// -------------------- Tare / Zero --------------------
void tareScale() {
  long sum = 0;
  const int samples = 20;

  for (int i = 0; i < samples; i++) {
    sum += readRaw();
    delay(10);
  }

  zeroOffset = sum / samples;

  // Initialize smoothedWeight after taring
  smoothedWeight = getWeightGrams();
}

// -------------------- Portion Setting --------------------
void setUserPortion() {
  targetPortionGrams = getWeightGrams();

  // Optional: reset smoothing
  smoothedWeight = targetPortionGrams;
}

// -------------------- Smoothed Weight --------------------
float getSmoothedWeight() {
  float current = getWeightGrams();
  smoothedWeight = alpha * current + (1 - alpha) * smoothedWeight;
  return smoothedWeight;
}

// -------------------- Portion Check --------------------
bool portionReached() {
  float current = getSmoothedWeight();   // use smoothed weight
  return current >= targetPortionGrams;
}

// -------------------- TESTING WEIGHT SENSOR --------------------
float readPortionFromSerial() {
  float value = 0;
  
  // Wait until data is available
  while (Serial.available() == 0) {
    // do nothing
  }

  // Read the number
  value = Serial.parseFloat();

  // Clear any leftover characters
  Serial.readStringUntil('\n');

  return value;
}

void setTargetPortionFromUser() {
  Serial.println("Enter desired portion in grams:");
  float inputGrams = readPortionFromSerial();
  targetPortionGrams = inputGrams;

  // Optional: initialize smoothing
  smoothedWeight = targetPortionGrams;

  Serial.print("Target portion set to: ");
  Serial.println(targetPortionGrams);
}

//-----------------------------------------
// Setup & Loop
//-----------------------------------------

void startWokwiWifi() {
  WiFi.begin("Wokwi-GUEST", "");
  Serial.print("wifi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(600);
    Serial.print(".");
  }
  Serial.println("\nwifi connected");
}

void setupWifiManager() {
  WiFiManager wifiManager;

  if (!wifiManager.autoConnect("autofeeder", "connectNOWBRO")) {
    Serial.println("wifi failed, rebooting...");
    ESP.restart();
    delay(1306);
  }

  Serial.println("wifi connected");
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  scale.begin(HX_DOUT, HX_SCK);
  // setupWifiManager();
  startWokwiWifi();

  Serial.println("Taring scale...");
  tareScale();
  Serial.print("Zero offset = ");
  Serial.println(zeroOffset);
  // Ask user to input target portion
  setTargetPortionFromUser();
}

void loop() {
  // Distance output
  foodAmount();

  // Weight output
  if (scale.is_ready()) {
    // Read raw weight
    long raw = readRaw();
    Serial.print("Raw reading: ");
    Serial.println(raw);

    // Smoothed weight
    float grams = getSmoothedWeight();
    Serial.print("Smoothed weight (g): ");
    Serial.println(grams);

    // Portion reached (if target portion is set)
    if (targetPortionGrams > 0) {
      Serial.print("Target portion (g): ");
      Serial.println(targetPortionGrams);

      if (portionReached()) {
        Serial.println("Portion reached!");
      } else {
        Serial.println("Portion not yet reached.");
      }
    }
  } else {
    Serial.println("HX711 not ready");
  }

  Serial.println("------------------------");
  delay(500);
}
