# Error Handling System Guide

## Overview

The new error handling system provides consistent, type-safe error handling across the entire codebase. It replaces the inconsistent mix of return codes, exceptions, and logging that was previously used.

## Key Components

### 1. Error Types (`ErrorType`)

Categorized error types for programmatic handling:

```cpp
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
```

### 2. Error Structure (`Error`)

Detailed error information with context:

```cpp
struct Error {
    ErrorType type;
    std::string message;
    std::string context;
    std::string component;
    std::optional<std::shared_ptr<Error>> nested;
    
    // Helper methods
    bool is(ErrorType t) const;
    bool isFileSystemError() const;
    bool isAudioError() const;
    bool isDataError() const;
    std::string toString() const;
};
```

### 3. Result Template (`Result<T>`)

Type-safe error handling that can hold either a value or an error:

```cpp
template<typename T>
class Result {
public:
    // Constructors
    Result(const T& value);
    Result(const Error& error);
    
    // Status checking
    bool isSuccess() const;
    bool isError() const;
    
    // Value access
    const T& value() const;  // Throws if error
    T& value();              // Throws if error
    T valueOr(const T& defaultValue) const;
    
    // Error access
    const Error& error() const;  // Throws if success
    
    // Functional programming methods
    template<typename U>
    Result<U> map(std::function<U(const T&)> fn) const;
    
    template<typename U>
    Result<U> andThen(std::function<Result<U>(const T&)> fn) const;
};
```

### 4. Error Handler (`ErrorHandler`)

Centralized error processing with logging and recovery:

```cpp
class ErrorHandler {
public:
    static ErrorHandler& getInstance();
    
    void handleError(const Error& error);
    void handleError(const Error& error, std::function<void()> recovery);
    bool isRecoverable(const Error& error) const;
    std::string getUserMessage(const Error& error) const;
    void setErrorCallback(std::function<void(const std::string&)> callback);
};
```

### 5. Error Utilities (`ErrorUtils`)

Helper functions for creating common errors:

```cpp
namespace ErrorUtils {
    // File system errors
    Error fileNotFound(const std::string& path, const std::string& component = "");
    Error fileAccessDenied(const std::string& path, const std::string& component = "");
    Error directoryNotFound(const std::string& path, const std::string& component = "");
    
    // Audio errors
    Error audioInitFailed(const std::string& details, const std::string& component = "");
    Error audioLoadFailed(const std::string& path, const std::string& details, const std::string& component = "");
    Error audioFormatUnsupported(const std::string& format, const std::string& component = "");
    
    // JSON errors
    Error jsonParseError(const std::string& path, const std::string& details, const std::string& component = "");
    Error jsonValidationError(const std::string& field, const std::string& details, const std::string& component = "");
    Error jsonMissingField(const std::string& field, const std::string& component = "");
    
    // Parameter errors
    Error invalidParameter(const std::string& param, const std::string& value, const std::string& component = "");
    Error invalidOperation(const std::string& operation, const std::string& reason, const std::string& component = "");
    
    // Resource errors
    Error resourceNotFound(const std::string& resource, const std::string& component = "");
    Error resourceAlreadyExists(const std::string& resource, const std::string& component = "");
    
    // Conversion utilities
    Error fromException(const std::exception& e, const std::string& context, const std::string& component = "");
    Error fromFilesystemError(const std::filesystem::filesystem_error& e, const std::string& component = "");
}
```

## Usage Examples

### Basic Error Handling

```cpp
// BEFORE: Inconsistent error handling
bool loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: " + path);
        return false;
    }
    // ... process file ...
    return true;
}

// AFTER: Type-safe error handling
Result<std::string> loadFile(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return Result<std::string>(ErrorUtils::fileNotFound(path, "FileLoader"));
        }
        
        std::ifstream file(path);
        if (!file.is_open()) {
            return Result<std::string>(ErrorUtils::fileAccessDenied(path, "FileLoader"));
        }
        
        std::string content;
        // ... read file content ...
        
        return Result<std::string>(content);
        
    } catch (const std::exception& e) {
        return Result<std::string>(ErrorUtils::fromException(e, "Loading file", "FileLoader"));
    }
}
```

### Error Propagation

