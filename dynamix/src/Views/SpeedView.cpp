#include "SpeedView.h"
#include "AudioController.h"
#include "AudioState.h"
#include "AudioSystem.h"
#include "Logger.h"
#include <imgui.h>

namespace Dynamix {
    namespace Views {

        SpeedView::SpeedView(Dynamix::AudioController* controller) : m_controller(controller) {
            // Initialize tempo UI state
            m_masterTempoUI = 1.0f;
        }

        void SpeedView::Render() {
            // Guard against null controller
            if (!m_controller) {
                ImGui::Text("No controller available");
                return;
            }

            RenderSpeedControls();
        }

        void SpeedView::RenderSpeedControls() {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            ImGui::Begin("Speed Controls", nullptr, ImGuiWindowFlags_None);

            // Tempo Control Section
            ImGui::Text("Tempo Control");
            ImGui::Separator();

            if (m_controller) {
                // Sync UI state with controller state only when not actively editing
                bool uiIsDragging = ImGui::IsAnyItemActive();
                if (!uiIsDragging) {
                    m_masterTempoUI = m_controller->getMasterTempo();
                }

                // Playback Speed slider (tape-style, affects pitch)
                if (ImGui::SliderFloat("Tape Speed", &m_masterTempoUI, 0.1f, 4.0f, "%.2fx")) {
                    m_controller->setMasterTempo(m_masterTempoUI);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Tape-style speed control (affects pitch)");
                }

                // Granular Tempo slider (pitch-preserving)
                float granularTempo = m_controller->getGranularTempo();
                if (ImGui::SliderFloat("Granular Tempo", &granularTempo, 0.5f, 2.0f, "%.2fx")) {
                    m_controller->setGranularTempo(granularTempo);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    float latency = m_controller->getGranularLatencyMs();
                    ImGui::SetTooltip("Pitch-preserving tempo stretching\nLatency: %.1f ms",
                                      latency);
                }
            }

            ImGui::End();
        }

    } // namespace Views
} // namespace Dynamix