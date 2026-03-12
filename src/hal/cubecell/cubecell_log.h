#pragma once
#include <Arduino.h>
#include "hal/i_log.h"
namespace MiniCore {
class CubeCellLog : public ILog {
public:
    void log(LogLevel level, const char* msg) override {
        const char* prefix = "";
        switch (level) {
            case LogLevel::Debug: prefix = "[D] "; break;
            case LogLevel::Info: prefix = "[I] "; break;
            case LogLevel::Warning: prefix = "[W] "; break;
            case LogLevel::Error: prefix = "[E] "; break;
        }
        Serial.print(prefix);
        Serial.println(msg);
    }
};
}
