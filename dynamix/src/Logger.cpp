#include "Logger.h"
#include <iostream>
#include <sstream>

namespace Dynamix {

    void Logger::log(LogLevel level, const std::string& message, const std::string& component) {
        if (level > m_currentLevel) {
            return; // Skip logging if level is higher than current setting
        }

        std::stringstream ss;
        ss << "[" << levelToString(level) << "]";

        if (!component.empty()) {
            ss << " [" << component << "]";
        }

        ss << " " << message;

        // Output to appropriate stream based on level
        if (level == LogLevel::ERROR) {
            std::cerr << ss.str() << std::endl;
        } else {
            std::cout << ss.str() << std::endl;
        }
    }

    std::string Logger::levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::WARN:
                return "WARN";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::TRACE:
                return "TRACE";
            default:
                return "UNKNOWN";
        }
    }

    void Logger::error(const std::string& message, const std::string& component) {
        log(LogLevel::ERROR, message, component);
    }

    void Logger::warn(const std::string& message, const std::string& component) {
        log(LogLevel::WARN, message, component);
    }

    void Logger::info(const std::string& message, const std::string& component) {
        log(LogLevel::INFO, message, component);
    }

    void Logger::debug(const std::string& message, const std::string& component) {
        log(LogLevel::DEBUG, message, component);
    }

    void Logger::trace(const std::string& message, const std::string& component) {
        log(LogLevel::TRACE, message, component);
    }

} // namespace Dynamix