#include "BusView.h"
#include "AudioController.h"
#include "AudioState.h"
#include "AudioSystem.h"
#include "Logger.h"
#include <algorithm>
#include <imgui.h>

namespace Dynamix {
    namespace Views {

        BusView::BusView(Dynamix::AudioController* controller) : m_controller(controller) {}

        void BusView::Render() {
            // Guard against null controller
            if (!m_controller) {
                ImGui::Text("No controller available");
                return;
            }

            RenderBusControls();
        }

        void BusView::RenderBusControls() {
            const auto& state = m_controller->getState();
            const auto& audioSystem = m_controller->getAudioSystem();

            ImGui::Begin("Bus Controls");

            // Bus volume
            float busVolume = state.busVolume;
            if (ImGui::SliderFloat("Bus Volume", &busVolume, 0.0f, 1.0f)) {
                m_controller->setBusVolume(busVolume);
            }

            ImGui::Separator();
            ImGui::Text("Bus FX");
            const auto& busFilters = audioSystem.getBusFilters();

            // Iterate through all bus filters in signal chain order (enabled first, then available)
            for (const auto& filterName : audioSystem.getBusFiltersInSignalChainOrder()) {
                // Check if filter is currently enabled (based on wet parameter > 0)
                bool effectivelyEnabled = audioSystem.isBusFilterEnabled(filterName);

                // Create unique ID for this filter's expanded state
                std::string expandedId = "expand_bus_" + filterName;

                // Get or initialize expanded state
                static std::map<std::string, bool> expandedStates;
                bool& isExpanded = expandedStates[expandedId];

                // Show dropdown arrow with effect name and status
                ImGui::PushID(filterName.c_str());

                // Color the arrow based on effect enabled state
                if (effectivelyEnabled) {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.4f, 0.8f, 0.4f, 1.0f)); // Green when enabled
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Gray when disabled
                }

                // Dropdown arrow (TreeNode style but manual)
                if (ImGui::ArrowButton("##arrow", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                    isExpanded = !isExpanded;
                }
                ImGui::PopStyleColor();

                // Effect name on same line
                ImGui::SameLine();
                ImGui::Text("%s%s", filterName.c_str(), effectivelyEnabled ? " (ON)" : " (OFF)");

                // Show parameters when expanded
                if (isExpanded) {
                    ImGui::Indent();

                    // Get fresh filter state after any potential changes
                    const auto& currentBusFilters = audioSystem.getBusFilters();
                    auto it = currentBusFilters.find(filterName);
                    bool filterExists = (it != currentBusFilters.end());

                    if (filterExists) {
                        // Filter exists, show all parameters
                        for (const auto& [paramId, param] : it->second.parameters) {
                            float value = param.value;
                            bool changed = false;

                            // Use appropriate control based on parameter type
                            switch (param.type) {
                                case Dynamix::ParameterType::BOOL: {
                                    bool boolValue = value > 0.5f;
                                    if (ImGui::Checkbox(param.name.c_str(), &boolValue)) {
                                        value = boolValue ? 1.0f : 0.0f;
                                        changed = true;
                                    }
                                    break;
                                }
                                case Dynamix::ParameterType::INT: {
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

                            if (changed) {
                                m_controller->setBusFilterParameter(filterName, paramId, value);
                            }
                        }
                    } else {
                        // Filter doesn't exist yet, show wet parameter to enable it
                        float wetValue = 0.0f;
                        ImGui::PushStyleColor(
                            ImGuiCol_FrameBg,
                            ImVec4(0.6f, 0.2f, 0.2f, 0.4f)); // Red for disabled state
                        if (ImGui::SliderFloat("wet", &wetValue, 0.0f, 1.0f)) {
                            m_controller->setBusFilterParameter(filterName, 0, wetValue);
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

            ImGui::End();
        }

    } // namespace Views
} // namespace Dynamix