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
// Distance Measurement
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
// Setup & Loop
//-----------------------------------------

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

  setupWifiManager();
}

void loop() {
  // Print distance info
  foodAmount();

  // Check if HX711 has data
  if (scale.is_ready()) {
    long reading = scale.read();    // raw units
    Serial.print("Raw weight reading: ");
    Serial.println(reading);
  } else {
    Serial.println("HX711 not ready");
  }

  Serial.println("------------------------");
  delay(500);
}
