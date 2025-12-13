#include "Dispenser.h"

Dispenser::Dispenser(int pin, unsigned long timeout): pin(pin), timeout(timeout) {}

void Dispenser::setup() {
    servo.attach(pin);
    stop();
}

void Dispenser::start() { servo.write(90); }
void Dispenser::stop() { servo.write(0); }

void Dispenser::loop() {
    if (state != DISPENSING) return;

    if (bowlGetter() >= targetWeight) {
        stop();
        state = DONE;
    }
    else if (storageGetter() <= 0) {
        stop();
        state = ERROR;
    }
    else if (millis() - startTime > timeout) {
        stop();
        state = ERROR;
    }
}

bool Dispenser::dispense(int grams, float (*weightFunc)(void), float (*storageFunc)(void)) {
    if (state != IDLE) return false;

    targetWeight = grams;
    bowlGetter = weightFunc;
    storageGetter = storageFunc;

    start();
    startTime = millis();
    state = DISPENSING;

    return true;
}

Dispenser::State Dispenser::getState() const { return state; }
void Dispenser::reset() { state = IDLE; }
