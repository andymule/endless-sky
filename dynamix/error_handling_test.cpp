#include "src/ErrorHandling.h"
#include <cassert>
#include <iostream>

using namespace AudioTester;

// Mock logger for testing
namespace AudioTester {
    void Logger::error(const std::string& message, const std::string& component) {
        std::cout << "[ERROR][" << component << "] " << message << std::endl;
    }
    void Logger::warn(const std::string& message, const std::string& component) {
        std::cout << "[WARN][" << component << "] " << message << std::endl;
    }
    void Logger::info(const std::string& message, const std::string& component) {
        std::cout << "[INFO][" << component << "] " << message << std::endl;
    }
    void Logger::debug(const std::string& message, const std::string& component) {
        std::cout << "[DEBUG][" << component << "] " << message << std::endl;
    }
    void Logger::trace(const std::string& message, const std::string& component) {
        std::cout << "[TRACE][" << component << "] " << message << std::endl;
    }
} // namespace AudioTester

void testErrorCreation() {
    std::cout << "\n=== Testing Error Creation ===" << std::endl;

    // Test basic error creation
    Error fileError = ErrorUtils::fileNotFound("/path/to/file.txt", "FileLoader");
    assert(fileError.type == ErrorType::FILE_NOT_FOUND);
    assert(fileError.message == "File not found");
    assert(fileError.context == "/path/to/file.txt");
    assert(fileError.component == "FileLoader");

    // Test error type checking
    assert(fileError.isFileSystemError());
    assert(!fileError.isAudioError());
    assert(!fileError.isDataError());

    // Test severity
    assert(fileError.getSeverity() == 5); // Default severity

    // Test retryable
    assert(!fileError.isRetryable());

    std::cout << "✓ Error creation tests passed" << std::endl;
}

void testResultTemplate() {
    std::cout << "\n=== Testing Result Template ===" << std::endl;

    // Test success result
    Result<std::string> successResult = Result<std::string>("test value");
    assert(successResult.isSuccess());
    assert(!successResult.isError());
    assert(successResult.value() == "test value");

    // Test error result
    Error testError = ErrorUtils::fileNotFound("test.txt", "TestComponent");
    Result<std::string> errorResult = Result<std::string>(testError);
    assert(errorResult.isError());
    assert(!errorResult.isSuccess());
    assert(errorResult.error().type == ErrorType::FILE_NOT_FOUND);

    // Test valueOr
    assert(successResult.valueOr("default") == "test value");
    assert(errorResult.valueOr("default") == "default");

    // Test move semantics
    Result<std::string> movedResult = std::move(successResult);
    assert(movedResult.isSuccess());
    assert(movedResult.value() == "test value");

    std::cout << "✓ Result template tests passed" << std::endl;
}

void testFunctionalProgramming() {
    std::cout << "\n=== Testing Functional Programming Methods ===" << std::endl;

    // Test map
    Result<int> numberResult = Result<int>(42);
    auto stringResult = numberResult.map([](const int& n) { return std::to_string(n); });
    assert(stringResult.isSuccess());
    assert(stringResult.value() == "42");

    // Test map with error
    Error mapError = ErrorUtils::invalidParameter("test", "value", "TestComponent");
    Result<int> errorNumberResult = Result<int>(mapError);
    auto errorStringResult = errorNumberResult.map([](const int& n) { return std::to_string(n); });
    assert(errorStringResult.isError());
    assert(errorStringResult.error().type == ErrorType::INVALID_PARAMETER);

    // Test andThen
    auto chainedResult = numberResult.andThen([](const int& n) {
        if (n > 0) {
            return Result<std::string>(std::to_string(n * 2));
        } else {
            return Result<std::string>(
                ErrorUtils::invalidParameter("number", std::to_string(n), "TestComponent"));
        }
    });
    assert(chainedResult.isSuccess());
    assert(chainedResult.value() == "84");

    // Test match
    std::string matchResult =
        numberResult.match([](const int& n) { return "Success: " + std::to_string(n); },
                           [](const Error& e) { return "Error: " + e.message; });
    assert(matchResult == "Success: 42");

    std::cout << "✓ Functional programming tests passed" << std::endl;
}

