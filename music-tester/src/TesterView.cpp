#include "TesterView.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL_opengl.h>
#include <filesystem>
#include <iostream>
#include <unordered_map>

TesterView::TesterView()
{
    // Preload the sound_staging folder as the default music directory
    m_musicDir = "/Users/arckex/source/endless-sky/music-tester/sound_staging";
    strncpy(m_dirInput, m_musicDir.c_str(), sizeof(m_dirInput));
    m_dirInput[sizeof(m_dirInput) - 1] = '\0';
    SetMusicDirectory(m_musicDir);
}

TesterView::~TesterView()
{
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool TesterView::Initialize(SDL_Window* window, SDL_GLContext glContext)
{
    m_window = window;
    m_glContext = glContext;

    // Initialize audio system
    if (!m_audioSystem.initialize())
    {
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
    if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext))
    {
        std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
        return false;
    }

    const char* glsl_version = "#version 150";
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    return true;
}

void TesterView::ProcessEvents(const SDL_Event& event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) m_isRunning = false;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(m_window))
        m_isRunning = false;
}

void TesterView::SetMusicDirectory(const std::string& dir)
{
    m_musicDir = dir;
    if (!std::filesystem::exists(m_musicDir))
    {
        try
        {
            std::filesystem::create_directories(m_musicDir);
            std::cout << "Created music directory: " << m_musicDir << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error creating music directory: " << e.what() << std::endl;
        }
    }
}

void TesterView::LoadMusicFromDirectory()
{
    if (!std::filesystem::exists(m_musicDir))
    {
        std::cerr << "Music directory doesn't exist: " << m_musicDir << std::endl;
        return;
    }

    std::cout << "Loading music from: " << m_musicDir << std::endl;

    // Store current track states
    std::unordered_map<std::string, bool> trackLoopingStates;
    for (const auto& track : m_tracks)
    {
        trackLoopingStates[track.name] = track.looping;
    }

    // Load audio files using AudioSystem
    if (m_audioSystem.loadDirectory(m_musicDir))
    {
        // Update track list
        m_tracks.clear();
        for (const auto& entry : std::filesystem::directory_iterator(m_musicDir))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".ogg" || ext == ".wav" || ext == ".flac" || ext == ".mp3")
                {
                    Track track;
                    track.name = entry.path().filename().string();
                    // Set looping to true by default for new tracks
                    track.looping = true;
                    // Restore looping state if it existed before
                    auto it = trackLoopingStates.find(track.name);
                    if (it != trackLoopingStates.end())
                    {
                        track.looping = it->second;
                    }
                    m_tracks.push_back(track);
                    m_audioSystem.setTrackLooping(m_tracks.size() - 1, track.looping);
                    std::cout << "Found track: " << track.name << std::endl;
                }
            }
        }
    }
}

void TesterView::Render()
{
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

void TesterView::RenderMainWindow()
{
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

void TesterView::RenderDirectoryInput()
{
    ImGui::Text("Music Directory:");
    if (ImGui::InputText("##dir", m_dirInput, 256, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        SetMusicDirectory(m_dirInput);
        LoadMusicFromDirectory();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        SetMusicDirectory(m_dirInput);
        LoadMusicFromDirectory();
    }
}

void TesterView::RenderGlobalControls()
{
    if (ImGui::Button(m_isPlaying ? "Pause All" : "Play All"))
    {
        if (m_isPlaying)
        {
            m_audioSystem.pauseAll();
        }
        else
        {
            m_audioSystem.playAll();
        }
        m_isPlaying = !m_isPlaying;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop All"))
    {
        m_audioSystem.stopAll();
        m_isPlaying = false;
    }

    // Playback position slider
    float currentPosition = m_audioSystem.getPlaybackPosition();
    float maxLength = m_audioSystem.getLongestTrackLength();
    if (ImGui::SliderFloat("Playback Position", &currentPosition, 0.0f, maxLength, "%.2f s"))
    {
        m_audioSystem.setPlaybackPosition(currentPosition);
    }
}

void TesterView::RenderTrackControls()
{
    ImGui::Text("Tracks (%d):", static_cast<int>(m_tracks.size()));
    for (size_t i = 0; i < m_tracks.size(); i++)
    {
        Track& track = m_tracks[i];
        ImGui::PushID(static_cast<int>(i));

        // Track name and playback toggle
        ImGui::Checkbox("##active", &track.active);
        ImGui::SameLine();
        ImGui::Text("%s", track.name.c_str());

        // Loop toggle
        if (ImGui::Checkbox("Loop", &track.looping))
        {
            m_audioSystem.setTrackLooping(i, track.looping);
        }

        // Volume slider
        float currentVolume = m_audioSystem.getTrackVolume(i);
        if (ImGui::SliderFloat("Volume", &currentVolume, 0.0f, 1.0f))
        {
            m_audioSystem.setTrackVolume(i, currentVolume);
        }

        // Call drawFilterControls() for this track
        drawFilterControls(i);

        ImGui::PopID();
        ImGui::Separator();
    }
}

void TesterView::drawFilterControls(size_t trackIndex)
{
    for (const auto& filterName : AudioSystem::AVAILABLE_FILTERS)
    {
        bool enabled = m_audioSystem.isFilterEnabled(trackIndex, filterName);
        if (ImGui::Checkbox(filterName.c_str(), &enabled))
        {
            m_audioSystem.setFilterEnabled(trackIndex, filterName, enabled);
        }
        if (enabled)
        {
            const auto& filters = m_audioSystem.getFilters(trackIndex);
            auto it = filters.find(filterName);
            if (it != filters.end())
            {
                for (const auto& [paramId, param] : it->second.parameters)
                {
                    float value = m_audioSystem.getFilterParameter(trackIndex, filterName, paramId);
                    if (ImGui::SliderFloat(param.name.c_str(), &value, param.min, param.max))
                    {
                        m_audioSystem.setFilterParameter(trackIndex, filterName, paramId, value);
                    }
                }
            }
        }
    }
}

void TesterView::RenderBusControls()
{
    ImGui::Begin("Bus Controls");
    float busVolume = m_audioSystem.getBusVolume();
    if (ImGui::SliderFloat("Bus Volume", &busVolume, 0.0f, 1.0f))
    {
        m_audioSystem.setBusVolume(busVolume);
    }
    ImGui::Separator();
    ImGui::Text("Bus FX");
    for (const auto& filterName : AudioSystem::AVAILABLE_FILTERS)
    {
        bool enabled = m_audioSystem.isBusFilterEnabled(filterName);
        if (ImGui::Checkbox(filterName.c_str(), &enabled))
        {
            m_audioSystem.setBusFilterEnabled(filterName, enabled);
        }
        if (enabled)
        {
            const auto& filters = m_audioSystem.getBusFilters();
            auto it = filters.find(filterName);
            if (it != filters.end())
            {
                for (const auto& [paramId, param] : it->second.parameters)
                {
                    float value = m_audioSystem.getBusFilterParameter(filterName, paramId);
                    if (ImGui::SliderFloat(param.name.c_str(), &value, param.min, param.max))
                    {
                        m_audioSystem.setBusFilterParameter(filterName, paramId, value);
                    }
                }
            }
        }
    }
    ImGui::End();
}