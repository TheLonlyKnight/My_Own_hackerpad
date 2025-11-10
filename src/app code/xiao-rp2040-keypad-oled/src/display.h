#pragma once
#include <Arduino.h>
#include "config.h"

class Display {
public:
    Display() {}
    void init();
    void drawText(const String &text);
    void updateDisplay(const String &text);
};