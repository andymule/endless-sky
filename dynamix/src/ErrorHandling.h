#pragma once

#include "Logger.h"
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

namespace Dynamix {

    /**
     * Error Types - Categorized Error System
     *
     * This enum defines all possible error types in the system, organized by category.
     * Each error type has a specific meaning and can be handled appropriately.
     */
    enum class ErrorType {
        // File System Errors
        FILE_NOT_FOUND,
        FILE_ACCESS_DENIED,
        FILE_ALREADY_EXISTS,
        DIRECTORY_NOT_FOUND,
        DIRECTORY_ACCESS_DENIED,
        DIRECTORY_ALREADY_EXISTS,
        DISK_FULL,
        INVALID_PATH,

        // Audio System Errors
        AUDIO_INIT_FAILED,
        AUDIO_LOAD_FAILED,
        AUDIO_FORMAT_UNSUPPORTED,
        AUDIO_DEVICE_ERROR,
        AUDIO_BUFFER_OVERFLOW,
        AUDIO_BUFFER_UNDERFLOW,
        AUDIO_PROCESSING_ERROR,

        // JSON/Data Errors
        JSON_PARSE_ERROR,
        JSON_VALIDATION_ERROR,
        JSON_MISSING_FIELD,
        JSON_INVALID_TYPE,
        JSON_INVALID_VALUE,

        // State/Logic Errors
        INVALID_STATE,
        INVALID_PARAMETER,
        INVALID_OPERATION,
        RESOURCE_NOT_FOUND,
        RESOURCE_ALREADY_EXISTS,
        RESOURCE_IN_USE,

        // System Errors
        MEMORY_ERROR,
        THREAD_ERROR,
        TIMEOUT_ERROR,
        NETWORK_ERROR,

        // Unknown/Generic Errors
        UNKNOWN_ERROR
    };

    /**
     * Error Structure - Detailed Error Information
     *
     * Contains all information needed to understand and handle an error:
     * - Type: Categorized error type for programmatic handling
     * - Message: Human-readable error description
     * - Context: Additional context about where/why the error occurred
     * - Component: Which component generated the error
     * - Nested: Optional nested error for error chaining
     */
    struct Error {
        ErrorType type;
        std::string message;
        std::string context;
        std::string component;
        std::optional<std::shared_ptr<Error>> nested;

        // Constructors with move semantics
        Error(ErrorType t, std::string msg, std::string ctx = "", std::string comp = "")
            : type(t), message(std::move(msg)), context(std::move(ctx)),
              component(std::move(comp)) {}

        Error(ErrorType t, std::string msg, std::string ctx, std::string comp,
              std::shared_ptr<Error> nested_error)
            : type(t), message(std::move(msg)), context(std::move(ctx)), component(std::move(comp)),
              nested(std::move(nested_error)) {}

        // Copy constructor
        Error(const Error&) = default;

        // Move constructor
        Error(Error&&) = default;

        // Copy assignment
        Error& operator=(const Error&) = default;

        // Move assignment
        Error& operator=(Error&&) = default;

        // Destructor
        ~Error() = default;

        // Convert to string for logging
        std::string toString() const;

        // Check if this is a specific error type
        bool is(ErrorType t) const { return type == t; }

        // Check if this is a file system error
        bool isFileSystemError() const;

        // Check if this is an audio error
        bool isAudioError() const;

        // Check if this is a data error
        bool isDataError() const;

        // Check if this is a system error
        bool isSystemError() const;

        // Check if this is a state/logic error
        bool isStateError() const;

        // Get error severity (for prioritization)
        int getSeverity() const;

        // Check if error can be retried
        bool isRetryable() const;
    };

