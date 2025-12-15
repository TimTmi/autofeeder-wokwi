#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Preferences.h>

#include "MQTTManager.h"
#include "Storage.h"
#include "WeightSensor.h"
#include "Dispenser.h"
#include "Button.h"

// -----------------------------
// Pin configuration
// -----------------------------
constexpr int trig = 2;
constexpr int echo = 4;
constexpr int dt = 18;
constexpr int sck = 5;
constexpr int SERVO_PIN = 19;
constexpr int BUTTON_PIN = 21;
constexpr int LED_PIN = 17;

// -----------------------------
// WiFi configuration
// -----------------------------
WiFiClientSecure espClient;

// -----------------------------
// MQTT configuration
// -----------------------------
constexpr char mqttHost[] = "a63c6d5a32cf4a67b9d6a209a8e13525.s1.eu.hivemq.cloud";
constexpr int mqttPort = 8883; //used for ESP
constexpr char mqttUsername[] = "tmitmi";
constexpr char mqttPassword[] = "Tm123456";
PubSubClient client(espClient);

// -----------------------------
// Global instances
// -----------------------------
MQTTManager mqttManager(client);
Storage storage(2.0f, 30.0f, 2000.0f);  // minDist=2cm (full), maxDist=30cm (empty), maxWeight=2000g
WeightSensor weightSensor;
Dispenser dispenser(SERVO_PIN);
Button feedButton(21, true);  // active HIGH with pulldown

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
// Ultrasonic sensor reading
// -----------------------------
float readUltrasonicDistance() {
    // Clear trigger pin
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    
    // Send 10μs HIGH pulse to trigger
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    // Read echo pulse duration
    long duration = pulseIn(echo, HIGH, 30000);  // Timeout after 30ms (max ~5m range)
    
    if (duration == 0) {
        // Timeout or no echo - return last known good value or max distance
        return 50.0f;  // Return maxDist if sensor fails
    }
    
    // Calculate distance in cm
    // Speed of sound = 340 m/s = 0.034 cm/μs
    // Distance = (duration * speed) / 2 (divide by 2 because sound travels there and back)
    float distance = (duration * 0.034) / 2.0f;
    
    return distance;
}


// -----------------------------
// Activate feeding servo
// -----------------------------
void feedToTargetPortion() {
    if (!weightSensor.isReady()) { 
        Serial.println("Weight sensor not ready."); 
        return; 
    } 
    float current = weightSensor.getWeight();
    float target  = weightSensor.getTargetPortion();

    if (target <= 0) {
        mqttManager.publish("feed_rejected", "target_not_set", false);
        return;
    }

    if (current >= target) {
        mqttManager.publish("feed_skipped", "already_sufficient", false);
        return;
    }

    if (storage.isEmpty()) {
        mqttManager.publish("feed_rejected", "storage_empty", false);
        return;
    }

    if (dispenser.getState() != Dispenser::IDLE) {
        mqttManager.publish("feed_rejected", "dispenser_busy", false);
        return;
    }

    bool ok = dispenser.dispenseToPortion(
        target,
        [](){ return weightSensor.getWeight(); },
        [](){ return storage.getEstimatedWeight(); }
    );

    if (ok) {
        mqttManager.publish("feed_started", String(target), false);
    } else {
        mqttManager.publish("feed_failed", "start_failed", false);
    }
}

// -----------------------------
// Serial command handler
// -----------------------------
void handleSerialCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "feed") {
            // // Trigger dispense to target portion
            // float target = weightSensor.getTargetPortion();
            // float current = weightSensor.getWeight();
            
            // Serial.print("Current weight: ");
            // Serial.print(current);
            // Serial.print("g, Target: ");
            // Serial.print(target);
            // Serial.println("g");
            
            // if (dispenser.dispenseToPortion(
            //     target,
            //     [](){ return weightSensor.getWeight(); },
            //     [](){ return storage.getEstimatedWeight(); }
            // )) {
            //     Serial.println("Dispensing started!");
            // } else {
            //     if (current >= target) {
            //         Serial.println("Already at or above target portion. No dispensing needed.");
            //     } else {
            //         Serial.println("Cannot start dispensing (dispenser busy or error).");
            //     }
            // }
            feedToTargetPortion();
        }
        else if (command == "status") {
            // Show current status
            Serial.println("=== Status ===");
            Serial.print("Current weight: ");
            Serial.print(weightSensor.getWeight());
            Serial.println("g");
            Serial.print("Target portion: ");
            Serial.print(weightSensor.getTargetPortion());
            Serial.println("g");
            
            float remaining = weightSensor.getTargetPortion() - weightSensor.getWeight();
            if (remaining > 0) {
                Serial.print("Remaining needed: ");
                Serial.print(remaining);
                Serial.println("g");
            } else {
                Serial.println("Target portion reached or exceeded.");
            }
            
            // Storage information
            float distance = readUltrasonicDistance();
            Serial.print("Storage distance: ");
            Serial.print(distance);
            Serial.print("cm | ");
            Serial.print("Storage: ");
            Serial.print(storage.getRemainingPercent());
            Serial.print("% (");
            Serial.print(storage.getEstimatedWeight());
            Serial.println("g estimated)");
            
            Serial.print("Dispenser state: ");
            switch (dispenser.getState()) {
                case Dispenser::IDLE:
                    Serial.println("IDLE");
                    break;
                case Dispenser::DISPENSING:
                    Serial.println("DISPENSING");
                    break;
                case Dispenser::DONE:
                    Serial.println("DONE");
                    break;
                case Dispenser::ERROR:
                    Serial.println("ERROR");
                    break;
            }
            Serial.println("==============");
        }
        else if (command == "tare") {
            // Re-tare the scale
            Serial.println("Taring scale...");
            weightSensor.tareScale();
            Serial.println("Scale tared.");
        }
        else if (command.startsWith("set ")) {
            // Set target portion: "set 500"
            float newTarget = command.substring(4).toFloat();
            if (newTarget > 0) {
                weightSensor.setTargetPortion(newTarget);
                Serial.print("Target portion set to: ");
                Serial.print(newTarget);
                Serial.println("g");
            } else {
                Serial.println("Invalid target value. Usage: set 500");
            }
        }
        else if (command == "help" || command == "?") {
            // Show help
            Serial.println("=== Available Commands ===");
            Serial.println("feed        - Dispense to target portion");
            Serial.println("status      - Show current status");
            Serial.println("tare        - Re-tare the scale");
            Serial.println("set <value> - Set target portion (e.g., set 500)");
            Serial.println("help        - Show this help");
            Serial.println("========================");
        }
        else if (command.length() > 0) {
            Serial.print("Unknown command: ");
            Serial.println(command);
            Serial.println("Type 'help' for available commands.");
        }
    }
}

