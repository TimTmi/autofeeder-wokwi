#include "Button.h"

Button::Button(int pin, bool activeHigh)
    : pin(pin), activeHigh(activeHigh) {}

void Button::setup() {
    pinMode(pin, INPUT);  // external pulldown → correct
}

void Button::loop() {
    bool raw = digitalRead(pin);
    bool logical = activeHigh ? raw : !raw;

    if (logical != lastState) {
        lastChange = millis();
    }

    if ((millis() - lastChange) > debounceMs) {
        if (logical && !pressedEvent && !stableState) {
            pressedEvent = true;
        }
        stableState = logical;
    }

    lastState = logical;
}

bool Button::wasPressed() {
    if (pressedEvent) {
        pressedEvent = false;
        return true;
    }
    return false;
}
