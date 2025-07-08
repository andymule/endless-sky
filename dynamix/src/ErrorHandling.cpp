#include "ErrorHandling.h"
#include <algorithm>
#include <sstream>

namespace Dynamix {

    std::string Error::toString() const {
        std::ostringstream oss;
        oss << "[" << component << "] " << message;
        if (!context.empty()) {
            oss << " (Context: " << context << ")";
        }
        if (nested) {
            oss << " -> " << nested->get()->toString();
        }
        return oss.str();
    }

    bool Error::isFileSystemError() const {
        return type == ErrorType::FILE_NOT_FOUND || type == ErrorType::FILE_ACCESS_DENIED ||
               type == ErrorType::FILE_ALREADY_EXISTS || type == ErrorType::DIRECTORY_NOT_FOUND ||
               type == ErrorType::DIRECTORY_ACCESS_DENIED ||
               type == ErrorType::DIRECTORY_ALREADY_EXISTS || type == ErrorType::DISK_FULL ||
               type == ErrorType::INVALID_PATH;
    }

    bool Error::isAudioError() const {
        return type == ErrorType::AUDIO_INIT_FAILED || type == ErrorType::AUDIO_LOAD_FAILED ||
               type == ErrorType::AUDIO_FORMAT_UNSUPPORTED ||
               type == ErrorType::AUDIO_DEVICE_ERROR || type == ErrorType::AUDIO_BUFFER_OVERFLOW ||
               type == ErrorType::AUDIO_BUFFER_UNDERFLOW ||
               type == ErrorType::AUDIO_PROCESSING_ERROR;
    }

    bool Error::isDataError() const {
        return type == ErrorType::JSON_PARSE_ERROR || type == ErrorType::JSON_VALIDATION_ERROR ||
               type == ErrorType::JSON_MISSING_FIELD || type == ErrorType::JSON_INVALID_TYPE ||
               type == ErrorType::JSON_INVALID_VALUE;
    }

    bool Error::isSystemError() const {
        return type == ErrorType::MEMORY_ERROR || type == ErrorType::THREAD_ERROR ||
               type == ErrorType::TIMEOUT_ERROR || type == ErrorType::NETWORK_ERROR;
    }

    bool Error::isStateError() const {
        return type == ErrorType::INVALID_STATE || type == ErrorType::INVALID_PARAMETER ||
               type == ErrorType::INVALID_OPERATION || type == ErrorType::RESOURCE_NOT_FOUND ||
               type == ErrorType::RESOURCE_ALREADY_EXISTS || type == ErrorType::RESOURCE_IN_USE;
    }

    int Error::getSeverity() const {
        // Critical errors (severity 1)
        if (type == ErrorType::MEMORY_ERROR || type == ErrorType::AUDIO_INIT_FAILED ||
            type == ErrorType::DISK_FULL) {
            return 1;
        }

        // High severity errors (severity 2)
        if (type == ErrorType::AUDIO_DEVICE_ERROR || type == ErrorType::THREAD_ERROR ||
            type == ErrorType::AUDIO_LOAD_FAILED) {
            return 2;
        }

        // Medium severity errors (severity 3)
        if (type == ErrorType::FILE_ACCESS_DENIED || type == ErrorType::DIRECTORY_ACCESS_DENIED ||
            type == ErrorType::AUDIO_PROCESSING_ERROR) {
            return 3;
        }

        // Low severity errors (severity 4)
        if (type == ErrorType::INVALID_PARAMETER || type == ErrorType::JSON_MISSING_FIELD) {
            return 4;
        }

        // Default severity (severity 5)
        return 5;
    }

    bool Error::isRetryable() const {
        // Network errors are usually retryable
        if (type == ErrorType::NETWORK_ERROR) {
            return true;
        }

        // Audio buffer errors can be retried
        if (type == ErrorType::AUDIO_BUFFER_OVERFLOW || type == ErrorType::AUDIO_BUFFER_UNDERFLOW) {
            return true;
        }

        // Timeout errors can be retried
        if (type == ErrorType::TIMEOUT_ERROR) {
            return true;
        }

        // File access errors might be temporary
        if (type == ErrorType::FILE_ACCESS_DENIED || type == ErrorType::DIRECTORY_ACCESS_DENIED) {
            return true;
        }

        return false;
    }

