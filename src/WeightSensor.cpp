#include "WeightSensor.h"

WeightSensor::WeightSensor() {}

// -------------------- Setup --------------------
void WeightSensor::setup(int dtPin, int sckPin) {
    hx.begin(dtPin, sckPin);
    tareScale();
}

// -------------------- Raw readings --------------------
long WeightSensor::readRaw() {
    return hx.read();
}

float WeightSensor::getWeight() {
    long raw = readRaw();
    long adjusted = raw - zeroOffset;
    return adjusted / calibrationFactor;
}

float WeightSensor::getWeightGrams() {
    return getWeight();
}

// -------------------- Smoothed readings --------------------
float WeightSensor::getSmoothedWeight() {
    float current = getWeightGrams();
    smoothedWeight = alpha * current + (1 - alpha) * smoothedWeight;
    return smoothedWeight;
}

// -------------------- Tare / Zero --------------------
void WeightSensor::tareScale() {
    long sum = 0;
    const int samples = 20;
    for (int i = 0; i < samples; i++) {
        sum += readRaw();
        delay(10);
    }
    zeroOffset = sum / samples;
    smoothedWeight = getWeightGrams();
}

void WeightSensor::webTareCommand() {
    tareScale();
    Serial.println("Scale tared.");
}

long WeightSensor::getZeroOffset() {
    return zeroOffset;
}

bool WeightSensor::isReady() {
    return hx.is_ready();
}

// -------------------- Portion Management --------------------
void WeightSensor::setUserPortion() {
    targetPortionGrams = getWeightGrams();
    smoothedWeight = targetPortionGrams;
}

bool WeightSensor::portionReached() {
    return getSmoothedWeight() >= targetPortionGrams;
}

void WeightSensor::setPortionFromBowl() {
    const float threshold = 0.2;
    float lastWeight = getSmoothedWeight();
    unsigned long stableTime = 0;

    while (stableTime < 1000) {
        float current = getSmoothedWeight();
        if (abs(current - lastWeight) < threshold) {
            stableTime += 50;
        } else {
            stableTime = 0;
        }
        lastWeight = current;
        delay(50);
    }

    targetPortionGrams = lastWeight;
    Serial.print("Target portion set to: ");
    Serial.println(targetPortionGrams);
}

float WeightSensor::readPortionFromSerial() {
    while (Serial.available() == 0) delay(10);
    float value = Serial.parseFloat();
    Serial.readStringUntil('\n');
    return value;
}

void WeightSensor::setTargetPortionFromUser() {
    Serial.println("Enter desired portion in grams:");
    targetPortionGrams = readPortionFromSerial();
    smoothedWeight = targetPortionGrams;
    Serial.print("Target portion set to: ");
    Serial.println(targetPortionGrams);
}

void WeightSensor::confirmUserPortion() {
    float grams = getWeightGrams();
    targetPortionGrams = grams;
    smoothedWeight = grams;
    Serial.print("User-confirmed portion: ");
    Serial.println(targetPortionGrams);
}

float WeightSensor::getTargetPortion() const {
    return targetPortionGrams;
}

void WeightSensor::setTargetPortion(float grams) {
    if (grams >= 0) {
        targetPortionGrams = grams;
    }
}

bool WeightSensor::hasFoodInBowl(float thresholdGrams) const {
    return smoothedWeight > thresholdGrams;
}