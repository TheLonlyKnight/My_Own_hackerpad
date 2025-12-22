// xiao_rp2040_keypad_oled.ino
// Single-file Arduino sketch (merged from project sources)

#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <Bounce2.h>

// If interrupts/direct port access are disabled or not available on this core,
// provide a small, polling-based Encoder replacement to avoid pulling in the
// AVR-optimized Encoder library which requires AVR port macros.
#ifdef ENCODER_DO_NOT_USE_INTERRUPTS
class Encoder {
public:
    Encoder(uint8_t pinA, uint8_t pinB) : a(pinA), b(pinB), pos(0), last(0) {
        pinMode(a, INPUT_PULLUP);
        pinMode(b, INPUT_PULLUP);
        last = (digitalRead(a) << 1) | digitalRead(b);
    }

    long read() {
        int s = (digitalRead(a) << 1) | digitalRead(b);
        if (s != last) {
            // simple quadrature state transitions
            // clockwise sequence: 00 -> 01 -> 11 -> 10 -> 00
            if ((last == 0 && s == 1) || (last == 1 && s == 3) || (last == 3 && s == 2) || (last == 2 && s == 0)) pos++;
            else pos--;
            last = s;
        }
        return pos;
    }

private:
    uint8_t a, b;
    long pos;
    int last;
};
#else
#include <Encoder.h>
#endif

// ------------------------- config ---------------------------------
// OLED I2C pins and address (128x32)
#define OLED_SDA D4
#define OLED_SCL D5
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C

// OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// 3x3 keypad
#define KEYPAD_ROWS 3
#define KEYPAD_COLS 3

// Keymap (3x3)
static char KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'}
};

// User-provided wiring (define once here)
uint8_t ROW_PINS[KEYPAD_ROWS] = {D2, D3, D6}; // rows: digital pins 0,1,10
uint8_t COL_PINS[KEYPAD_COLS] = {D0, D1, D10};  // cols: digital pins 2,3,6

// Rotary encoder pins (A, B, push)
#define ENCODER_A D7
#define ENCODER_B D8
#define ENCODER_BTN D9

#define DEBOUNCE_TIME 50 // Debounce time in milliseconds

// NO_KEY is provided by the Keypad library (Key.h). Don't redefine it here.

// ------------------------- display ---------------------------------
class Display {
public:
    Display() {}
    void init();
    void drawText(const String &text);
    void updateDisplay(const String &text);
    void drawDemo();              // new
    void drawShapes();            // new
};

static Adafruit_SSD1306 ssd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void Display::drawDemo() {
    ssd.clearDisplay();
    ssd.setTextSize(2);
    ssd.setCursor(0, 0);
    ssd.print("Hello");
    ssd.drawRect(0, 18, 60, 12, SSD1306_WHITE); // outline rectangle
    ssd.fillCircle(100, 16, 6, SSD1306_WHITE);  // filled circle
    ssd.display(); // push buffer to OLED
}

void Display::drawShapes() {
    ssd.clearDisplay();
    ssd.drawLine(0, 0, 127, 31, SSD1306_WHITE);
    ssd.drawTriangle(10, 30, 30, 10, 50, 30, SSD1306_WHITE);
    ssd.fillRect(70, 8, 20, 16, SSD1306_WHITE);
    ssd.display();
}
void Display::init() {
    // For ESP32C6: pass SDA/SCL to begin
    Wire.begin(OLED_SDA, OLED_SCL);  // D4, D5

    if (!ssd.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        // handle init failure
        return;
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
enum class EventType { None=0, Key, EncLeft, EncRight, EncBtn };
struct Event { EventType type; char key; };

static const int EVENT_QUEUE_SIZE = 32;
Event evtQueue[EVENT_QUEUE_SIZE];
int evtHead = 0;
int evtTail = 0;

static inline bool evtPush(const Event &e) {
    int next = (evtHead + 1) % EVENT_QUEUE_SIZE;
    if (next == evtTail) return false;
    evtQueue[evtHead] = e;
    evtHead = next;
    return true;
}

static inline bool evtPop(Event &out) {
    if (evtTail == evtHead) return false;
    out = evtQueue[evtTail];
    evtTail = (evtTail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

KeypadMatrix keypad;
Display display;

Encoder encoder(ENCODER_A, ENCODER_B);
long lastEncPos = 0;
Bounce encBtn = Bounce();

void setup() {
    Serial.begin(115200);
    keypad.init();
    display.init();

    pinMode(ENCODER_BTN, INPUT_PULLUP);
    encBtn.attach(ENCODER_BTN);
    encBtn.interval(DEBOUNCE_TIME);

    lastEncPos = encoder.read();
}

void processEvent(const Event &e) {
    switch (e.type) {
        case EventType::Key: {
            String s; s += e.key;
            Serial.println("Key event: " + s);
            display.updateDisplay("Key: " + s);
            break;
        }
        case EventType::EncLeft:
            Serial.println("Encoder left");
            display.updateDisplay("Enc: Left"); break;
        case EventType::EncRight:
            Serial.println("Encoder right");
            display.updateDisplay("Enc: Right"); break;
        case EventType::EncBtn:
            Serial.println("Encoder button");
            display.updateDisplay("Enc: Btn"); break;
        default: break;
    if Eevent
    }
}

void loop() {
    char key = keypad.scan();
    if (key != NO_KEY) { evtPush({EventType::Key, key}); }

    long pos = encoder.read();
    if (pos != lastEncPos) {
        if (pos > lastEncPos) evtPush({EventType::EncRight, 0});
        else evtPush({EventType::EncLeft, 0});
        lastEncPos = pos;
    }

    encBtn.update();
    if (encBtn.fell()) evtPush({EventType::EncBtn, 0});

    Event ev; int processed = 0;
    while (processed < 4 && evtPop(ev)) { processEvent(ev); processed++; }

    delay(5);
}
