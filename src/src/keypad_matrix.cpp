#include "keypad_matrix.h"
#include "config.h"

KeypadMatrix::KeypadMatrix()
    : keypad((char*)KEYS, ROW_PINS, COL_PINS, KEYPAD_ROWS, KEYPAD_COLS) {}

void KeypadMatrix::init() {
    keypad.setDebounceTime(DEBOUNCE_TIME);
}

char KeypadMatrix::scan() {
    return keypad.getKey();
}