// -----------------------------
// Setup
// -----------------------------
void setup() {
    Serial.begin(9600);
    Serial.println("setting up...");

    feederId = loadOrCreateFeederId();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);

    Serial.println("feeder id: " + feederId);
    
    weightSensor.setup(dt, sck);
    weightSensor.setTargetPortionFromUser();

    feedButton.setup();

    dispenser.setup();
    
    // Initialize storage to 100% full for testing (prevents ERROR state)
    // Will be updated live by ultrasonic sensor in loop()
    storage.update(2.0f);  // 2cm = minDist = full container
    
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

    weightSensor.tareScale();
}

// -----------------------------
// Main loop
// -----------------------------
void loop() {
    // Update MQTT
    mqttManager.loop();

    // Read ultrasonic sensor and update storage
    float distance = readUltrasonicDistance();
    storage.update(distance);

    // -------------------- Storage status LED --------------------
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (storage.isEmpty()) {
        // EMPTY → LED solid ON
        digitalWrite(LED_PIN, HIGH);
    }
    else if (storage.isLow()) {
        // LOW → LED blinking
        unsigned long now = millis();
        if (now - lastBlink >= 500) {   // 500 ms blink rate
            lastBlink = now;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        }
    }
    else {
        // OK → LED OFF
        digitalWrite(LED_PIN, LOW);
        ledState = false;
    }

    // Feed button 
    feedButton.loop();

    if (feedButton.wasPressed()) {
        Serial.println("[BUTTON] Feed requested");
        feedToTargetPortion();
    }

    // Update dispenser
    dispenser.loop();

    // if (dispenser.getState() == Dispenser::State::DONE) {
    //     mqttManager.publish("fed", String(dispenser.getTargetWeight()));
    //     dispenser.setState(Dispenser::State::IDLE);
    // }

    static Dispenser::State lastDispState = Dispenser::IDLE;
    Dispenser::State current = dispenser.getState();

    if (current != lastDispState) {
        lastDispState = current;

        if (current == Dispenser::DONE) {
            mqttManager.publish("feed_completed", "Feed Success", false);
        }
        else if (current == Dispenser::ERROR) {
            mqttManager.publish("feed_failed", "Feed Failed", false);
        }
    }

    // Handle serial commands
    handleSerialCommands();

    // Basic monitoring (reduced frequency - only print every 2 seconds)
    static unsigned long lastPrint = 0;
    unsigned long now = millis();
    // if (now - lastPrint >= 500) {  // Print every 2 seconds instead of every loop
    //     lastPrint = now;
    //     if (weightSensor.isReady()) {
    //         Serial.print("Weight (g): ");
    //         Serial.print(weightSensor.getWeight());
    //         Serial.print(" / Target: ");
    //         Serial.print(weightSensor.getTargetPortion());
    //         Serial.print("g | State: ");
    //         switch (dispenser.getState()) {
    //             case Dispenser::IDLE: Serial.print("IDLE"); break;
    //             case Dispenser::DISPENSING: Serial.print("DISPENSING"); break;
    //             case Dispenser::DONE: Serial.print("DONE"); break;
    //             case Dispenser::ERROR: Serial.print("ERROR"); break;
    //         }
    //         Serial.println();
    //     }
    // }

    // -------------------- Periodic status publishing --------------------
    static unsigned long lastStatusPublish = 0;

    if (now - lastStatusPublish >= 5000) {
        lastStatusPublish = now;

        // --- Bowl status ---
        float bowlWeight = weightSensor.getWeight();
        float targetPortion = weightSensor.getTargetPortion();

        // --- Storage status ---
        float storagePercent = storage.getRemainingPercent();
        float storageWeight  = storage.getEstimatedWeight();
        String storageState = "OK";
        if (storage.isEmpty()) storageState = "EMPTY";
        else if (storage.isLow()) storageState = "LOW";

        // --- Publish individual topics ---
        mqttManager.publish("bowl", String(bowlWeight), false);
        mqttManager.publish("bowl_target_portion", String(targetPortion), false);

        mqttManager.publish("storage_percent", String(storagePercent, 1), false);
        mqttManager.publish("storage_currentWeight", String((int)storageWeight), false);
        mqttManager.publish("storage_state", storageState, false);

        // Optional: Debug output
        Serial.print("[MQTT] Bowl: "); 
        Serial.print(bowlWeight); 
        Serial.print("g, "); 
        Serial.print("Target portion: "); 
        Serial.print(targetPortion); 
        Serial.println("g");

        Serial.print("[MQTT] Storage: "); Serial.print(storagePercent); Serial.print("%, "); Serial.print(storageWeight); Serial.print("g, "); Serial.println(storageState);
    }


    // Serial.print(digitalRead(22));
    // delay(300);
}