    void ErrorHandler::handleError(const Error& error) {
        updateStats(error);
        logError(error);

        switch (m_policy) {
            case Policy::LOG_ONLY:
                // Only log, no further action
                break;
            case Policy::NOTIFY_USER:
                notifyUser(error);
                break;
            case Policy::ATTEMPT_RECOVERY:
                if (isRecoverable(error)) {
                    // Attempt automatic recovery
                    try {
                        // Default recovery: just log and notify
                        notifyUser(error);
                    } catch (const std::exception& e) {
                        Error recoveryError =
                            ErrorUtils::fromException(e, "Error recovery failed", error.component);
                        logError(recoveryError);
                    }
                } else {
                    notifyUser(error);
                }
                break;
            case Policy::THROW_EXCEPTION:
                if (error.getSeverity() <= 2) { // Critical or high severity
                    throw std::runtime_error(error.toString());
                } else {
                    notifyUser(error);
                }
                break;
        }
    }

    void ErrorHandler::handleError(const Error& error, std::function<void()> recovery) {
        updateStats(error);
        logError(error);

        if (isRecoverable(error)) {
            try {
                recovery();
            } catch (const std::exception& e) {
                // If recovery fails, log the recovery error
                Error recoveryError =
                    ErrorUtils::fromException(e, "Error recovery failed", error.component);
                logError(recoveryError);
                notifyUser(recoveryError);
            }
        } else {
            notifyUser(error);
        }
    }

    bool ErrorHandler::isRecoverable(const Error& error) const {
        // File system errors that might be recoverable
        if (error.isFileSystemError()) {
            return error.type == ErrorType::FILE_ACCESS_DENIED ||
                   error.type == ErrorType::DIRECTORY_ACCESS_DENIED ||
                   error.type == ErrorType::DISK_FULL;
        }

        // Audio errors that might be recoverable
        if (error.isAudioError()) {
            return error.type == ErrorType::AUDIO_BUFFER_OVERFLOW ||
                   error.type == ErrorType::AUDIO_BUFFER_UNDERFLOW ||
                   error.type == ErrorType::AUDIO_PROCESSING_ERROR;
        }

        // Parameter errors are usually recoverable
        if (error.type == ErrorType::INVALID_PARAMETER) {
            return true;
        }

        // Network errors are often recoverable
        if (error.type == ErrorType::NETWORK_ERROR) {
            return true;
        }

        // Timeout errors can be retried
        if (error.type == ErrorType::TIMEOUT_ERROR) {
            return true;
        }

        return false;
    }

