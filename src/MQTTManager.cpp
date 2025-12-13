#include "MQTTManager.h"
#include "Dispenser.h"
#include "WeightSensor.h"
#include "Storage.h"

extern Dispenser dispenser;
extern WeightSensor weightSensor;
extern Storage storage;
extern void feedToTargetPortion();

MQTTManager::MQTTManager(PubSubClient& c): client(c) {}

void MQTTManager::setup(
    const char* host, int port,
    const char* user, const char* pass,
    const char* id,
    const String& feeder
) {
    client.setServer(host, port);
    username = user;
    password = pass;
    clientId = id;
    feederId = feeder;

    // install callback too
    client.setCallback([this](char* t, byte* p, unsigned int l){
        this->callback(t, p, l);
    });
}

void MQTTManager::loop() {
    if (!client.connected()) {
        if (connect()) onConnected();
    }
    client.loop();
}


bool MQTTManager::connect() {
    if (username && password)
        return client.connect(clientId, username, password);
    return client.connect(clientId);
}

void MQTTManager::onConnected() {
    client.subscribe(buildTopic("cmd").c_str());
    publish("status", "online", true);

    publishTargetPortion();
    publishBowlStatus();
    publishWeight();
    publishStorageStatus();
}

bool MQTTManager::publish(const String& sub, const String& payload, bool retained) {
    if (!client.connected()) return false;
    return client.publish(buildTopic(sub).c_str(), payload.c_str(), retained);
}

void MQTTManager::publishBowlStatus() {
    const bool hasFood = weightSensor.hasFoodInBowl();
    String payload = hasFood ? "has_food" : "empty";

    Serial.print("[MQTT] bowl → ");
    Serial.println(payload);

    publish("bowl", payload, true);
}

void MQTTManager::publishTargetPortion() {
    Serial.print("[MQTT] portion → ");
    Serial.println(weightSensor.getTargetPortion());

    publish("portion", String(weightSensor.getTargetPortion()), true);
}

void MQTTManager::publishWeight() {
    float w = weightSensor.getSmoothedWeight();

    Serial.print("[MQTT] weight → ");
    Serial.println(w);

    publish("weight", String(w), false);
}

void MQTTManager::publishStorageStatus() {
    float percent = storage.getRemainingPercent();
    float weight  = storage.getEstimatedWeight();

    String state = "OK";
    if (storage.isEmpty()) state = "EMPTY";
    else if (storage.isLow()) state = "LOW";

    Serial.print("[MQTT] storage percent → ");
    Serial.println(percent);

    Serial.print("[MQTT] storage weight → ");
    Serial.println(weight);

    Serial.print("[MQTT] storage state → ");
    Serial.println(state);

    publish("storage/percent", String(percent, 1), true);
    publish("storage/weight", String((int)weight), true);
    publish("storage/state", state, true);
}

String MQTTManager::buildTopic(const String& s) const {
    return "autofeeder/" + feederId + "/" + s;
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    // convert payload → string
    String msg;
    msg.reserve(length);
    for (unsigned int i = 0; i < length; i++)
        msg += (char)payload[i];

    String t(topic);

    // only care about "cmd"
    if (t != buildTopic("cmd")) return;

    // feed command
    if (msg == "feed") {
        feedToTargetPortion();
        return;
    }

    // status request
    if (msg == "status") {
        String s = "weight:" + String(weightSensor.getWeight());
        publish("status", s, false);
    }
}
