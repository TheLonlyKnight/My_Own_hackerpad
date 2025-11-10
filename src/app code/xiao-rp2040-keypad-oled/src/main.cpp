#include <Arduino.h>
#include "config.h"
#include "keypad_matrix.h"
#include "display.h"

#include <Encoder.h>
#include <Bounce2.h>

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