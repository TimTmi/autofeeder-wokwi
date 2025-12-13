#include "Storage.h"

Storage::Storage(float minDistanceCM, float maxDistanceCM, float maxWeightGrams): minDist(minDistanceCM), maxDist(maxDistanceCM), maxWeight(maxWeightGrams) {}

void Storage::setCalibration(float minD, float maxD, float maxW) {
    minDist = minD;
    maxDist = maxD;
    maxWeight = maxW;
}

void Storage::update(float distanceCM) {
    // clamp raw sensor values
    if (distanceCM < minDist) distanceCM = minDist;
    if (distanceCM > maxDist) distanceCM = maxDist;

    // convert to % remaining
    float ratio = (distanceCM - minDist) / (maxDist - minDist);
    percent = 100.0f * (1.0f - ratio);
}

float Storage::getRemainingPercent() const {
    return percent;
}

float Storage::getEstimatedWeight() const {
    return (percent / 100.0f) * maxWeight;
}

bool Storage::isLow() const {
    return percent <= 20.0f;
}

bool Storage::isEmpty() const {
    return percent <= 2.0f;
}
