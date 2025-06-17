#include "TesterView.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL2/SDL_opengl.h>
#include <filesystem>
#include <iostream>
#include <unordered_map>

TesterView::TesterView() {
    // Preload the sound_staging folder as the default music directory
    m_musicDir = "sound_staging";
    strncpy(m_dirInput, m_musicDir.c_str(), sizeof(m_dirInput));
    m_dirInput[sizeof(m_dirInput) - 1] = '\0';
    SetMusicDirectory(m_musicDir);
}

TesterView::~TesterView() { cleanup(); }

bool TesterView::Initialize(SDL_Window* window, SDL_GLContext glContext) {
    m_window = window;
    m_glContext = glContext;

    // Initialize audio system
    if (!m_audioSystem.initialize()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return false;
    }

    // After audio system is initialized, load music
    LoadMusicFromDirectory();

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
        std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
        return false;
    }

    const char* glsl_version = "#version 150";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    return true;
}

void TesterView::ProcessEvents(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
        m_isRunning = false;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(m_window))
        m_isRunning = false;
}

void TesterView::SetMusicDirectory(const std::string& dir) {
    m_musicDir = dir;
    LoadMusicFromDirectory();
}

void TesterView::LoadMusicFromDirectory() {
    if (m_musicDir.empty())
        return;

    m_tracks.clear();
    for (const auto& entry : std::filesystem::directory_iterator(m_musicDir)) {
        if (entry.is_regular_file()) {
            const auto& path = entry.path();
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (AudioTester::AudioSystem::isSupportedFileExtension(ext)) {
                Track track;
                track.name = path.filename().string();
                track.active = true; // Set all tracks to enabled by default
                m_tracks.push_back(track);
                m_audioSystem.loadAudioFile(path.string());
                // Set looping to true for all tracks
                m_audioSystem.setTrackLooping(m_tracks.size() - 1, true);
            }
        }
    }
}

void TesterView::Render() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    RenderMainWindow();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);
}

void TesterView::RenderMainWindow() {
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
    ImGui::Begin("Music Tester");

    RenderDirectoryInput();
    ImGui::Separator();
    RenderGlobalControls();
    ImGui::Separator();
    RenderTrackControls();
    ImGui::Separator();
    RenderBusControls();

    ImGui::End();
}

void TesterView::RenderDirectoryInput() {
    ImGui::Text("Music Directory:");
    if (ImGui::InputText("##dir", m_dirInput, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        SetMusicDirectory(m_dirInput);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        SetMusicDirectory(m_dirInput);
    }
}

void TesterView::RenderGlobalControls() {
    if (ImGui::Button(m_isPlaying ? "Stop" : "Play")) {
        m_isPlaying = !m_isPlaying;
        if (m_isPlaying) {
            // Play all tracks regardless of enabled state
            for (size_t i = 0; i < m_tracks.size(); ++i) {
                m_audioSystem.playTrack(i);
            }
        } else {
            for (size_t i = 0; i < m_tracks.size(); ++i) {
                m_audioSystem.stopTrack(i);
            }
        }
    }
}

void TesterView::RenderTrackControls() {
    ImGui::Text("Tracks (%d):", static_cast<int>(m_tracks.size()));
    for (size_t i = 0; i < m_tracks.size(); i++) {
        auto& track = m_tracks[i];
        ImGui::PushID(static_cast<int>(i));

        bool wasActive = track.active;
        ImGui::Checkbox("##active", &track.active);
        if (wasActive != track.active) {
            // Update volume immediately when enabled state changes
            m_audioSystem.setTrackVolume(i, track.active ? track.volume : 0.0f);
        }
        ImGui::SameLine();
        ImGui::Text("%s", track.name.c_str());

        // Loop toggle
        if (ImGui::Checkbox("Loop", &track.looping)) {
            m_audioSystem.setTrackLooping(i, track.looping);
        }

        // Volume slider
        if (ImGui::SliderFloat("Volume", &track.volume, 0.0f, 1.0f)) {
            // Update volume immediately when slider changes
            m_audioSystem.setTrackVolume(i, track.active ? track.volume : 0.0f);
        }

        // Call drawFilterControls() for this track
        drawFilterControls(i);

        ImGui::PopID();
        ImGui::Separator();
    }
}

void TesterView::drawFilterControls(size_t trackIndex) {
    for (const auto& filterName : AudioTester::AudioSystem::AVAILABLE_FILTERS) {
        bool enabled = m_audioSystem.isFilterEnabled(trackIndex, filterName);
        if (ImGui::Checkbox(filterName.c_str(), &enabled)) {
            m_audioSystem.setFilterEnabled(trackIndex, filterName, enabled);
        }
        if (enabled) {
            // Get track filters directly as vector
            const auto& filters = m_audioSystem.getTrackFilters(trackIndex);
            for (const auto& instance : filters) {
                if (instance.filterName == filterName && instance.enabled) {
                    for (const auto& [paramId, param] : instance.parameters) {
                        float value =
                            m_audioSystem.getFilterParameter(trackIndex, filterName, paramId);
                        if (ImGui::SliderFloat(param.name.c_str(), &value, param.min, param.max)) {
                            m_audioSystem.setFilterParameter(trackIndex, filterName, paramId,
                                                             value);
                        }
                    }
                    break;
                }
            }
        }
    }
}

void TesterView::RenderEffectsControls(Track& track, size_t trackIndex) {
    // This method is no longer used - replaced by drawFilterControls
}

void TesterView::RenderBusControls() {
    ImGui::Begin("Bus Controls");
    float busVolume = m_audioSystem.getBusVolume();
    if (ImGui::SliderFloat("Bus Volume", &busVolume, 0.0f, 1.0f)) {
        m_audioSystem.setBusVolume(busVolume);
    }
    ImGui::Separator();
    ImGui::Text("Bus FX");
    for (const auto& filterName : AudioTester::AudioSystem::AVAILABLE_FILTERS) {
        bool enabled = m_audioSystem.isBusFilterEnabled(filterName);
        if (ImGui::Checkbox(filterName.c_str(), &enabled)) {
            m_audioSystem.setBusFilterEnabled(filterName, enabled);
        }
        if (enabled) {
            const auto& filters = m_audioSystem.getBusFilters();
            auto it = filters.find(filterName);
            if (it != filters.end()) {
                for (const auto& [paramId, param] : it->second.parameters) {
                    float value = m_audioSystem.getBusFilterParameter(filterName, paramId);
                    if (ImGui::SliderFloat(param.name.c_str(), &value, param.min, param.max)) {
                        m_audioSystem.setBusFilterParameter(filterName, paramId, value);
                    }
                }
            }
        }
    }
    ImGui::End();
}

void TesterView::cleanup() {
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}