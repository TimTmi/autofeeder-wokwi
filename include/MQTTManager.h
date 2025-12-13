#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

class MQTTManager {
public:
    MQTTManager(PubSubClient& c);

    void setup(
        const char* host,
        int port,
        const char* user,
        const char* pass,
        const char* id,
        const String& feeder
    );
    void loop();

    bool publish(const String& sub, const String& payload, bool retained = false);

private:
    PubSubClient& client;

    const char* username = nullptr;
    const char* password = nullptr;
    const char* clientId = "feeder";
    String feederId = "unknown";

    bool connect();
    void onConnected();
    void callback(char* topic, byte* payload, unsigned int length);

    String buildTopic(const String& s) const;
};
