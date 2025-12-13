#include "WeightSensor.h"

WeightSensor::WeightSensor() {}

void WeightSensor::setup(int dtPin, int sckPin) {
    hx.begin(dtPin, sckPin);
    tareScale();
}

void WeightSensor::tareScale() {
    long sum = 0;
    const int samples = 10;

    for (int i = 0; i < samples; i++) {
        sum += readRaw();
        delay(10);
    }

    zeroOffset = sum / samples;
}

long WeightSensor::readRaw() {
    return hx.read();
}

float WeightSensor::getWeight() {
    long raw = readRaw();
    long adjusted = raw - zeroOffset;
    return adjusted / calibrationFactor;
}

long WeightSensor::getZeroOffset() {
    return zeroOffset;
}

bool WeightSensor::isReady() {
    return hx.is_ready();
}
