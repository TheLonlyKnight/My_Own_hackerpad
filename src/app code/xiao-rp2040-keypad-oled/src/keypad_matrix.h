#pragma once
#include <Keypad.h>
#include "config.h"

class KeypadMatrix {
public:
    KeypadMatrix();
    void init();
    char scan();

private:
    Keypad keypad;
};