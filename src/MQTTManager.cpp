#include "MQTTManager.h"
#include "Dispenser.h"
#include "WeightSensor.h"
#include "Storage.h"

extern Dispenser dispenser;
extern WeightSensor weightSensor;
extern Storage storage;

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
}

bool MQTTManager::publish(const String& sub, const String& payload, bool retained) {
    if (!client.connected()) return false;
    return client.publish(buildTopic(sub).c_str(), payload.c_str(), retained);
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
    if (msg.startsWith("feed")) {
        int amount = msg.substring(4).toInt();
        if (amount <= 0) return;

        dispenser.dispense(
            amount,
            [](){ return weightSensor.getWeight(); },
            [](){ return storage.getEstimatedWeight(); }
        );
        return;
    }

    // status request
    if (msg == "status") {
        String s = "weight:" + String(weightSensor.getWeight());
        publish("status", s, false);
    }
}
