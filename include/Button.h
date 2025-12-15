#pragma once
#include <Arduino.h>

class Button {
public:
    Button(int pin, bool activeHigh = true);

    void setup();
    void loop();

    bool wasPressed();  

private:
    int pin;
    bool activeHigh;

    bool lastState = false;
    bool pressedEvent = false;
    bool stableState = false;

    unsigned long lastChange = 0;
    static constexpr unsigned long debounceMs = 50;
};