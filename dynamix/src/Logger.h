#pragma once

#include <iostream>
#include <string>

namespace Dynamix {

    enum class LogLevel { Error = 0, WARN = 1, INFO = 2, DEBUG = 3, TRACE = 4 };

    class Logger {
    public:
        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        // Set the current log level
        void setLevel(LogLevel level) { m_currentLevel = level; }
        LogLevel getLevel() const { return m_currentLevel; }

        // Logging methods
        void error(const std::string& message, const std::string& component = "");
        void warn(const std::string& message, const std::string& component = "");
        void info(const std::string& message, const std::string& component = "");
        void debug(const std::string& message, const std::string& component = "");
        void trace(const std::string& message, const std::string& component = "");

    private:
        Logger() : m_currentLevel(LogLevel::INFO) {}
        ~Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void log(LogLevel level, const std::string& message, const std::string& component);
        std::string levelToString(LogLevel level) const;

        LogLevel m_currentLevel;
    };

// Convenience macros for easier usage
#define LOG_ERROR(msg) Dynamix::Logger::getInstance().error(msg)
#define LOG_WARN(msg) Dynamix::Logger::getInstance().warn(msg)
#define LOG_INFO(msg) Dynamix::Logger::getInstance().info(msg)
#define LOG_DEBUG(msg) Dynamix::Logger::getInstance().debug(msg)
#define LOG_TRACE(msg) Dynamix::Logger::getInstance().trace(msg)

#define LOG_ERROR_COMP(comp, msg) Dynamix::Logger::getInstance().error(msg, comp)
#define LOG_WARN_COMP(comp, msg) Dynamix::Logger::getInstance().warn(msg, comp)
#define LOG_INFO_COMP(comp, msg) Dynamix::Logger::getInstance().info(msg, comp)
#define LOG_DEBUG_COMP(comp, msg) Dynamix::Logger::getInstance().debug(msg, comp)
#define LOG_TRACE_COMP(comp, msg) Dynamix::Logger::getInstance().trace(msg, comp)

} // namespace Dynamix