void testErrorHandler() {
    std::cout << "\n=== Testing Error Handler ===" << std::endl;

    ErrorHandler& handler = ErrorHandler::getInstance();

    // Test error handling
    Error testError = ErrorUtils::fileNotFound("test.txt", "TestComponent");
    handler.handleError(testError);

    // Test error statistics
    auto stats = handler.getErrorStats();
    assert(stats.totalErrors == 1);
    assert(stats.errorTypeCounts[ErrorType::FILE_NOT_FOUND] == 1);

    // Test policy changes
    handler.setPolicy(ErrorHandler::Policy::LOG_ONLY);
    handler.handleError(ErrorUtils::invalidParameter("test", "value", "TestComponent"));

    stats = handler.getErrorStats();
    assert(stats.totalErrors == 2);

    // Clear stats
    handler.clearErrorStats();
    stats = handler.getErrorStats();
    assert(stats.totalErrors == 0);

    std::cout << "✓ Error handler tests passed" << std::endl;
}

void testErrorChaining() {
    std::cout << "\n=== Testing Error Chaining ===" << std::endl;

    // Test nested errors
    Error innerError = ErrorUtils::fileNotFound("inner.txt", "InnerComponent");
    Error outerError = ErrorUtils::fromException(std::runtime_error("outer error"), "outer context",
                                                 "OuterComponent");

    Error chainedError = ErrorUtils::withNested(outerError, innerError);
    assert(chainedError.nested.has_value());
    assert(chainedError.nested->get()->type == ErrorType::FILE_NOT_FOUND);

    // Test context addition
    Error contextError = ErrorUtils::withContext(innerError, "additional context");
    assert(contextError.context.find("additional context") != std::string::npos);

    std::cout << "✓ Error chaining tests passed" << std::endl;
}

void testErrorRecovery() {
    std::cout << "\n=== Testing Error Recovery ===" << std::endl;

    ErrorHandler& handler = ErrorHandler::getInstance();

    // Test recoverable error
    Error recoverableError = ErrorUtils::networkError("connection failed", "NetworkComponent");
    assert(recoverableError.isRetryable());

    bool recoveryCalled = false;
    handler.handleError(recoverableError, [&recoveryCalled]() { recoveryCalled = true; });
    assert(recoveryCalled);

    // Test non-recoverable error
    Error nonRecoverableError = ErrorUtils::memoryError("out of memory", "MemoryComponent");
    assert(!nonRecoverableError.isRetryable());

    recoveryCalled = false;
    handler.handleError(nonRecoverableError, [&recoveryCalled]() { recoveryCalled = true; });
    assert(!recoveryCalled);

    std::cout << "✓ Error recovery tests passed" << std::endl;
}

void testErrorSeverity() {
    std::cout << "\n=== Testing Error Severity ===" << std::endl;

    // Test critical errors
    Error memoryError = ErrorUtils::memoryError("out of memory", "MemoryComponent");
    assert(memoryError.getSeverity() == 1);

    // Test high severity errors
    Error audioError = ErrorUtils::audioInitFailed("device not found", "AudioComponent");
    assert(audioError.getSeverity() == 1);

    // Test medium severity errors
    Error accessError = ErrorUtils::fileAccessDenied("test.txt", "FileComponent");
    assert(accessError.getSeverity() == 3);

    // Test low severity errors
    Error paramError = ErrorUtils::invalidParameter("test", "value", "TestComponent");
    assert(paramError.getSeverity() == 4);

    std::cout << "✓ Error severity tests passed" << std::endl;
}

int main() {
    std::cout << "Starting Error Handling System Tests..." << std::endl;

    try {
        testErrorCreation();
        testResultTemplate();
        testFunctionalProgramming();
        testErrorHandler();
        testErrorChaining();
        testErrorRecovery();
        testErrorSeverity();

        std::cout << "\n🎉 All tests passed! The error handling system is working correctly."
                  << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}