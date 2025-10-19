#pragma once

#include <Arduino.h>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void logMessage(LogLevel level, const char* message) = 0;
    virtual void logFormat(LogLevel level, const char* format, ...) = 0;
    virtual void setLevel(LogLevel level) = 0;
    virtual LogLevel getLevel() const = 0;
};

class SerialLogger : public ILogger {
public:
    explicit SerialLogger(uint32_t baudRate = 115200);
    ~SerialLogger() override = default;
    
    void begin();
    void logMessage(LogLevel level, const char* message) override;
    void logFormat(LogLevel level, const char* format, ...) override;
    void setLevel(LogLevel level) override;
    LogLevel getLevel() const override;

private:
    uint32_t baudRate;
    LogLevel currentLevel;
    const char* levelToString(LogLevel level) const;
};

class NullLogger : public ILogger {
public:
    ~NullLogger() override = default;
    void begin() {}
    void logMessage(LogLevel level, const char* message) override {}
    void logFormat(LogLevel level, const char* format, ...) override {}
    void setLevel(LogLevel level) override {}
    LogLevel getLevel() const override { return LogLevel::ERROR; }
};

#ifdef ENABLE_LOGGING
    extern SerialLogger logger;
    #define LOG_DEBUG(msg) logger.logMessage(LogLevel::DEBUG, msg)
    #define LOG_INFO(msg) logger.logMessage(LogLevel::INFO, msg)
    #define LOG_WARN(msg) logger.logMessage(LogLevel::WARN, msg)
    #define LOG_ERROR(msg) logger.logMessage(LogLevel::ERROR, msg)
    #define LOG_DEBUG_FMT(fmt, ...) logger.logFormat(LogLevel::DEBUG, fmt, __VA_ARGS__)
    #define LOG_INFO_FMT(fmt, ...) logger.logFormat(LogLevel::INFO, fmt, __VA_ARGS__)
    #define LOG_WARN_FMT(fmt, ...) logger.logFormat(LogLevel::WARN, fmt, __VA_ARGS__)
    #define LOG_ERROR_FMT(fmt, ...) logger.logFormat(LogLevel::ERROR, fmt, __VA_ARGS__)
#else
    extern NullLogger logger;
    #define LOG_DEBUG(msg) ((void)0)
    #define LOG_INFO(msg) ((void)0)
    #define LOG_WARN(msg) ((void)0)
    #define LOG_ERROR(msg) ((void)0)
    #define LOG_DEBUG_FMT(fmt, ...) ((void)0)
    #define LOG_INFO_FMT(fmt, ...) ((void)0)
    #define LOG_WARN_FMT(fmt, ...) ((void)0)
    #define LOG_ERROR_FMT(fmt, ...) ((void)0)
#endif
