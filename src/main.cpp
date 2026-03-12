#include "app/device_factory.h"
#include "app/application.h"
#ifdef __asr650x__
#include <Arduino.h>
#endif
namespace {
    MiniCore::Application* g_app = nullptr;
}
void setup() {
#ifdef __asr650x__
    Serial.begin(115200);
    delay(200);
    Serial.println("[boot] setup start");
#endif
    MiniCore::IDevice& device = MiniCore::createDevice();
#ifdef __asr650x__
    Serial.println("[boot] device created");
#endif

    static MiniCore::Application app(device);
    g_app = &app;
#ifdef __asr650x__
    Serial.println("[boot] application constructed");
#endif
    g_app->init();
#ifdef __asr650x__
    Serial.println("[boot] init done");
#endif
}
void loop() {
    if (g_app != nullptr) {
        g_app->loop();
    }
}
