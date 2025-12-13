#pragma once
#include <Arduino.h>
#include <Servo.h>

class Dispenser {
public:
    enum State {
        IDLE,
        DISPENSING,
        DONE,
        ERROR
    };

    Dispenser(int pin, unsigned long timeout = 5000);

    void setup();
    void loop();
    bool dispense(int grams, float (*weightFunc)(void), float (*storageFunc)(void));

    State getState() const;
    void reset();

private:
    void start();
    void stop();

    Servo servo;
    int pin;
    State state = IDLE;

    float targetWeight = 0;
    float (*bowlGetter)(void) = nullptr;
    float (*storageGetter)(void) = nullptr;

    unsigned long startTime = 0;
    unsigned long timeout;
};
