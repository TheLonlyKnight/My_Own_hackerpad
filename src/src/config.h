#pragma once
#include <Arduino.h>

// OLED I2C pins and address (128x32)
#define OLED_SDA 4
#define OLED_SCL 5
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C

// OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// 3x3 keypad
#define KEYPAD_ROWS 3
#define KEYPAD_COLS 3

// Keymap (3x3)
// Make these non-const so they can be passed to libraries that expect mutable pointers.
static char KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'}
};

// User-provided wiring (declare only; define once in config.cpp)
extern uint8_t ROW_PINS[KEYPAD_ROWS];
extern uint8_t COL_PINS[KEYPAD_COLS];

// Rotary encoder pins (A, B, push)
#define ENCODER_A 7
#define ENCODER_B 8
#define ENCODER_BTN 9

#define DEBOUNCE_TIME 50 // Debounce time in milliseconds