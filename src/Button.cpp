#include "Button.h"

//Button declaration
Button::Button(int pin, bool activeHigh)
    : pin(pin), activeHigh(activeHigh) {}

void Button::setup()
{
    pinMode(pin, INPUT); // external pulldown → correct
}