```cpp
// BEFORE: Manual error checking
bool processSong(const std::string& songPath) {
    nlohmann::json json;
    if (!loadJsonFile(songPath, json)) {
        return false;  // Lost error context
    }
    
    if (!validateJson(json)) {
        return false;  // Lost error context
    }
    
    if (!parseSong(json)) {
        return false;  // Lost error context
    }
    
    return true;
}

// AFTER: Automatic error propagation
Result<Song> processSong(const std::string& songPath) {
    return loadJsonFile(songPath)
        .andThen(validateJson)
        .andThen(parseSong);
}
```

### Error Recovery

```cpp
Result<Song> loadSongWithRecovery(const std::filesystem::path& songFolder) {
    auto result = loadSong(songFolder);
    
    if (result.isError()) {
        const auto& error = result.error();
        
        if (error.isFileSystemError()) {
            // Try to create missing files or directories
            HANDLE_ERROR_WITH_RECOVERY(error, [&]() {
                LOG_INFO("Attempting to recover from filesystem error");
                // Recovery logic here
            });
        } else if (error.isDataError()) {
            // Try to repair corrupted JSON or use defaults
            HANDLE_ERROR_WITH_RECOVERY(error, [&]() {
                LOG_INFO("Attempting to recover from data error");
                // Recovery logic here
            });
        } else {
            // Log non-recoverable errors
            HANDLE_ERROR(error);
        }
    }
    
    return result;
}
```

### UI Integration

```cpp
// Set up error callback for UI notifications
ErrorHandler::getInstance().setErrorCallback([](const std::string& message) {
    // Update UI with error message
    showErrorDialog(message);
});

// Use in methods
Result<Song> loadSong(const std::string& path) {
    auto result = loadSongInternal(path);
    
    if (result.isError()) {
        // This will automatically log and notify the UI
        HANDLE_ERROR(result.error());
    }
    
    return result;
}
```

## Migration Guide

### Phase 1: Add Error Handling Headers

1. Include the new error handling headers in your files:
```cpp
#include "ErrorHandling.h"
```

2. Update CMakeLists.txt to include the new source files:
```cmake
target_sources(dynamix PRIVATE
    src/ErrorHandling.cpp
    # ... other files ...
)
```

### Phase 2: Refactor Return Types

1. Identify methods that return `bool` for error status
2. Change return type to `Result<T>` where T is the actual return type
3. Replace error logging with proper error creation
4. Update callers to handle the new return type

### Phase 3: Replace Exception Handling

1. Replace generic exception catches with specific error types
2. Use `ErrorUtils::fromException()` for unexpected exceptions
3. Use `ErrorUtils::fromFilesystemError()` for filesystem errors

### Phase 4: Add Error Recovery

1. Identify recoverable error scenarios
2. Implement recovery logic using `HANDLE_ERROR_WITH_RECOVERY`
3. Test recovery mechanisms

### Phase 5: UI Integration

1. Set up error callback for user notifications
2. Replace manual error dialogs with centralized error handling
3. Test user experience with various error scenarios

## Benefits

### 1. Type Safety
- No more confusion between return values and error codes
- Compile-time checking of error handling
- Impossible to ignore errors accidentally

### 2. Consistency
- All errors follow the same structure
- Consistent logging and user notification
- Standardized error messages

### 3. Maintainability
- Clear error categorization
- Easy to add new error types
- Centralized error handling logic

### 4. Debugging
- Rich error context information
- Error chaining for complex scenarios
- Component-specific error tracking

### 5. User Experience
- User-friendly error messages
- Automatic error recovery where possible
- Consistent error presentation

## Best Practices

### 1. Error Creation
- Always provide meaningful context
- Use appropriate error types
- Include component name for debugging

### 2. Error Handling
- Check for errors immediately after operations
- Use `andThen()` for chaining operations
- Use `map()` for transforming successful results

### 3. Error Recovery
- Only attempt recovery for recoverable errors
- Log recovery attempts
- Provide fallback behavior when possible

### 4. UI Integration
- Set up error callback early in application startup
- Use user-friendly messages for UI display
- Log technical details separately

### 5. Testing
- Test both success and error paths
- Verify error messages are appropriate
- Test error recovery mechanisms

## Migration Checklist

- [ ] Add ErrorHandling.h and ErrorHandling.cpp to project
- [ ] Update CMakeLists.txt to include new files
- [ ] Identify methods with boolean error returns
- [ ] Refactor one method at a time to use Result<T>
- [ ] Update callers to handle new return types
- [ ] Replace exception handling with error types
- [ ] Add error recovery where appropriate
- [ ] Set up UI error callback
- [ ] Test error scenarios
- [ ] Update documentation 