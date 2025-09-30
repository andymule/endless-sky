#include "ConsoleLog.h"
#include "JsonValidator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Dynamix {

    ConsoleLog::ConsoleLog() {
        // Initialize with welcome message
        LogInfo("Console initialized");
    }

    void ConsoleLog::Render(const ImVec2& windowSize) {
        if (!m_isVisible && m_animationHeight <= 0.0f) {
            return; // Completely hidden
        }

        // Animate height
        float targetHeight = m_isVisible ? m_drawerHeight : 0.0f;
        if (m_animationHeight != targetHeight) {
            float diff = targetHeight - m_animationHeight;
            float step = std::min(std::abs(diff), m_animationSpeed);
            m_animationHeight += (diff > 0) ? step : -step;
            
            // Clamp to target when close enough
            if (std::abs(diff) <= m_animationSpeed) {
                m_animationHeight = targetHeight;
            }
        }

        // Position at bottom of screen
        ImVec2 consolePos(0, windowSize.y - m_animationHeight);
        ImVec2 consoleSize(windowSize.x, m_animationHeight);

        ImGui::SetNextWindowPos(consolePos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(consoleSize, ImGuiCond_Always);

        // Console background style
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        
        if (ImGui::Begin("##Console", nullptr, CONSOLE_FLAGS)) {
            RenderHeader();
            
            // Resize grip
            if (m_isVisible) {
                ImVec2 gripPos = ImGui::GetWindowPos();
                gripPos.y += HEADER_HEIGHT;
                ImVec2 gripSize(consoleSize.x, RESIZE_GRIP_SIZE);
                
                ImGui::SetCursorPos(ImVec2(0, HEADER_HEIGHT));
                ImGui::InvisibleButton("##ResizeGrip", gripSize);
                
                if (ImGui::IsItemHovered()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    float delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y;
                    m_drawerHeight = std::clamp(m_drawerHeight - delta, m_minHeight, m_maxHeight);
                    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                }
                
                // Visual resize grip indicator
                if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    ImVec2 center(gripPos.x + gripSize.x * 0.5f, gripPos.y + gripSize.y * 0.5f);
                    ImU32 gripColor = ImGui::GetColorU32(ImGuiCol_ResizeGrip);
                    
                    for (int i = -1; i <= 1; ++i) {
                        drawList->AddLine(
                            ImVec2(center.x - 30, center.y + i * 3),
                            ImVec2(center.x + 30, center.y + i * 3),
                            gripColor, 2.0f
                        );
                    }
                }
            }
            
            // Content area
            if (m_isVisible) {
                ImVec2 contentSize(consoleSize.x, m_animationHeight - HEADER_HEIGHT - RESIZE_GRIP_SIZE);
                ImGui::SetCursorPos(ImVec2(0, HEADER_HEIGHT + RESIZE_GRIP_SIZE));
                
                if (ImGui::BeginChild("ConsoleContent", contentSize, false, ImGuiWindowFlags_HorizontalScrollbar)) {
                    RenderMessages();
                    
                    // Auto-scroll to bottom
                    if (m_scrollToBottom) {
                        ImGui::SetScrollHereY(1.0f);
                        m_scrollToBottom = false;
                    }
                }
                ImGui::EndChild();
            }
        }
        ImGui::End();
        
        ImGui::PopStyleColor(2);
    }

    void ConsoleLog::RenderHeader() {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        
        // Console title and controls
        ImGui::Text("Console");
        ImGui::SameLine();
        
        // Message count
        size_t visibleCount = 0;
        for (const auto& msg : m_messages) {
            bool show = false;
            switch (msg.type) {
                case MessageType::INFO: show = m_showInfo; break;
                case MessageType::WARNING: show = m_showWarnings; break;
                case MessageType::ERROR: show = m_showErrors; break;
                case MessageType::SUCCESS: show = m_showSuccess; break;
            }
            if (show) visibleCount++;
        }
        
        ImGui::Text("(%zu messages)", visibleCount);
        ImGui::SameLine();
        
        // Spacer to push buttons right
        float windowWidth = ImGui::GetWindowWidth();
        float buttonsWidth = 200.0f; // Approximate width of all buttons
        ImGui::SetCursorPosX(windowWidth - buttonsWidth);
        
        // Filter buttons
        if (ImGui::SmallButton(m_showInfo ? "Info" : "Info##off")) m_showInfo = !m_showInfo;
        ImGui::SameLine();
        
        if (ImGui::SmallButton(m_showWarnings ? "Warn" : "Warn##off")) m_showWarnings = !m_showWarnings;
        ImGui::SameLine();
        
        if (ImGui::SmallButton(m_showErrors ? "Error" : "Error##off")) m_showErrors = !m_showErrors;
        ImGui::SameLine();
        
        if (ImGui::SmallButton(m_showSuccess ? "OK" : "OK##off")) m_showSuccess = !m_showSuccess;
        ImGui::SameLine();
        
        // Clear button
        if (ImGui::SmallButton("Clear")) {
            Clear();
        }
        ImGui::SameLine();
        
        // Hide button
        if (ImGui::SmallButton("Hide")) {
            Hide();
        }
        
        ImGui::PopStyleColor(2);
    }

    void ConsoleLog::RenderMessages() {
        ImGuiListClipper clipper;
        
        // Count visible messages
        std::vector<size_t> visibleIndices;
        for (size_t i = 0; i < m_messages.size(); ++i) {
            bool show = false;
            switch (m_messages[i].type) {
                case MessageType::INFO: show = m_showInfo; break;
                case MessageType::WARNING: show = m_showWarnings; break;
                case MessageType::ERROR: show = m_showErrors; break;
                case MessageType::SUCCESS: show = m_showSuccess; break;
            }
            if (show) {
                visibleIndices.push_back(i);
            }
        }
        
        clipper.Begin(static_cast<int>(visibleIndices.size()));
        
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                if (i >= 0 && i < static_cast<int>(visibleIndices.size())) {
                    const auto& message = m_messages[visibleIndices[i]];
                    
                    // Timestamp
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", FormatTimestamp(message.timestamp).c_str());
                    ImGui::SameLine();
                    
                    // Icon
                    ImGui::TextColored(GetMessageColor(message.type), "%s", GetMessageIcon(message.type));
                    ImGui::SameLine();
                    
                    // Message text
                    ImGui::TextColored(GetMessageColor(message.type), "%s", message.text.c_str());
                }
            }
        }
    }

    void ConsoleLog::LogInfo(const std::string& message) {
        m_messages.emplace_back(message, MessageType::INFO);
        TrimMessages();
        
        if (m_autoScroll) {
            m_scrollToBottom = true;
        }
    }

    void ConsoleLog::LogWarning(const std::string& message) {
        m_messages.emplace_back(message, MessageType::WARNING);
        TrimMessages();
        
        if (m_autoScroll) {
            m_scrollToBottom = true;
        }
    }

    void ConsoleLog::LogError(const std::string& message) {
        m_messages.emplace_back(message, MessageType::ERROR);
        TrimMessages();
        
        if (m_autoScroll) {
            m_scrollToBottom = true;
        }
        
        // Auto-show console on errors
        if (!m_isVisible) {
            Show();
        }
    }

    void ConsoleLog::LogSuccess(const std::string& message) {
        m_messages.emplace_back(message, MessageType::SUCCESS);
        TrimMessages();
        
        if (m_autoScroll) {
            m_scrollToBottom = true;
        }
    }

    void ConsoleLog::LogValidationResult(const JsonValidator::ValidationResult& result, 
                                       const std::string& context) {
        std::string prefix = context.empty() ? "" : context + ": ";
        
        if (result.isValid && result.errors.empty() && result.warnings.empty()) {
            LogSuccess(prefix + "Validation passed");
        } else {
            // Log all errors
            for (const auto& error : result.errors) {
                LogError(prefix + error);
            }
            
            // Log all warnings
            for (const auto& warning : result.warnings) {
                LogWarning(prefix + warning);
            }
            
            if (!result.isValid) {
                LogError(prefix + "Validation failed with " + std::to_string(result.errors.size()) + " errors");
            }
        }
    }

    void ConsoleLog::Clear() {
        m_messages.clear();
        LogInfo("Console cleared");
    }

    void ConsoleLog::Show() {
        m_isVisible = true;
    }

    void ConsoleLog::Hide() {
        m_isVisible = false;
    }

    void ConsoleLog::Toggle() {
        m_isVisible = !m_isVisible;
    }

    ImVec4 ConsoleLog::GetMessageColor(MessageType type) const {
        switch (type) {
            case MessageType::INFO:    return ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
            case MessageType::WARNING: return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Orange
            case MessageType::ERROR:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
            case MessageType::SUCCESS: return ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green
            default:                   return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        }
    }

    const char* ConsoleLog::GetMessageIcon(MessageType type) const {
        switch (type) {
            case MessageType::INFO:    return "[i]";
            case MessageType::WARNING: return "[!]";
            case MessageType::ERROR:   return "[X]";
            case MessageType::SUCCESS: return "[✓]";
            default:                   return "[ ]";
        }
    }

    std::string ConsoleLog::FormatTimestamp(const std::chrono::system_clock::time_point& timestamp) const {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    void ConsoleLog::TrimMessages() {
        while (m_messages.size() > m_maxMessages) {
            m_messages.pop_front();
        }
    }

} // namespace Dynamix