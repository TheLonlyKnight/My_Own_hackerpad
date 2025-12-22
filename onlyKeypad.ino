// Minimal standalone keypad test sketch

#include <Arduino.h>
#include <Keypad.h>

// 3x3 keypad layout
#define KEYPAD_ROWS 3
#define KEYPAD_COLS 3

// Keymap
static char KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'}
};

// Wiring: adjust these to match your board
// Using the mapping found in the project single file sketch
uint8_t ROW_PINS[KEYPAD_ROWS] = {0, 1, 10}; // row pins
uint8_t COL_PINS[KEYPAD_COLS] = {2, 3, 6};  // column pins

// Debounce time (ms)
#define DEBOUNCE_TIME 50

// Create keypad instance
Keypad keypad((char*)KEYS, ROW_PINS, COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);

void setup() {
	Serial.begin(115200);
	while (!Serial) { /* wait for serial on some boards */ }

	keypad.setDebounceTime(DEBOUNCE_TIME);

	Serial.println("Keypad test ready. Press keys (1-9).\n");
	Serial.println("Rows: D0,D1,D10 | Cols: D2,D3,D6");
}

void loop() {
	char key = keypad.getKey();
	if (key != NO_KEY) {
		Serial.print("Key pressed: ");
		Serial.println(key);
	}

	delay(5);
}

