#pragma once

#include <stdexcept>
#include <string>

namespace AudioTester {

    // Simple error types for the audio tester
    class AudioError : public std::runtime_error {
    public:
        AudioError(const std::string& message) : std::runtime_error(message) {}
    };

    class FilterError : public std::runtime_error {
    public:
        FilterError(const std::string& message) : std::runtime_error(message) {}
    };

} // namespace AudioTester