#pragma once

#include "imgui.h"
#include <chrono>
#include <deque>
#include <string>
#include <vector>

#include "JsonValidator.h"

namespace Dynamix {

    /**
     * Scrolling, resizable, hidable console log drawer for system messages
     * 
     * Features:
     * - Scrollable message history with auto-scroll to bottom
     * - Resizable drawer height
     * - Show/hide with smooth animation
     * - Color-coded message types (Info, Warning, Error)
     * - Clear all messages
     * - Copy messages to clipboard
     * - Timestamps for each message
     */
    class ConsoleLog {
    public:
        enum class MessageType {
            INFO,
            WARNING,
            ERROR,
            SUCCESS
        };

        struct LogMessage {
            std::string text;
            MessageType type;
            std::chrono::system_clock::time_point timestamp;

            LogMessage(const std::string& msg, MessageType msgType) 
                : text(msg), type(msgType), timestamp(std::chrono::system_clock::now()) {}
        };

        ConsoleLog();
        ~ConsoleLog() = default;

        // Main rendering function - call this at the end of your main window rendering
        void Render(const ImVec2& windowSize);

        // Message logging
        void LogInfo(const std::string& message);
        void LogWarning(const std::string& message);
        void LogError(const std::string& message);
        void LogSuccess(const std::string& message);

        // Batch logging for validation results
        void LogValidationResult(const JsonValidator::ValidationResult& result, 
                               const std::string& context = "");

        // Controls
        void Clear();
        void Show();
        void Hide();
        void Toggle();
        bool IsVisible() const { return m_isVisible; }

        // Configuration
        void SetMaxMessages(size_t maxMessages) { m_maxMessages = maxMessages; }
        void SetAutoScroll(bool autoScroll) { m_autoScroll = autoScroll; }

    private:
        // Message storage
        std::deque<LogMessage> m_messages;
        size_t m_maxMessages = 1000;

        // UI state
        bool m_isVisible = false;
        float m_drawerHeight = 200.0f;
        float m_minHeight = 100.0f;
        float m_maxHeight = 400.0f;
        bool m_autoScroll = true;
        bool m_scrollToBottom = false;

        // Animation
        float m_animationHeight = 0.0f;
        float m_animationSpeed = 8.0f; // pixels per frame
        
        // Filtering
        bool m_showInfo = true;
        bool m_showWarnings = true;
        bool m_showErrors = true;
        bool m_showSuccess = true;

        // Helper methods
        ImVec4 GetMessageColor(MessageType type) const;
        const char* GetMessageIcon(MessageType type) const;
        std::string FormatTimestamp(const std::chrono::system_clock::time_point& timestamp) const;
        void RenderHeader();
        void RenderMessages();
        void RenderControls();
        void TrimMessages();

        // Constants
        static constexpr float HEADER_HEIGHT = 30.0f;
        static constexpr float RESIZE_GRIP_SIZE = 10.0f;
        static constexpr ImGuiWindowFlags CONSOLE_FLAGS = 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;
    };

} // namespace Dynamix