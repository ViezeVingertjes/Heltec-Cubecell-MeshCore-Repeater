#include "Logger.h"
#include "Config.h"

SerialLogger::SerialLogger(uint32_t baudRate)
    : baudRate(baudRate), currentLevel(LogLevel::INFO) {}

void SerialLogger::begin() {
#ifdef ENABLE_LOGGING
  Serial.begin(baudRate);
  delay(500);  // Reduced from 2000ms for faster startup
#endif
}

void SerialLogger::logMessage(LogLevel level, const char *message) {
#ifdef ENABLE_LOGGING
  if (level >= currentLevel) {
    Serial.print("[");
    Serial.print(levelToString(level));
    Serial.print("] ");
    Serial.println(message);
  }
#endif
}

void SerialLogger::logFormat(LogLevel level, const char *format, ...) {
#ifdef ENABLE_LOGGING
  if (level >= currentLevel) {
    char buffer[Config::Logging::BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.print("[");
    Serial.print(levelToString(level));
    Serial.print("] ");
    Serial.println(buffer);
  }
#endif
}

void SerialLogger::setLevel(LogLevel level) { currentLevel = level; }

LogLevel SerialLogger::getLevel() const { return currentLevel; }

const char *SerialLogger::levelToString(LogLevel level) const {
  switch (level) {
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARN:
    return "WARN";
  case LogLevel::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

#ifdef ENABLE_LOGGING
SerialLogger logger(115200);
#else
NullLogger logger;
#endif
