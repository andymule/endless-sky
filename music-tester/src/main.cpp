#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

// OAML includes
#include "oaml.h"

// Dear ImGui includes
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>

// Structure to hold a track and its properties
struct Track {
    std::string name;
    float volume = 1.0f;
    bool active = false;
    std::vector<std::string> effects;
};

// Main application class
class MusicTester {
private:
    oamlApi* oaml;
    std::vector<Track> tracks;
    std::string musicDir;
    bool isRunning;
    
    // SDL variables
    SDL_Window* window;
    SDL_GLContext glContext;
    
public:
    MusicTester() : oaml(nullptr), isRunning(true), window(nullptr), glContext(nullptr) {
        // Initialize OAML
        oaml = new oamlApi();
    }
    
    ~MusicTester() {
        // Cleanup
        if (oaml) {
            oaml->Shutdown();
            delete oaml;
        }
        
        // ImGui cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        
        // SDL cleanup
        if (glContext) SDL_GL_DeleteContext(glContext);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
    
    bool Initialize(const std::string& dir) {
        musicDir = dir;
        
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            std::cerr << "Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        
        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow("Endless Sky - Music Tester", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        glContext = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, glContext);
        SDL_GL_SetSwapInterval(1); // Enable vsync
        
        // Initialize Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        
        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window, glContext);
        ImGui_ImplOpenGL3_Init(glsl_version);
        
        // Initialize OAML
        if (!oaml->Init("music-tester.defs")) {
            // No definition file found, initialize with defaults
            std::cout << "No music definition file found, starting with empty configuration" << std::endl;
            oaml->Init(nullptr);
        }
        
        return true;
    }
    
    void LoadMusicFromDirectory() {
        if (!std::filesystem::exists(musicDir)) {
            std::cerr << "Music directory doesn't exist: " << musicDir << std::endl;
            return;
        }
        
        // Load audio files from directory
        try {
            for (const auto& entry : std::filesystem::directory_iterator(musicDir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".ogg" || ext == ".wav" || ext == ".aif") {
                        Track track;
                        track.name = entry.path().filename().string();
                        tracks.push_back(track);
                        
                        // Add the track to OAML
                        std::cout << "Found track: " << track.name << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading music: " << e.what() << std::endl;
        }
    }
    
    void Run() {
        LoadMusicFromDirectory();
        
        // Main loop
        while (isRunning) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    isRunning = false;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
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
            if (ImGui::InputText("##dir", dirInput, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
                musicDir = dirInput;
                tracks.clear();
                LoadMusicFromDirectory();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                musicDir = dirInput;
                tracks.clear();
                LoadMusicFromDirectory();
            }
            
            ImGui::Separator();
            
            // Global playback controls
            if (ImGui::Button("Play All")) {
                // TODO: Play all tracks
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop All")) {
                // TODO: Stop all tracks
                oaml->StopPlaying();
            }
            
            ImGui::Separator();
            
            // Track controls
            ImGui::Text("Tracks:");
            for (size_t i = 0; i < tracks.size(); i++) {
                Track& track = tracks[i];
                ImGui::PushID(static_cast<int>(i));
                
                // Track name and playback toggle
                ImGui::Checkbox("##active", &track.active);
                ImGui::SameLine();
                ImGui::Text("%s", track.name.c_str());
                
                // Volume slider
                ImGui::SliderFloat("Volume", &track.volume, 0.0f, 1.0f);
                
                // Effects dropdown
                if (ImGui::BeginCombo("Effects", "Add Effect...")) {
                    static const char* effects[] = { "Reverb", "Delay", "Distortion", "EQ" };
                    for (int n = 0; n < IM_ARRAYSIZE(effects); n++) {
                        bool is_selected = false;
                        if (ImGui::Selectable(effects[n], is_selected)) {
                            track.effects.push_back(effects[n]);
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                
                // List current effects
                for (size_t j = 0; j < track.effects.size(); j++) {
                    ImGui::Text("- %s", track.effects[j].c_str());
                    ImGui::SameLine();
                    
                    // Create a unique button ID using string concatenation
                    std::string buttonId = "X##" + std::to_string(j);
                    if (ImGui::SmallButton(buttonId.c_str())) {
                        track.effects.erase(track.effects.begin() + j);
                        j--;
                    }
                }
                
                ImGui::Separator();
                ImGui::PopID();
            }
            
            ImGui::End();
            
            // Rendering
            ImGui::Render();
            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
            
            // Call OAML update function
            oaml->Update();
        }
    }
};

int main(int argc, char* argv[]) {
    std::string musicDir = "sound_staging";
    
    // Allow user to specify a different music directory
    if (argc > 1) {
        musicDir = argv[1];
    }
    
    MusicTester app;
    
    if (!app.Initialize(musicDir)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    app.Run();
    
    return 0;
} 