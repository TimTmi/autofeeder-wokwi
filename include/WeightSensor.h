#pragma once
#include <Arduino.h>
#include <HX711.h>

class WeightSensor {
public:
    WeightSensor();

    void setup(int dtPin, int sckPin);
    void tareScale();

    float getWeight();
    long getZeroOffset();
    bool isReady();

private:
    long readRaw();

    HX711 hx;
    long zeroOffset = 0;
    float calibrationFactor = 0.42f;
};
