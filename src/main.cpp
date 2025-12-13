#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Preferences.h>

#include "MQTTManager.h"
#include "Storage.h"
#include "WeightSensor.h"
#include "Dispenser.h"

// -----------------------------
// Pin configuration
// -----------------------------
constexpr int trig = 2;
constexpr int echo = 4;
constexpr int dt = 18;
constexpr int sck = 5;
constexpr int SERVO_PIN = 19;

// -----------------------------
// WiFi configuration
// -----------------------------
WiFiClientSecure espClient;

// -----------------------------
// MQTT configuration
// -----------------------------
constexpr char mqttHost[] = "a63c6d5a32cf4a67b9d6a209a8e13525.s1.eu.hivemq.cloud";
constexpr int mqttPort = 8883;
constexpr char mqttUsername[] = "tmitmi";
constexpr char mqttPassword[] = "Tm123456";
PubSubClient client(espClient);

// -----------------------------
// Global instances
// -----------------------------
MQTTManager mqttManager(client);
Storage storage(2.0f, 20.0f, 2000.0f);
WeightSensor weightSensor;
Dispenser dispenser(SERVO_PIN);

// -----------------------------
// Feeder ID
// -----------------------------

Preferences prefs;
String feederId;

String generateFeederId() {
    uint64_t mac = ESP.getEfuseMac();
    uint32_t id = (uint32_t)(mac & 0xFFFFFF); // lower 24 bits
    char buf[7];
    snprintf(buf, sizeof(buf), "%06X", id);
    return String(buf);
}

String loadOrCreateFeederId() {
    prefs.begin("feeder", false);
    String id = prefs.getString("id", "");

    if (id.length() == 0) {
        id = generateFeederId();
        prefs.putString("id", id);
    }

    prefs.end();
    return id;
}

// -----------------------------
// Wi-Fi setup
// -----------------------------

bool startWifi() {
    Serial.println("Starting Wi-Fi priority sequence...");

    WiFi.mode(WIFI_STA);

    // --- 1. Try Wokwi-GUEST first ---
    Serial.print("Trying Wokwi-GUEST...");
    WiFi.begin("Wokwi-GUEST");
    unsigned long start = millis();
    unsigned long lastPrint = start;
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - start >= 5000) break;   // 5s timeout
        if (now - lastPrint >= 500) {
            Serial.print(".");
            lastPrint = now;
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wokwi-GUEST");
        return true;
    }

    // --- 2. Try saved credentials ---
    Serial.print("\nTrying saved Wi-Fi credentials...");
    WiFi.begin(); // attempt stored SSID/password
    start = millis();
    lastPrint = start;
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - start >= 8000) break;   // 8s timeout
        if (now - lastPrint >= 500) {
            Serial.print(".");
            lastPrint = now;
        }
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to saved Wi-Fi");
        return true;
    }

    // --- 3. Fall back to WiFiManager portal ---
    Serial.println("\nSaved credentials failed, starting config portal...");
    WiFiManager wm;
    wm.setConfigPortalTimeout(60);
    wm.setDebugOutput(false);

    String suffix = feederId.length() > 6 ? feederId.substring(feederId.length() - 6) : feederId;
    String apName = "AutoFeeder-" + suffix;

    if (!wm.autoConnect(apName.c_str(), "connectNOWBRO")) {
        Serial.println("WiFiManager portal failed");
        return false;
    }

    Serial.println("Connected via WiFiManager portal");
    return true;
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
    Serial.begin(9600);
    Serial.println("setting up...");

    feederId = loadOrCreateFeederId();

    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);

    Serial.println("feeder id: " + feederId);
    
    weightSensor.setup(dt, sck);
    dispenser.setup();
    
    if (!startWifi()) {
        Serial.println("restarting...");
        ESP.restart();
    }
    
    espClient.setInsecure();
    
    // Initialize MQTT system
    Serial.println("setting up mqtt...");
    mqttManager.setup(
        mqttHost, mqttPort, mqttUsername, mqttPassword,
        feederId.c_str(),
        feederId
    );
    Serial.println("done");
}

// -----------------------------
// Main loop
// -----------------------------
void loop() {
    // Update MQTT
    mqttManager.loop();

    // Update dispenser
    dispenser.loop();

    // Basic monitoring
    if (weightSensor.isReady()) {
        Serial.print("Weight (g): ");
        Serial.println(weightSensor.getSmoothedWeight());
    }

    // mqttManager.publish("event", "fed");
}
