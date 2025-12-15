#include "Button.h"

// Button declaration
Button::Button(int pin, bool activeHigh)
    : pin(pin), activeHigh(activeHigh) {}

void Button::setup()
{
    pinMode(pin, INPUT); // external pulldown → correct
}

void Button::loop()
{
    bool raw = digitalRead(pin);

    bool logical;
    if (activeHigh)
    {
        logical = raw;
    }
    else
    {
        logical = !raw;
    }

    if (logical != lastState)
    {
        lastChange = millis();
    }

    if ((millis() - lastChange) > debounceMs)
    {
        if (logical && !pressedEvent && !stableState)
        {
            pressedEvent = true;
        }
        stableState = logical;
    }

    lastState = logical;
}

bool Button::wasPressed()
{
    if (pressedEvent)
    {
        pressedEvent = false;
        return true;
    }

    return false;
}
