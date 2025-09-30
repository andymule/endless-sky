#include "SongView.h"
#include "AudioController.h"
#include "AudioState.h"
#include "AudioSystem.h"
#include "Logger.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>

namespace Dynamix {
    namespace Views {

        SongView::SongView(Dynamix::AudioController* controller) : m_controller(controller) {
            // Initialize dialog input
            std::fill(std::begin(m_newSongName), std::end(m_newSongName), 0);
        }

        void SongView::Render() {
            RenderTrackControls();

            // Show dialogs
            if (m_showNewSongDialog) {
                RenderNewSongDialog();
            }
        }

        void SongView::RenderTrackControls() {
            // Guard against null controller
            if (!m_controller) {
                ImGui::Text("No controller available");
                return;
            }

            // Get tracks directly from the folder contents
            std::string currentSong = m_controller->getCurrentSong();
            if (currentSong.empty()) {
                ImGui::Text("No song loaded");
                return;
            }

            // Get the song folder path
            std::filesystem::path songFolderPath = m_controller->getCurrentSongFolderPath();
            if (songFolderPath.empty()) {
                ImGui::Text("No song folder found");
                return;
            }

            // Discover tracks in the folder (this is the source of truth)
            std::vector<std::string> trackFiles;
            try {
                for (const auto& entry : std::filesystem::directory_iterator(songFolderPath)) {
                    if (entry.is_regular_file()) {
                        auto filepath = entry.path().string();
                        if (m_controller->isSupportedFile(filepath)) {
                            trackFiles.push_back(entry.path().filename().string());
                        }
                    }
                }
            } catch (const std::exception& e) {
                ImGui::Text("Error reading folder: %s", e.what());
                return;
            }

            // Sort tracks alphabetically
            std::sort(trackFiles.begin(), trackFiles.end());

            ImGui::Text("Tracks (%d):", static_cast<int>(trackFiles.size()));
            for (size_t i = 0; i < trackFiles.size(); i++) {
                const auto& trackFile = trackFiles[i];
                ImGui::PushID(static_cast<int>(i));

                // Effects dropdown on same line as volume
                // Find the actual track index by filename to ensure we get the right volume
                const auto& state = m_controller->getState();
                int actualTrackIndex = m_controller->findTrackByFilename(trackFile);

                // Create unique ID for this track's effects expanded state
                std::string expandedId = "expand_track_" + std::to_string(actualTrackIndex);

                // Get or initialize expanded state (shared between dropdown and effects display)
                static std::map<std::string, bool> trackExpandedStates;
                bool& isExpanded = trackExpandedStates[expandedId];

                if (actualTrackIndex >= 0) {
                    // Show dropdown arrow (no text, just triangle) on the far left
                    ImGui::PushID(("track_effects_" + std::to_string(actualTrackIndex)).c_str());

                    // Color the arrow based on whether any effects are enabled
                    const auto& audioSystem = m_controller->getAudioSystem();
                    const auto& filters = audioSystem.getFilters(actualTrackIndex);
                    bool hasEnabledEffects = false;
                    for (const auto& [filterName, filterInstance] : filters) {
                        if (filterInstance.enabled) {
                            hasEnabledEffects = true;
                            break;
                        }
                    }

                    if (hasEnabledEffects) {
                        ImGui::PushStyleColor(
                            ImGuiCol_Text,
                            ImVec4(0.4f, 0.8f, 0.4f, 1.0f)); // Green when effects enabled
                    } else {
                        ImGui::PushStyleColor(
                            ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Gray when no effects
                    }

                    // Dropdown arrow (TreeNode style but manual)
                    if (ImGui::ArrowButton("##arrow",
                                           isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                        isExpanded = !isExpanded;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopID();

                    ImGui::SameLine();
                }

                // Volume slider with track name as overlay
                // Get volume from AudioSystem (single source of truth), otherwise default to 1.0
                float volume = 1.0f;

                if (actualTrackIndex >= 0 &&
                    actualTrackIndex <
                        static_cast<int>(m_controller->getAudioSystem().getTrackCount())) {
                    volume = m_controller->getAudioSystem().getTrackVolume(actualTrackIndex);
                }

                // Create track title with keyboard shortcut
                std::string keyText = (i < 9) ? std::to_string(i + 1) : "0";

                // Remove file extension and capitalize
                std::string trackName = trackFile;
                size_t dotPos = trackName.find_last_of('.');
                if (dotPos != std::string::npos) {
                    trackName = trackName.substr(0, dotPos);
                }

                // Convert to uppercase
                std::transform(trackName.begin(), trackName.end(), trackName.begin(), ::toupper);

                // Calculate slider width to span the full window width, but reserve space for X
                // button
                float sliderWidth =
                    ImGui::GetContentRegionAvail().x - 30.0f; // Reserve 30px for X button

                // Set the slider width
                ImGui::SetNextItemWidth(sliderWidth);

                if (ImGui::SliderFloat("##volume", &volume, 0.0f, 1.0f, "")) {
                    // Use the actual track index, not the loop index
                    if (actualTrackIndex >= 0) {
                        m_controller->setTrackVolume(actualTrackIndex, volume);
                    }
                }

                // Draw track name as overlay in the middle of the slider
                ImVec2 sliderMin = ImGui::GetItemRectMin();
                ImVec2 sliderMax = ImGui::GetItemRectMax();
                ImVec2 sliderCenter =
                    ImVec2((sliderMin.x + sliderMax.x) * 0.5f, (sliderMin.y + sliderMax.y) * 0.5f);

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 textSize = ImGui::CalcTextSize(trackName.c_str());
                ImVec2 textPos =
                    ImVec2(sliderCenter.x - textSize.x * 0.5f, sliderCenter.y - textSize.y * 0.5f);

                // Draw text with a subtle background for better readability
                drawList->AddRectFilled(
                    ImVec2(textPos.x - 2, textPos.y - 1),
                    ImVec2(textPos.x + textSize.x + 2, textPos.y + textSize.y + 1),
                    IM_COL32(0, 0, 0, 100) // Semi-transparent black background
                );
                drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), trackName.c_str());

                // Delete button for track (positioned to the right of slider)
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding(); // Ensure proper vertical alignment

                std::string trackDeleteId = "track_delete_" + trackFile;
                if (RenderDeleteButton(trackDeleteId, trackFile.c_str())) {
                    // Track was deleted
                    m_controller->removeTrackFromCurrentSong(trackFile);
                }

                // Show effects when expanded (using the same shared state)
                if (actualTrackIndex >= 0 && isExpanded) {
                    ImGui::Indent();
                    drawFilterControls(actualTrackIndex);
                    ImGui::Unindent();
                }

                ImGui::PopID();
            }
        }

        void SongView::drawFilterControls(size_t trackIndex) {
            const auto& audioSystem = m_controller->getAudioSystem();

            // Get fresh filter state on every frame to ensure UI sync with audio system
            const auto& filters = audioSystem.getFilters(trackIndex);

            // Iterate through all filters in signal chain order (enabled first, then available)
            for (const auto& filterName : audioSystem.getFiltersInSignalChainOrder(trackIndex)) {
                // Check if filter is currently enabled (based on wet parameter > 0)
                bool effectivelyEnabled = audioSystem.isFilterEnabled(trackIndex, filterName);

                // Create unique ID for this filter's expanded state
                std::string expandedId = "expand_" + filterName + "_" + std::to_string(trackIndex);

                // Get or initialize expanded state using static map for persistence
                static std::map<std::string, bool> expandedStates;
                bool& isExpanded = expandedStates[expandedId];

                // Show dropdown arrow with effect name and status
                ImGui::PushID(filterName.c_str());

                // Color the arrow based on effect enabled state for visual feedback
                if (effectivelyEnabled) {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.4f, 0.8f, 0.4f, 1.0f)); // Green when enabled
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Gray when disabled
                }

                // Dropdown arrow (TreeNode style but manual control)
                if (ImGui::ArrowButton("##arrow", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                    isExpanded = !isExpanded; // Toggle expanded state
                }
                ImGui::PopStyleColor();

                // Effect name on same line with enable/disable status
                ImGui::SameLine();
                ImGui::Text("%s%s", filterName.c_str(), effectivelyEnabled ? " (ON)" : " (OFF)");

                // Show parameters when expanded
                if (isExpanded) {
                    ImGui::Indent();

                    // Get fresh filter state after any potential changes to ensure accuracy
                    const auto& currentFilters = audioSystem.getFilters(trackIndex);
                    auto it = currentFilters.find(filterName);
                    bool filterExists = (it != currentFilters.end());

                    if (filterExists) {
                        // Filter exists, show all parameters with type-specific controls
                        for (const auto& [paramId, param] : it->second.parameters) {
                            float value = param.value;
                            bool changed = false;

                            // Use appropriate control based on parameter type for better UX
                            switch (param.type) {
                                case Dynamix::ParameterType::BOOL: {
                                    // Boolean parameters use checkbox interface
                                    bool boolValue = value > 0.5f;
                                    if (ImGui::Checkbox(param.name.c_str(), &boolValue)) {
                                        value = boolValue ? 1.0f : 0.0f;
                                        changed = true;
                                    }
                                    break;
                                }
                                case Dynamix::ParameterType::INT: {
                                    // Integer parameters use slider with integer steps
                                    int intValue = static_cast<int>(value);
                                    // Special case for biquad filter type: show words instead of
                                    // numbers
                                    if (filterName == "biquad" && paramId == 1) {
                                        static const char* biquadTypeNames[] = {
                                            "Lowpass", "Highpass", "Bandpass"};
                                        // Clamp intValue to valid range
                                        intValue = std::max(0, std::min(2, intValue));

                                        // Use empty format string to hide the value
                                        if (ImGui::SliderInt("##biquad_type", &intValue, 0, 2,
                                                             "")) {
                                            value = static_cast<float>(intValue);
                                            changed = true;
                                        }

                                        // Draw filter type name as overlay in the middle of the
                                        // slider
                                        ImVec2 sliderMin = ImGui::GetItemRectMin();
                                        ImVec2 sliderMax = ImGui::GetItemRectMax();
                                        ImVec2 sliderCenter =
                                            ImVec2((sliderMin.x + sliderMax.x) * 0.5f,
                                                   (sliderMin.y + sliderMax.y) * 0.5f);

                                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                                        const char* typeName = biquadTypeNames[intValue];
                                        ImVec2 textSize = ImGui::CalcTextSize(typeName);
                                        ImVec2 textPos = ImVec2(sliderCenter.x - textSize.x * 0.5f,
                                                                sliderCenter.y - textSize.y * 0.5f);

                                        // Draw text with a subtle background for better readability
                                        drawList->AddRectFilled(
                                            ImVec2(textPos.x - 2, textPos.y - 1),
                                            ImVec2(textPos.x + textSize.x + 2,
                                                   textPos.y + textSize.y + 1),
                                            IM_COL32(0, 0, 0,
                                                     100)); // Semi-transparent black background
                                        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255),
                                                          typeName);

                                        // Draw parameter name above the slider
                                        ImGui::SameLine();
                                        ImGui::Text("%s", param.name.c_str());
                                    } else {
                                        if (ImGui::SliderInt(param.name.c_str(), &intValue,
                                                             static_cast<int>(param.min),
                                                             static_cast<int>(param.max))) {
                                            value = static_cast<float>(intValue);
                                            changed = true;
                                        }
                                    }
                                    break;
                                }
                                case Dynamix::ParameterType::FLOAT:
                                default: {
                                    // Float parameters use standard slider with special wet
                                    // parameter handling Highlight wet parameter for easy
                                    // identification (most effects use param ID 0)
                                    if (paramId == 0) {
                                        ImGui::PushStyleColor(
                                            ImGuiCol_FrameBg,
                                            value > 0.0f
                                                ? ImVec4(0.2f, 0.6f, 0.2f,
                                                         0.4f) // Green background when enabled
                                                : ImVec4(0.6f, 0.2f, 0.2f,
                                                         0.4f)); // Red background when disabled
                                    }

                                    if (ImGui::SliderFloat(param.name.c_str(), &value, param.min,
                                                           param.max)) {
                                        changed = true;
                                    }

                                    if (paramId == 0) {
                                        ImGui::PopStyleColor();
                                        // Add tooltip for wet parameter to explain its purpose
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip(
                                                "Wet parameter: Controls effect enable/disable.\n"
                                                "0.0 = effect disabled, >0.0 = effect enabled");
                                        }
                                    }
                                    break;
                                }
                            }

                            // Apply changes immediately to audio system
                            if (changed) {
                                m_controller->setTrackFilterParameter(trackIndex, filterName,
                                                                      paramId, value);
                            }
                        }
                    } else {
                        // Filter doesn't exist yet, show wet parameter to enable it
                        float wetValue = 0.0f;
                        ImGui::PushStyleColor(
                            ImGuiCol_FrameBg,
                            ImVec4(0.6f, 0.2f, 0.2f, 0.4f)); // Red for disabled state
                        if (ImGui::SliderFloat("wet", &wetValue, 0.0f, 1.0f)) {
                            m_controller->setTrackFilterParameter(trackIndex, filterName, 0,
                                                                  wetValue);
                        }
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Set wet > 0.0 to enable this effect");
                        }
                    }

