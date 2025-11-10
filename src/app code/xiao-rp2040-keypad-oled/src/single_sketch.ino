// Merged single-file sketch (config + display + keypad_matrix + main)
// Generated from project source files to allow a single-file build.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <Encoder.h>
#include <Bounce2.h>

// ------------------------- config ---------------------------------
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

// User-provided wiring (define once here)
uint8_t ROW_PINS[KEYPAD_ROWS] = {0, 1, 10}; // rows: digital pins 0,1,10 (kept as original mapping)
uint8_t COL_PINS[KEYPAD_COLS] = {2, 3, 6};  // cols: digital pins 2,3,6

// Rotary encoder pins (A, B, push)
#define ENCODER_A 7
#define ENCODER_B 8
#define ENCODER_BTN 9

#define DEBOUNCE_TIME 50 // Debounce time in milliseconds

// NO_KEY is provided by the Keypad library (Key.h). Don't redefine it here.

// ------------------------- display ---------------------------------
class Display {
public:
    Display() {}
    void init();
    void drawText(const String &text);
    void updateDisplay(const String &text);
};

// Single shared SSD1306 instance used by Display methods
static Adafruit_SSD1306 ssd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void Display::init() {
    // Initialize I2C. Use default begin() so it compiles across Arduino cores.
    // If you need non-default SDA/SCL pins on your board, replace with Wire.begin(sda, scl)
    // after selecting the correct board in Arduino IDE.
    Wire.begin();
    if (!ssd.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        // failed to init display; you may want to handle this
    }
    ssd.clearDisplay();
    ssd.setTextSize(1);
    ssd.setTextColor(SSD1306_WHITE);
}

void Display::drawText(const String &text) {
    ssd.clearDisplay();
    ssd.setCursor(0, 0);
    ssd.print(text);
    ssd.display();
}

void Display::updateDisplay(const String &text) {
    drawText(text);
}

// ----------------------- keypad matrix -----------------------------
class KeypadMatrix {
public:
    KeypadMatrix();
    void init();
    char scan();

private:
    Keypad keypad;
};

KeypadMatrix::KeypadMatrix()
    : keypad((char*)KEYS, ROW_PINS, COL_PINS, KEYPAD_ROWS, KEYPAD_COLS) {}

void KeypadMatrix::init() {
    keypad.setDebounceTime(DEBOUNCE_TIME);
}

char KeypadMatrix::scan() {
    return keypad.getKey();
}

// --------------------------- main ---------------------------------
// Simple event queue for inputs
enum class EventType { None=0, Key, EncLeft, EncRight, EncBtn };
struct Event {
    EventType type;
    char key; // valid for Key events
};

static const int EVENT_QUEUE_SIZE = 32;
Event evtQueue[EVENT_QUEUE_SIZE];
int evtHead = 0;
int evtTail = 0;

static inline bool evtPush(const Event &e) {
    int next = (evtHead + 1) % EVENT_QUEUE_SIZE;
    if (next == evtTail) return false; // full
    evtQueue[evtHead] = e;
    evtHead = next;
    return true;
}

static inline bool evtPop(Event &out) {
    if (evtTail == evtHead) return false; // empty
    out = evtQueue[evtTail];
    evtTail = (evtTail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

KeypadMatrix keypad;
Display display;

// Encoder and button
Encoder encoder(ENCODER_A, ENCODER_B);
long lastEncPos = 0;
Bounce encBtn = Bounce();

void setup() {
    Serial.begin(115200);
    keypad.init();
    display.init();

    // Encoder button setup
    pinMode(ENCODER_BTN, INPUT_PULLUP);
    encBtn.attach(ENCODER_BTN);
    encBtn.interval(DEBOUNCE_TIME);

    // initialize encoder position
    lastEncPos = encoder.read();
}

void processEvent(const Event &e) {
    switch (e.type) {
        case EventType::Key: {
            String s;
            s += e.key;
            display.updateDisplay("Key: " + s);
            break;
        }
        case EventType::EncLeft:
            display.updateDisplay("Enc: Left");
            break;
        case EventType::EncRight:
            display.updateDisplay("Enc: Right");
            break;
        case EventType::EncBtn:
            display.updateDisplay("Enc: Btn");
            break;
        default:
            break;
    }
}

void loop() {
    // 1) Scan keypad
    char key = keypad.scan();
    if (key != NO_KEY) {
        Event e; e.type = EventType::Key; e.key = key;
        evtPush(e);
    }

    // 2) Read encoder rotation
    long pos = encoder.read();
    if (pos != lastEncPos) {
        if (pos > lastEncPos) evtPush({EventType::EncRight, 0});
        else evtPush({EventType::EncLeft, 0});
        lastEncPos = pos;
    }

    // 3) Read encoder button (debounced)
    encBtn.update();
    if (encBtn.fell()) {
        evtPush({EventType::EncBtn, 0});
    }

    // 4) Process up to a few events per loop to avoid starvation
    Event ev;
    int processed = 0;
    while (processed < 4 && evtPop(ev)) {
        processEvent(ev);
        processed++;
    }

    // Short delay to avoid starving CPU; keeps loop responsive
    delay(5);
}