    std::string ErrorHandler::getUserMessage(const Error& error) const {
        switch (error.type) {
            case ErrorType::FILE_NOT_FOUND:
                return "File not found: " + error.context;
            case ErrorType::FILE_ACCESS_DENIED:
                return "Access denied to file: " + error.context;
            case ErrorType::FILE_ALREADY_EXISTS:
                return "File already exists: " + error.context;
            case ErrorType::DIRECTORY_NOT_FOUND:
                return "Directory not found: " + error.context;
            case ErrorType::DIRECTORY_ACCESS_DENIED:
                return "Access denied to directory: " + error.context;
            case ErrorType::DIRECTORY_ALREADY_EXISTS:
                return "Directory already exists: " + error.context;
            case ErrorType::DISK_FULL:
                return "Disk is full. Please free up some space.";
            case ErrorType::INVALID_PATH:
                return "Invalid path: " + error.context;
            case ErrorType::AUDIO_INIT_FAILED:
                return "Failed to initialize audio system: " + error.context;
            case ErrorType::AUDIO_LOAD_FAILED:
                return "Failed to load audio file: " + error.context;
            case ErrorType::AUDIO_FORMAT_UNSUPPORTED:
                return "Unsupported audio format: " + error.context;
            case ErrorType::AUDIO_DEVICE_ERROR:
                return "Audio device error: " + error.context;
            case ErrorType::JSON_PARSE_ERROR:
                return "Failed to parse JSON file: " + error.context;
            case ErrorType::JSON_VALIDATION_ERROR:
                return "Invalid JSON data: " + error.context;
            case ErrorType::JSON_MISSING_FIELD:
                return "Missing required field: " + error.context;
            case ErrorType::INVALID_PARAMETER:
                return "Invalid parameter: " + error.context;
            case ErrorType::INVALID_OPERATION:
                return "Invalid operation: " + error.context;
            case ErrorType::RESOURCE_NOT_FOUND:
                return "Resource not found: " + error.context;
            case ErrorType::RESOURCE_ALREADY_EXISTS:
                return "Resource already exists: " + error.context;
            case ErrorType::MEMORY_ERROR:
                return "Memory error: " + error.context;
            case ErrorType::THREAD_ERROR:
                return "Thread error: " + error.context;
            case ErrorType::TIMEOUT_ERROR:
                return "Operation timed out: " + error.context;
            case ErrorType::NETWORK_ERROR:
                return "Network error: " + error.context;
            case ErrorType::UNKNOWN_ERROR:
            default:
                return "An unexpected error occurred: " + error.message;
        }
    }

    void ErrorHandler::setErrorCallback(std::function<void(const std::string&)> callback) {
        m_errorCallback = callback;
    }

    ErrorHandler::ErrorStats ErrorHandler::getErrorStats() const {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        return m_stats;
    }

    void ErrorHandler::clearErrorStats() {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats = ErrorStats{};
    }

    void ErrorHandler::setPolicy(Policy policy) { m_policy = policy; }

    void ErrorHandler::logError(const Error& error) {
        std::string logMessage = error.toString();

        // Use component-specific logging if available
        if (!error.component.empty()) {
            LOG_ERROR_COMP(error.component, logMessage);
        } else {
            LOG_ERROR(logMessage);
        }
    }

    void ErrorHandler::notifyUser(const Error& error) {
        if (m_errorCallback) {
            std::string userMessage = getUserMessage(error);
            m_errorCallback(userMessage);
        }
    }

    void ErrorHandler::updateStats(const Error& error) const {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.totalErrors++;
        m_stats.errorTypeCounts[error.type]++;

        if (isRecoverable(error)) {
            m_stats.recoverableErrors++;
        } else {
            m_stats.unrecoverableErrors++;
        }
    }

    // ErrorUtils implementation
    namespace ErrorUtils {

        Error fileNotFound(const std::string& path, const std::string& component) {
            return Error(ErrorType::FILE_NOT_FOUND, "File not found", path, component);
        }

        Error fileAccessDenied(const std::string& path, const std::string& component) {
            return Error(ErrorType::FILE_ACCESS_DENIED, "Access denied to file", path, component);
        }

        Error fileAlreadyExists(const std::string& path, const std::string& component) {
            return Error(ErrorType::FILE_ALREADY_EXISTS, "File already exists", path, component);
        }

        Error directoryNotFound(const std::string& path, const std::string& component) {
            return Error(ErrorType::DIRECTORY_NOT_FOUND, "Directory not found", path, component);
        }

        Error directoryAccessDenied(const std::string& path, const std::string& component) {
            return Error(ErrorType::DIRECTORY_ACCESS_DENIED, "Access denied to directory", path,
                         component);
        }

        Error directoryAlreadyExists(const std::string& path, const std::string& component) {
            return Error(ErrorType::DIRECTORY_ALREADY_EXISTS, "Directory already exists", path,
                         component);
        }

        Error audioInitFailed(const std::string& details, const std::string& component) {
            return Error(ErrorType::AUDIO_INIT_FAILED, "Audio initialization failed", details,
                         component);
        }

        Error audioLoadFailed(const std::string& path, const std::string& details,
                              const std::string& component) {
            return Error(ErrorType::AUDIO_LOAD_FAILED, "Failed to load audio file",
                         path + " - " + details, component);
        }

