#pragma once
#include <HX711.h>
#include <Arduino.h>

class WeightSensor {
private:
    HX711 hx;

    // Basic calibration
    long zeroOffset = 0;
    float calibrationFactor = 0.42;

    // Portion logic
    float targetPortionGrams = 0;  

    // Smoothing
    float smoothedWeight = 0;  
    float alpha = 0.2;

public:
    WeightSensor();

    // Setup
    void setup(int dtPin, int sckPin);

    // Raw readings
    long readRaw();
    float getWeight();
    float getWeightGrams();

    // Smoothed
    float getSmoothedWeight();

    // Tare / zero
    void tareScale();
    void webTareCommand();

    long getZeroOffset();
    bool isReady();

    // Portion management
    void setUserPortion();
    bool portionReached();
    void setPortionFromBowl();
    float readPortionFromSerial();
    void setTargetPortionFromUser();
    void confirmUserPortion();
    float getTargetPortion() const;
    void setTargetPortion(float grams);
    bool hasFoodInBowl(float thresholdGrams = 5.0f) const;
};