                    ImGui::Unindent();
                }

                ImGui::PopID();
            }
        }

        bool SongView::RenderDeleteButton(const std::string& eventId, const char* eventName) {
            ImGui::PushID(eventId.c_str());
            bool clicked = ImGui::Button("X", ImVec2(20, 20));
            ImGui::PopID();

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Delete %s", eventName);
            }

            return clicked;
        }

        bool SongView::RenderSaveButton(const std::string& eventId, const char* eventName) {
            ImGui::PushID(eventId.c_str());
            bool clicked = ImGui::Button("S", ImVec2(20, 20));
            ImGui::PopID();

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Save %s", eventName);
            }

            return clicked;
        }

        void SongView::ShowOggFileDialog() {
            // Forward to MainView's FileBrowser via callback
            if (m_oggFileDialogCallback) {
                m_oggFileDialogCallback();
            }
        }

        void SongView::RenderNewSongDialog() {
            ImGui::OpenPopup("Create New Song");
            if (ImGui::BeginPopupModal("Create New Song", nullptr,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Enter song name:");
                ImGui::InputText("##song_name", m_newSongName, sizeof(m_newSongName));

                if (ImGui::Button("Create")) {
                    if (strlen(m_newSongName) > 0) {
                        // Use the custom name instead of auto-generated one
                        bool success = m_controller->createNewSongFolder(std::string(m_newSongName));
                        if (success) {
                            // Reload the current directory to refresh song list
                            m_controller->loadMusicFromDirectory();
                            
                            // Auto-switch to the newly created song
                            m_controller->setCurrentSong(std::string(m_newSongName));
                        }
                        m_showNewSongDialog = false;
                        std::fill(std::begin(m_newSongName), std::end(m_newSongName), 0);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    m_showNewSongDialog = false;
                    std::fill(std::begin(m_newSongName), std::end(m_newSongName), 0);
                }

                ImGui::EndPopup();
            }
        }


    } // namespace Views
} // namespace Dynamix