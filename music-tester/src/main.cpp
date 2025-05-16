#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Dear ImGui includes
#include "AudioSystem.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>

// Structure to hold a track and its properties
struct Track
{
    std::string name;
    float volume = 1.0f;
    bool active = false;
    std::vector<std::string> effects;
};

// Main application class
class MusicTester
{
  private:
    std::vector<Track> tracks;
    std::string musicDir;
    bool isRunning;
    AudioSystem audioSystem;
    bool isPlaying = false;
    float playbackPosition = 0.0f;

    // SDL variables
    SDL_Window* window;
    SDL_GLContext glContext;

  public:
    MusicTester() : isRunning(true), window(nullptr), glContext(nullptr)
    {
        // Constructor
    }

    ~MusicTester()
    {
        // ImGui cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        // SDL cleanup
        if (glContext) SDL_GL_DeleteContext(glContext);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool Initialize(const std::string& dir)
    {
        musicDir = dir;

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
        {
            std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        // Initialize audio system
        if (!audioSystem.initialize())
        {
            std::cerr << "Failed to initialize audio system" << std::endl;
            return false;
        }

        // For MacOS, use OpenGL 3.2 Core Profile
        const char* glsl_version = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_WindowFlags window_flags =
            (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow("Endless Sky - Music Tester", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        if (!window)
        {
            std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext)
        {
            std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_GL_MakeCurrent(window, glContext);
        SDL_GL_SetSwapInterval(1); // Enable vsync

        // Initialize Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext))
        {
            std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
            return false;
        }

        if (!ImGui_ImplOpenGL3_Init(glsl_version))
        {
            std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
            return false;
        }

        // Create sound_staging directory if it doesn't exist
        if (!std::filesystem::exists(musicDir))
        {
            try
            {
                std::filesystem::create_directories(musicDir);
                std::cout << "Created music directory: " << musicDir << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error creating music directory: " << e.what() << std::endl;
                // Non-fatal error, continue
            }
        }

        std::cout << "Initialization complete!" << std::endl;
        return true;
    }

    void LoadMusicFromDirectory()
    {
        if (!std::filesystem::exists(musicDir))
        {
            std::cerr << "Music directory doesn't exist: " << musicDir << std::endl;
            return;
        }

        std::cout << "Loading music from: " << musicDir << std::endl;

        // Load audio files using AudioSystem
        if (audioSystem.loadDirectory(musicDir))
        {
            // Update track list
            tracks.clear();
            for (const auto& entry : std::filesystem::directory_iterator(musicDir))
            {
                if (entry.is_regular_file())
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".ogg" || ext == ".wav" || ext == ".flac" || ext == ".mp3")
                    {
                        Track track;
                        track.name = entry.path().filename().string();
                        tracks.push_back(track);
                        std::cout << "Found track: " << track.name << std::endl;
                    }
                }
            }
        }
    }

    void Run()
    {
        LoadMusicFromDirectory();

        // Main loop
        while (isRunning)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT) isRunning = false;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(window))
                    isRunning = false;
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // Create main window
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::Begin("Music Tester");

            // Directory input
            static char dirInput[256] = "";
            ImGui::Text("Music Directory:");
            if (ImGui::InputText("##dir", dirInput, 256, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                musicDir = dirInput;
                LoadMusicFromDirectory();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                musicDir = dirInput;
                LoadMusicFromDirectory();
            }

            ImGui::Separator();

            // Global playback controls
            if (ImGui::Button(isPlaying ? "Pause All" : "Play All"))
            {
                if (isPlaying)
                {
                    audioSystem.pauseAll();
                }
                else
                {
                    audioSystem.playAll();
                }
                isPlaying = !isPlaying;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop All"))
            {
                audioSystem.stopAll();
                isPlaying = false;
            }

            // Playback position slider
            float currentPosition = audioSystem.getPlaybackPosition();
            if (ImGui::SliderFloat("Playback Position", &currentPosition, 0.0f, 1.0f))
            {
                audioSystem.setPlaybackPosition(currentPosition);
            }

            ImGui::Separator();

            // Track controls
            ImGui::Text("Tracks (%d):", static_cast<int>(tracks.size()));
            for (size_t i = 0; i < tracks.size(); i++)
            {
                Track& track = tracks[i];
                ImGui::PushID(static_cast<int>(i));

                // Track name and playback toggle
                ImGui::Checkbox("##active", &track.active);
                ImGui::SameLine();
                ImGui::Text("%s", track.name.c_str());

                // Volume slider
                float currentVolume = audioSystem.getTrackVolume(i);
                if (ImGui::SliderFloat("Volume", &currentVolume, 0.0f, 1.0f))
                {
                    audioSystem.setTrackVolume(i, currentVolume);
                }

                // Effects dropdown
                if (ImGui::BeginCombo("Effects", "Add Effect..."))
                {
                    static const char* effects[] = {"Reverb", "Delay", "Distortion", "EQ"};
                    for (int n = 0; n < IM_ARRAYSIZE(effects); n++)
                    {
                        bool is_selected = false;
                        if (ImGui::Selectable(effects[n], is_selected))
                        {
                            track.effects.push_back(effects[n]);
                        }
                        if (is_selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                // List current effects
                for (size_t j = 0; j < track.effects.size(); j++)
                {
                    ImGui::Text("- %s", track.effects[j].c_str());
                    ImGui::SameLine();

                    // Create a unique button ID using string concatenation
                    std::string buttonId = "X##" + std::to_string(j);
                    if (ImGui::SmallButton(buttonId.c_str()))
                    {
                        track.effects.erase(track.effects.begin() + j);
                    }
                }

                ImGui::PopID();
                ImGui::Separator();
            }

            ImGui::End();

            // Rendering
            ImGui::Render();
            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }
    }
};

int main(int argc, char* argv[])
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // For MacOS, use OpenGL 3.2 Core Profile
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Endless Sky - Music Tester", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (!window)
    {
        std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Create and initialize the view
    MusicTester view;
    if (!view.Initialize(std::filesystem::current_path().string()))
    {
        std::cerr << "Failed to initialize view" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Get the executable's directory and set up music directory
    std::filesystem::path exePath = std::filesystem::current_path();
    std::string musicDir = (exePath / "sound_staging").string();
    view.SetMusicDirectory(musicDir);
    view.LoadMusicFromDirectory();

    // Main loop
    while (view.IsRunning())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            view.ProcessEvents(event);
        }

        view.Render();
    }

    // Cleanup
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}