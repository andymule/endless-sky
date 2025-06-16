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
    m_musicDir = "sound_staging";
    strncpy(m_dirInput, m_musicDir.c_str(), sizeof(m_dirInput));
    m_dirInput[sizeof(m_dirInput) - 1] = '\0';
    setMusicDirectory(m_musicDir);
}

TesterView::~TesterView()
{
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool TesterView::initialize(SDL_Window* window, SDL_GLContext glContext)
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
    loadMusicFromDirectory();

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

void TesterView::processEvents(const SDL_Event& event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) m_isRunning = false;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(m_window))
        m_isRunning = false;
}

void TesterView::setMusicDirectory(const std::string& dir)
{
    m_musicDir = dir;
    if (!std::filesystem::exists(m_musicDir))
    {
        std::cout << "Directory does not exist: " << m_musicDir << std::endl;
    }
}

void TesterView::loadMusicFromDirectory()
{
    if (!std::filesystem::exists(m_musicDir))
    {
        std::cout << "Directory does not exist: " << m_musicDir << std::endl;
        return;
    }

    std::cout << "Loading music from: " << m_musicDir << std::endl;

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
                    track.volume = 1.0f;
                    m_tracks.push_back(track);
                    std::cout << "Found track: " << track.name << std::endl;
                }
            }
        }
    }
}

void TesterView::render()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderMainWindow();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);
}

void TesterView::renderMainWindow()
{
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
    ImGui::Begin("Music Tester");

    renderDirectoryInput();
    ImGui::Separator();
    renderMasterControls();
    ImGui::Separator();
    renderTrackControls();
    ImGui::Separator();
    renderFilterControls();

    ImGui::End();
}

void TesterView::renderDirectoryInput()
{
    ImGui::Text("Music Directory:");
    if (ImGui::InputText("##dir", m_dirInput, 256, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        setMusicDirectory(m_dirInput);
        loadMusicFromDirectory();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        setMusicDirectory(m_dirInput);
        loadMusicFromDirectory();
    }
}

void TesterView::renderMasterControls()
{
    if (ImGui::CollapsingHeader("Master Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Master toggle
        if (ImGui::Checkbox("Enable Playback", &m_masterEnabled))
        {
            m_audioSystem.setMasterEnabled(m_masterEnabled);
        }

        // Master volume
        if (ImGui::SliderFloat("Master Volume", &m_masterVolume, 0.0f, 1.0f))
        {
            m_audioSystem.setBusVolume(m_masterVolume);
        }

        ImGui::Separator();
    }
}

void TesterView::renderTrackControls()
{
    if (ImGui::CollapsingHeader("Track Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t i = 0; i < m_tracks.size(); ++i)
        {
            auto& track = m_tracks[i];
            ImGui::PushID(static_cast<int>(i));

            // Track name and volume
            ImGui::Text("%s", track.name.c_str());
            if (ImGui::SliderFloat("Volume", &track.volume, 0.0f, 1.0f))
            {
                m_audioSystem.setTrackVolume(i, track.volume);
            }

            ImGui::PopID();
            ImGui::Separator();
        }
    }
}

void TesterView::renderFilterControls()
{
    if (ImGui::CollapsingHeader("Filter Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Bus filters
        if (ImGui::TreeNode("Bus Filters"))
        {
            for (const auto& filterName : AudioSystem::AVAILABLE_FILTERS)
            {
                bool enabled = m_audioSystem.isBusFilterEnabled(filterName);
                if (ImGui::Checkbox(filterName.c_str(), &enabled))
                {
                    m_audioSystem.setBusFilterEnabled(filterName, enabled);
                }

                if (enabled)
                {
                    const auto& params = m_audioSystem.getBusFilters();
                    auto it = params.find(filterName);
                    if (it != params.end())
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
            ImGui::TreePop();
        }

        // Track filters
        for (size_t i = 0; i < m_tracks.size(); ++i)
        {
            if (ImGui::TreeNode(("Track " + std::to_string(i + 1) + " Filters").c_str()))
            {
                for (const auto& filterName : AudioSystem::AVAILABLE_FILTERS)
                {
                    bool enabled = m_audioSystem.isFilterEnabled(i, filterName);
                    if (ImGui::Checkbox(filterName.c_str(), &enabled))
                    {
                        m_audioSystem.setFilterEnabled(i, filterName, enabled);
                    }

                    if (enabled)
                    {
                        const auto& params = m_audioSystem.getFilters(i);
                        auto it = params.find(filterName);
                        if (it != params.end())
                        {
                            for (const auto& [paramId, param] : it->second.parameters)
                            {
                                float value = m_audioSystem.getFilterParameter(i, filterName, paramId);
                                if (ImGui::SliderFloat(param.name.c_str(), &value, param.min, param.max))
                                {
                                    m_audioSystem.setFilterParameter(i, filterName, paramId, value);
                                }
                            }
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
    }
}