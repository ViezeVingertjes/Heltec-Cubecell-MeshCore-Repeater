#pragma once
#include "hal/i_log.h"
#include <vector>
#include <string>
#include <utility>
namespace MiniCore::Mocks {
class MockLog : public ILog {
public:
    std::vector<std::pair<LogLevel, std::string>> entries;
        void log(LogLevel level, const char* message) override {
        entries.emplace_back(level, message);
    }
        [[nodiscard]] size_t count() const { return entries.size(); }
        [[nodiscard]] size_t countLevel(LogLevel level) const {
        size_t n = 0;
        for (const auto& e : entries) {
            if (e.first == level) ++n;
        }
        return n;
    }
        [[nodiscard]] bool hasMessage(const char* message) const {
        for (const auto& e : entries) {
            if (e.second == message) return true;
        }
        return false;
    }
        void clear() { entries.clear(); }
};
}
