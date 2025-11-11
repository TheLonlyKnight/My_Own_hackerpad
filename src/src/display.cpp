#include "display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Single shared SSD1306 instance used by Display methods
// Create the display instance in the .cpp (hide implementation from header)
static Adafruit_SSD1306 ssd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void Display::init() {
    // Initialize I2C with the pins the user specified (if supported on platform)
    Wire.begin(OLED_SDA, OLED_SCL);
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