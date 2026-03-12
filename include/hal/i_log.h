#pragma once
namespace MiniCore {
enum class LogLevel : uint8_t {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};
class ILog {
public:
    virtual ~ILog() = default;
        virtual void log(LogLevel level, const char* message) = 0;
        void debug(const char* message) { log(LogLevel::Debug, message); }
    void info(const char* message) { log(LogLevel::Info, message); }
    void warning(const char* message) { log(LogLevel::Warning, message); }
    void error(const char* message) { log(LogLevel::Error, message); }
};
class NullLog : public ILog {
public:
    void log(LogLevel, const char*) override {}
};
}
