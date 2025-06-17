#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace AudioTester {

    // Error categories
    enum class ErrorCategory {
        Resource,     // Resource loading/management errors
        Audio,        // Audio playback/processing errors
        Filter,       // Filter-related errors
        System,       // System-level errors
        Configuration // Configuration errors
    };

    // Base error class
    class Error : public std::runtime_error {
    public:
        Error(const std::string& message, ErrorCategory category)
            : std::runtime_error(message), m_category(category) {}

        ErrorCategory getCategory() const { return m_category; }

    private:
        ErrorCategory m_category;
    };

    // Specific error types
    class ResourceError : public Error {
    public:
        ResourceError(const std::string& message) : Error(message, ErrorCategory::Resource) {}
    };

    class AudioError : public Error {
    public:
        AudioError(const std::string& message) : Error(message, ErrorCategory::Audio) {}
    };

    class FilterError : public Error {
    public:
        FilterError(const std::string& message) : Error(message, ErrorCategory::Filter) {}
    };

    class SystemError : public Error {
    public:
        SystemError(const std::string& message) : Error(message, ErrorCategory::System) {}
    };

    class ConfigurationError : public Error {
    public:
        ConfigurationError(const std::string& message)
            : Error(message, ErrorCategory::Configuration) {}
    };

    // Logging utilities
    class Logger {
    public:
        enum class Level { Debug, Info, Warning, Error };

        static void log(Level level, const std::string& message, const std::string& source = "") {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);

            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " ";

            switch (level) {
                case Level::Debug:
                    ss << "[DEBUG] ";
                    break;
                case Level::Info:
                    ss << "[INFO] ";
                    break;
                case Level::Warning:
                    ss << "[WARNING] ";
                    break;
                case Level::Error:
                    ss << "[ERROR] ";
                    break;
            }

            if (!source.empty()) {
                ss << "[" << source << "] ";
            }

            ss << message;

            // For now, just print to console
            // TODO: Add file logging, log rotation, etc.
            std::cout << ss.str() << std::endl;
        }

        static void debug(const std::string& message, const std::string& source = "") {
            log(Level::Debug, message, source);
        }

        static void info(const std::string& message, const std::string& source = "") {
            log(Level::Info, message, source);
        }

        static void warning(const std::string& message, const std::string& source = "") {
            log(Level::Warning, message, source);
        }

        static void error(const std::string& message, const std::string& source = "") {
            log(Level::Error, message, source);
        }
    };

    // Error handling utilities
    inline void handleError(const Error& error, const std::string& source = "") {
        Logger::error(error.what(), source);
        // TODO: Add error reporting, crash dumps, etc.
    }

// Helper macro for throwing errors with source information
#define THROW_ERROR(error_type, message)                                                           \
    throw error_type(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message)

} // namespace AudioTester