    /**
     * Result Template - Type-Safe Error Handling
     *
     * A template class that can hold either a successful result (T) or an error.
     * This eliminates the need for error codes and provides type-safe error handling.
     *
     * Usage:
     *   Result<std::string> result = loadFile("test.txt");
     *   if (result.isError()) {
     *       handleError(result.error());
     *   } else {
     *       useValue(result.value());
     *   }
     */
    template <typename T> class Result {
    private:
        std::variant<T, Error> m_data;

    public:
        // Success constructors with perfect forwarding
        template <typename U = T> Result(U&& value) : m_data(std::forward<U>(value)) {}

        // Error constructors
        Result(const Error& error) : m_data(error) {}
        Result(Error&& error) : m_data(std::move(error)) {}

        // Copy constructor
        Result(const Result&) = default;

        // Move constructor
        Result(Result&&) = default;

        // Copy assignment
        Result& operator=(const Result&) = default;

        // Move assignment
        Result& operator=(Result&&) = default;

        // Destructor
        ~Result() = default;

        // Check if result contains a value
        bool isSuccess() const noexcept { return std::holds_alternative<T>(m_data); }
        bool isError() const noexcept { return std::holds_alternative<Error>(m_data); }

        // Get the value (throws if error)
        const T& value() const& {
            if (isError()) {
                throw std::runtime_error("Attempted to get value from error result");
            }
            return std::get<T>(m_data);
        }

        T& value() & {
            if (isError()) {
                throw std::runtime_error("Attempted to get value from error result");
            }
            return std::get<T>(m_data);
        }

        T&& value() && {
            if (isError()) {
                throw std::runtime_error("Attempted to get value from error result");
            }
            return std::move(std::get<T>(m_data));
        }

        // Get the error (throws if success)
        const Error& error() const& {
            if (isSuccess()) {
                throw std::runtime_error("Attempted to get error from success result");
            }
            return std::get<Error>(m_data);
        }

        Error&& error() && {
            if (isSuccess()) {
                throw std::runtime_error("Attempted to get error from success result");
            }
            return std::move(std::get<Error>(m_data));
        }

        // Safe value access with default
        T valueOr(T defaultValue) const& { return isSuccess() ? value() : std::move(defaultValue); }

        T valueOr(T defaultValue) && {
            return isSuccess() ? std::move(value()) : std::move(defaultValue);
        }

        // Transform success value
        template <typename U> Result<U> map(std::function<U(const T&)> fn) const& {
            if (isError()) {
                return Result<U>(error());
            }
            return Result<U>(fn(value()));
        }

        template <typename U> Result<U> map(std::function<U(T&&)> fn) && {
            if (isError()) {
                return Result<U>(std::move(error()));
            }
            return Result<U>(fn(std::move(value())));
        }

        // Chain operations
        template <typename U> Result<U> andThen(std::function<Result<U>(const T&)> fn) const& {
            if (isError()) {
                return Result<U>(error());
            }
            return fn(value());
        }

        template <typename U> Result<U> andThen(std::function<Result<U>(T&&)> fn) && {
            if (isError()) {
                return Result<U>(std::move(error()));
            }
            return fn(std::move(value()));
        }

        // Handle both success and error cases
        template <typename U>
        U match(std::function<U(const T&)> onSuccess,
                std::function<U(const Error&)> onError) const& {
            if (isError()) {
                return onError(error());
            }
            return onSuccess(value());
        }

        template <typename U>
        U match(std::function<U(T&&)> onSuccess, std::function<U(Error&&)> onError) && {
            if (isError()) {
                return onError(std::move(error()));
            }
            return onSuccess(std::move(value()));
        }

        // Execute side effects
        Result<T> onSuccess(std::function<void(const T&)> fn) const& {
            if (isSuccess()) {
                fn(value());
            }
            return *this;
        }

        Result<T> onError(std::function<void(const Error&)> fn) const& {
            if (isError()) {
                fn(error());
            }
            return *this;
        }
    };

    /**
     * Error Handler - Centralized Error Processing
     *
     * Provides centralized error handling with logging, recovery strategies,
     * and user notification capabilities.
     */
    class ErrorHandler {
    public:
        static ErrorHandler& getInstance() {
            static ErrorHandler instance;
            return instance;
        }

        // Handle an error with appropriate logging and recovery
        void handleError(const Error& error);

        // Handle an error with custom recovery action
        void handleError(const Error& error, std::function<void()> recovery);

        // Check if an error is recoverable
        bool isRecoverable(const Error& error) const;

        // Get user-friendly error message
        std::string getUserMessage(const Error& error) const;

        // Set error callback for UI notification
        void setErrorCallback(std::function<void(const std::string&)> callback);

        // Get error statistics
        struct ErrorStats {
            size_t totalErrors = 0;
            size_t recoverableErrors = 0;
            size_t unrecoverableErrors = 0;
            std::map<ErrorType, size_t> errorTypeCounts;
        };
        ErrorStats getErrorStats() const;

