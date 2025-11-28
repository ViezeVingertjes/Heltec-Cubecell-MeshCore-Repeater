#include "LEDIndicator.h"
#include "Config.h"
#include "CubeCell_NeoPixel.h"

// Single RGB LED on the board
static CubeCell_NeoPixel pixels(1, RGB, NEO_GRB + NEO_KHZ800);

LEDIndicator &LEDIndicator::getInstance() {
  static LEDIndicator instance;
  return instance;
}

void LEDIndicator::initialize() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
  
  pixels.begin();
  pixels.setBrightness(Config::LED::BRIGHTNESS);
  pixels.clear();
  pixels.show();
  
  ledOn = false;
  vextOn = false;
  ledOffTime = 0;
}

void LEDIndicator::enableVext() {
  if (!vextOn) {
    digitalWrite(Vext, LOW);
    vextOn = true;
    delayMicroseconds(100);
  }
}

void LEDIndicator::disableVext() {
  if (vextOn) {
    digitalWrite(Vext, HIGH);
    vextOn = false;
  }
}

void LEDIndicator::flashRX() {
  if (!Config::LED::ENABLED) {
    return;
  }
  
  enableVext();
  pixels.setPixelColor(0, pixels.Color(0, Config::LED::BRIGHTNESS, 0));
  pixels.show();
  ledOn = true;
  ledOffTime = millis() + Config::LED::FLASH_DURATION_MS;
}

void LEDIndicator::flashTX() {
  if (!Config::LED::ENABLED) {
    return;
  }
  
  enableVext();
  pixels.setPixelColor(0, pixels.Color(0, 0, Config::LED::BRIGHTNESS));
  pixels.show();
  ledOn = true;
  ledOffTime = millis() + Config::LED::FLASH_DURATION_MS;
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
  disableVext();
}

