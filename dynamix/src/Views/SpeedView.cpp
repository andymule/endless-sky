#include "SpeedView.h"
#include "AudioController.h"
#include "AudioState.h"
#include "AudioSystem.h"
#include "Logger.h"
#include <imgui.h>

namespace Dynamix {
    namespace Views {

        SpeedView::SpeedView(Dynamix::AudioController* controller) : m_controller(controller) {}

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

            ImGui::Text("Tempo Control");
            ImGui::Separator();

            if (m_controller) {
                const bool uiIsDragging = ImGui::IsAnyItemActive();
                if (!uiIsDragging) {
                    m_masterTempoUI = m_controller->getMasterTempo();
                    m_granularTempoUI = m_controller->getGranularTempo();
                }

                if (ImGui::SliderFloat("Tape Speed", &m_masterTempoUI, 0.1f, 4.0f, "%.2fx")) {
                    m_controller->setMasterTempo(m_masterTempoUI);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "Repitch: speed and pitch move together.\nStacks with Grain Tempo.");
                }

                if (ImGui::SliderFloat("Grain Tempo", &m_granularTempoUI, 0.5f, 2.0f, "%.2fx")) {
                    m_controller->setGranularTempo(m_granularTempoUI);
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    const float latency = m_controller->getGranularLatencyMs();
                    ImGui::SetTooltip(
                        "Warp: speed changes, pitch stays put.\nStacks with Tape Speed.\nLatency: %.0f ms",
                        latency);
                }

                const float combined = m_masterTempoUI * m_granularTempoUI;
                ImGui::TextDisabled("Combined speed %.2fx  |  pitch %.2fx", combined,
                                    m_masterTempoUI);
            }

            ImGui::End();
        }

    } // namespace Views
} // namespace Dynamix