#include "Dispenser.h"

Dispenser::Dispenser(int pin, unsigned long timeout): pin(pin), timeout(timeout) {}

void Dispenser::setup() {
    servo.attach(pin);
    stop();
}

void Dispenser::start() { servo.write(90); }
void Dispenser::stop() { servo.write(0); }

void Dispenser::loop() {
    // if (state != DISPENSING) return;

    // // Serial.print("Weight: ");
    // // Serial.print(bowlGetter());
    // // Serial.print(" | Storage est: ");
    // // Serial.print(storageGetter());
    // // Serial.print(" | Elapsed: ");
    // // Serial.println(millis() - startTime);

    // if (bowlGetter() >= targetWeight) {
    //     stop();
    //     state = DONE;
    // }
    // else if (storageGetter() <= 0) {
    //     stop();
    //     state = ERROR;
    // }
    // else if (millis() - startTime > timeout) {
    //     stop();
    //     state = ERROR;
    // }

    if (state == DISPENSING) {
        if (bowlGetter() >= targetWeight) {
            stop();
            state = DONE;
            doneTime = millis();
        }
        else if (storageGetter() <= 0) {
            stop();
            state = ERROR;
            errorTime = millis();
        }
        else if (millis() - startTime > timeout) {
            stop();
            state = ERROR;
            errorTime = millis();
        }
    }
    else if (state == DONE) {
        if (millis() - doneTime > 500) {
            state = IDLE;
        }
    }
    else if (state == ERROR) {
        // Testing recovery path
        if (millis() - errorTime > 2000) {
            state = IDLE;
        }
    }
}

// Rollback for dispenser function 


bool Dispenser::dispenseToPortion(float targetPortionGrams, float (*weightFunc)(void), float (*storageFunc)(void)) {
    if (state != IDLE) return false;

    // Pre-check: get current weight
    float currentWeight = weightFunc();
    float availableFood = storageFunc();
    
    // If already at or above target, don't activate
    if (currentWeight >= targetPortionGrams) {
        return false;
    }

    // storage empty or invalid
    if (availableFood <= 0) {
        state = ERROR;
        errorTime = millis();
        return false;
    }

    // Set absolute target weight
    targetWeight = targetPortionGrams;
    bowlGetter = weightFunc;
    storageGetter = storageFunc;

    start();
    startTime = millis();
    state = DISPENSING;

    return true;
}

void Dispenser::setState(Dispenser::State newState) { state = newState; }
Dispenser::State Dispenser::getState() const { return state; }
float Dispenser::getTargetWeight() const { return targetWeight; }
void Dispenser::reset() { state = IDLE; }
