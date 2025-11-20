#include "LEDIndicator.h"
#include "CubeCell_NeoPixel.h"

// Single RGB LED on the board
static CubeCell_NeoPixel pixels(1, RGB, NEO_GRB + NEO_KHZ800);

LEDIndicator &LEDIndicator::getInstance() {
  static LEDIndicator instance;
  return instance;
}

void LEDIndicator::initialize() {
  // Enable Vext power rail (required for LED on some boards)
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  
  // Initialize the NeoPixel
  pixels.begin();
  pixels.setBrightness(50);  // Moderate brightness to save power
  pixels.clear();
  pixels.show();
  
  ledOn = false;
  ledOffTime = 0;
}

void LEDIndicator::flashRX() {
  // Green flash for packet received
  pixels.setPixelColor(0, pixels.Color(0, 30, 0));  // Green
  pixels.show();
  ledOn = true;
  ledOffTime = millis() + 50;  // 50ms flash
}

void LEDIndicator::flashTX() {
  // Blue flash for packet transmitted
  pixels.setPixelColor(0, pixels.Color(0, 0, 30));  // Blue
  pixels.show();
  ledOn = true;
  ledOffTime = millis() + 50;  // 50ms flash
}

void LEDIndicator::loop() {
  if (ledOn && millis() >= ledOffTime) {
    turnOff();
  }
}

void LEDIndicator::turnOff() {
  pixels.clear();
  pixels.show();
  ledOn = false;
}

