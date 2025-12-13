#pragma once
#include <Arduino.h>

class Storage {
public:
    Storage(float minDistanceCM, float maxDistanceCM, float maxWeightGrams);

    void update(float distanceCM);

    float getRemainingPercent() const;
    float getEstimatedWeight() const;

    bool isLow() const;
    bool isEmpty() const;

    void setCalibration(float minD, float maxD, float maxW);

private:
    float minDist;       // distance = full
    float maxDist;       // distance = empty
    float maxWeight;     // grams when full

    float percent = 0.0f;
};