        // Clear error statistics
        void clearErrorStats();

        // Set error handling policy
        enum class Policy {
            LOG_ONLY,         // Only log errors
            NOTIFY_USER,      // Log and notify user
            ATTEMPT_RECOVERY, // Log, notify, and attempt recovery
            THROW_EXCEPTION   // Throw exception for critical errors
        };
        void setPolicy(Policy policy);

    private:
        ErrorHandler() = default;
        ~ErrorHandler() = default;
        ErrorHandler(const ErrorHandler&) = delete;
        ErrorHandler& operator=(const ErrorHandler&) = delete;

        std::function<void(const std::string&)> m_errorCallback;
        Policy m_policy = Policy::NOTIFY_USER;
        mutable ErrorStats m_stats;
        mutable std::mutex m_statsMutex;

        void logError(const Error& error);
        void notifyUser(const Error& error);
        void updateStats(const Error& error) const;
    };

    /**
     * Error Utilities - Helper Functions
     *
     * Common error creation and handling utilities.
     */
    namespace ErrorUtils {

        // Create file system errors
        Error fileNotFound(const std::string& path, const std::string& component = "");
        Error fileAccessDenied(const std::string& path, const std::string& component = "");
        Error fileAlreadyExists(const std::string& path, const std::string& component = "");
        Error directoryNotFound(const std::string& path, const std::string& component = "");
        Error directoryAccessDenied(const std::string& path, const std::string& component = "");
        Error directoryAlreadyExists(const std::string& path, const std::string& component = "");

        // Create audio errors
        Error audioInitFailed(const std::string& details, const std::string& component = "");
        Error audioLoadFailed(const std::string& path, const std::string& details,
                              const std::string& component = "");
        Error audioFormatUnsupported(const std::string& format, const std::string& component = "");

        // Create JSON errors
        Error jsonParseError(const std::string& path, const std::string& details,
                             const std::string& component = "");
        Error jsonValidationError(const std::string& field, const std::string& details,
                                  const std::string& component = "");
        Error jsonMissingField(const std::string& field, const std::string& component = "");

        // Create parameter errors
        Error invalidParameter(const std::string& param, const std::string& value,
                               const std::string& component = "");
        Error invalidOperation(const std::string& operation, const std::string& reason,
                               const std::string& component = "");

        // Create resource errors
        Error resourceNotFound(const std::string& resource, const std::string& component = "");
        Error resourceAlreadyExists(const std::string& resource, const std::string& component = "");

        // Create system errors
        Error memoryError(const std::string& details, const std::string& component = "");
        Error networkError(const std::string& details, const std::string& component = "");
        Error threadError(const std::string& details, const std::string& component = "");
        Error timeoutError(const std::string& details, const std::string& component = "");

        // Convert exception to error
        Error fromException(const std::exception& e, const std::string& context,
                            const std::string& component = "");

        // Convert filesystem error to error
        Error fromFilesystemError(const std::filesystem::filesystem_error& e,
                                  const std::string& component = "");

        // Create error with nested error (for error chaining)
        Error withNested(const Error& outer, const Error& inner);

        // Create error with additional context
        Error withContext(const Error& error, const std::string& additionalContext);
    } // namespace ErrorUtils

// Convenience macros for error handling
#define RETURN_ERROR(type, msg, ctx, comp)                                                         \
    return Result<decltype(Result<void>())>(ErrorUtils::type(msg, ctx, comp))
#define RETURN_FILE_ERROR(type, path, comp)                                                        \
    return Result<decltype(Result<void>())>(ErrorUtils::type(path, comp))
#define RETURN_AUDIO_ERROR(type, details, comp)                                                    \
    return Result<decltype(Result<void>())>(ErrorUtils::type(details, comp))
#define RETURN_JSON_ERROR(type, field, details, comp)                                              \
    return Result<decltype(Result<void>())>(ErrorUtils::type(field, details, comp))

#define HANDLE_ERROR(error) ErrorHandler::getInstance().handleError(error)
#define HANDLE_ERROR_WITH_RECOVERY(error, recovery)                                                \
    ErrorHandler::getInstance().handleError(error, recovery)

} // namespace Dynamix