        Error audioFormatUnsupported(const std::string& format, const std::string& component) {
            return Error(ErrorType::AUDIO_FORMAT_UNSUPPORTED, "Unsupported audio format", format,
                         component);
        }

        Error jsonParseError(const std::string& path, const std::string& details,
                             const std::string& component) {
            return Error(ErrorType::JSON_PARSE_ERROR, "JSON parse error", path + " - " + details,
                         component);
        }

        Error jsonValidationError(const std::string& field, const std::string& details,
                                  const std::string& component) {
            return Error(ErrorType::JSON_VALIDATION_ERROR, "JSON validation error",
                         field + " - " + details, component);
        }

        Error jsonMissingField(const std::string& field, const std::string& component) {
            return Error(ErrorType::JSON_MISSING_FIELD, "Missing required JSON field", field,
                         component);
        }

        Error invalidParameter(const std::string& param, const std::string& value,
                               const std::string& component) {
            return Error(ErrorType::INVALID_PARAMETER, "Invalid parameter value",
                         param + " = " + value, component);
        }

        Error invalidOperation(const std::string& operation, const std::string& reason,
                               const std::string& component) {
            return Error(ErrorType::INVALID_OPERATION, "Invalid operation",
                         operation + " - " + reason, component);
        }

        Error resourceNotFound(const std::string& resource, const std::string& component) {
            return Error(ErrorType::RESOURCE_NOT_FOUND, "Resource not found", resource, component);
        }

        Error resourceAlreadyExists(const std::string& resource, const std::string& component) {
            return Error(ErrorType::RESOURCE_ALREADY_EXISTS, "Resource already exists", resource,
                         component);
        }

        Error memoryError(const std::string& details, const std::string& component) {
            return Error(ErrorType::MEMORY_ERROR, "Memory error", details, component);
        }

        Error networkError(const std::string& details, const std::string& component) {
            return Error(ErrorType::NETWORK_ERROR, "Network error", details, component);
        }

        Error threadError(const std::string& details, const std::string& component) {
            return Error(ErrorType::THREAD_ERROR, "Thread error", details, component);
        }

        Error timeoutError(const std::string& details, const std::string& component) {
            return Error(ErrorType::TIMEOUT_ERROR, "Timeout error", details, component);
        }

        Error fromException(const std::exception& e, const std::string& context,
                            const std::string& component) {
            return Error(ErrorType::UNKNOWN_ERROR, "Exception occurred", context + " - " + e.what(),
                         component);
        }

        Error fromFilesystemError(const std::filesystem::filesystem_error& e,
                                  const std::string& component) {
            std::string path = e.path1().string();
            if (!e.path2().empty()) {
                path += " -> " + e.path2().string();
            }

            // Map filesystem error codes to our error types
            if (e.code().value() == static_cast<int>(std::errc::no_such_file_or_directory)) {
                return fileNotFound(path, component);
            } else if (e.code().value() == static_cast<int>(std::errc::permission_denied)) {
                return fileAccessDenied(path, component);
            } else if (e.code().value() == static_cast<int>(std::errc::file_exists)) {
                return fileAlreadyExists(path, component);
            } else if (e.code().value() == static_cast<int>(std::errc::not_a_directory)) {
                return directoryNotFound(path, component);
            } else {
                return Error(ErrorType::UNKNOWN_ERROR, "Filesystem error", path + " - " + e.what(),
                             component);
            }
        }

        Error withNested(const Error& outer, const Error& inner) {
            return Error(outer.type, outer.message, outer.context, outer.component,
                         std::make_shared<Error>(inner));
        }

        Error withContext(const Error& error, const std::string& additionalContext) {
            std::string newContext = error.context;
            if (!additionalContext.empty()) {
                if (!newContext.empty()) {
                    newContext += " - ";
                }
                newContext += additionalContext;
            }
            return Error(error.type, error.message, newContext, error.component);
        }

    } // namespace ErrorUtils

} // namespace Dynamix