#include "AdaptiveMusic.h"
#include "config.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>

#include <stdio.h>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
AdaptiveMusic musicSystem;
std::string resourcePath = "resources/";
bool showDemoWindow = false;

// Function to browse and select music files
std::string BrowseForFile() {
    // For now, return a simple dialog result
    // In a real implementation, you could use ImGui file dialogs or platform-specific dialogs
    static char filePath[256] = "";
    
    // This is a placeholder - in practice you'd want to use a proper file dialog
    // For now, let's just prompt the user to enter a file path
    return "resources/music.mp3"; // Placeholder
}

// Initialize SDL and ImGui
bool Initialize() {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

    // Create window
    window = SDL_CreateWindow(
        "Adaptive Music Tester", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (window == nullptr) {
        printf("Error creating window: %s\n", SDL_GetError());
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(
        window, -1, 
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );
    if (renderer == nullptr) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    // Note: Docking is not available in this ImGui build

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Initialize the music system
    if (!musicSystem.Initialize()) {
        printf("Failed to initialize the music system\n");
        return false;
    }

    // Create default music layers
    musicSystem.AddLayer("Ambient", IntensityLevel::AMBIENT);
    musicSystem.AddLayer("Action Low", IntensityLevel::LOW);
    musicSystem.AddLayer("Action Medium", IntensityLevel::MEDIUM);
    musicSystem.AddLayer("Action High", IntensityLevel::HIGH);
    musicSystem.AddLayer("Boss", IntensityLevel::BOSS);

    // Create resources directory if it doesn't exist
    std::filesystem::create_directories("resources");

    return true;
}

// Clean up resources
void Cleanup() {
    // Shutdown music system
    musicSystem.Shutdown();

    // Cleanup ImGui
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Render main menu bar
void RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                SDL_Event event;
                event.type = SDL_QUIT;
                SDL_PushEvent(&event);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

// Render the layers panel
void RenderLayersPanel() {
    ImGui::Begin("Layers");
    
    // Add new layer button
    if (ImGui::Button("Add Layer")) {
        // In a real app, you'd show a dialog to enter layer name and select intensity
        std::string newLayerName = "New Layer " + std::to_string(musicSystem.GetLayers().size() + 1);
        musicSystem.AddLayer(newLayerName, IntensityLevel::AMBIENT);
    }
    
    ImGui::Separator();
    
    // List of layers
    for (const auto& layer : musicSystem.GetLayers()) {
        if (ImGui::TreeNode(layer->GetName().c_str())) {
            // Layer properties
            std::string levelName = musicSystem.GetIntensityNames().at(layer->GetLevel());
            ImGui::Text("Intensity: %s", levelName.c_str());
            
            // Tracks in this layer
            ImGui::Text("Tracks:");
            for (const auto& track : layer->GetTracks()) {
                ImGui::BulletText("%s", track->GetFilePath().c_str());
            }
            
            // Add track to this layer with file path input
            static char filePath[512] = "";
            ImGui::Text("Add Track:");
            ImGui::InputText("##filepath", filePath, sizeof(filePath));
            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                // For now, suggest some common paths
                strcpy(filePath, "resources/");
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Track")) {
                if (strlen(filePath) > 0) {
                    musicSystem.AddTrackToLayer(layer->GetName(), std::string(filePath));
                    // Clear the input after adding
                    filePath[0] = '\0';
                }
            }
            
            ImGui::TreePop();
        }
    }
    
    ImGui::End();
}

// Render the controls panel
void RenderControlsPanel() {
    ImGui::Begin("Controls");
    
    // Play/Stop buttons
    if (ImGui::Button(musicSystem.IsPlaying() ? "Stop" : "Play", ImVec2(120, 30))) {
        if (musicSystem.IsPlaying()) {
            musicSystem.Stop();
        } else {
            musicSystem.Play();
        }
    }
    
    ImGui::SameLine();
    
    // Pause/Resume buttons
    bool isPaused = false; // In a real implementation, this would come from the engine
    if (ImGui::Button(isPaused ? "Resume" : "Pause", ImVec2(120, 30))) {
        if (isPaused) {
            musicSystem.Resume();
        } else {
            musicSystem.Pause();
        }
    }
    
    ImGui::Separator();
    
    // Intensity level selection
    ImGui::Text("Intensity Level:");
    
    static const char* intensityLabels[] = { "Ambient", "Low", "Medium", "High", "Boss" };
    static int currentIntensity = static_cast<int>(musicSystem.GetCurrentIntensity());
    
    if (ImGui::Combo("##intensity", &currentIntensity, intensityLabels, IM_ARRAYSIZE(intensityLabels))) {
        musicSystem.SetIntensityLevel(static_cast<IntensityLevel>(currentIntensity));
    }
    
    ImGui::Separator();
    
    // Transition type selection
    static const char* transitionLabels[] = { "Immediate", "Crossfade", "Sequential" };
    static int currentTransition = 0;
    
    ImGui::Text("Transition Type:");
    ImGui::Combo("##transition", &currentTransition, transitionLabels, IM_ARRAYSIZE(transitionLabels));
    
    ImGui::End();
}

// Render visualization panel
void RenderVisualizationPanel() {
    ImGui::Begin("Visualization");
    
    // In a real app, you would have actual audio visualization here
    // For this demo, we'll just use a placeholder
    
    ImGui::Text("Audio Visualization");
    
    // Draw a fake visualization
    const float width = ImGui::GetContentRegionAvail().x;
    const float height = 100;
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Background
    draw_list->AddRectFilled(
        ImVec2(p.x, p.y), 
        ImVec2(p.x + width, p.y + height), 
        IM_COL32(50, 50, 50, 255)
    );
    
    // Draw fake waveform
    static float phase = 0.0f;
    phase += 0.05f;
    if (phase > 10.0f) phase -= 10.0f;
    
    const int segments = 100;
    const float dx = width / segments;
    
    for (int i = 0; i < segments; i++) {
        float x1 = p.x + i * dx;
        float x2 = p.x + (i + 1) * dx;
        
        // Only animate if music is playing
        float amplitude = musicSystem.IsPlaying() ? 
            0.5f + 0.5f * sinf((i * 0.2f) + phase) : 0.1f;
        
        float y1 = p.y + height * 0.5f + height * 0.4f * amplitude;
        float y2 = p.y + height * 0.5f - height * 0.4f * amplitude;
        
        draw_list->AddRectFilled(
            ImVec2(x1, y1), 
            ImVec2(x2, y2), 
            IM_COL32(0, 255, 0, 255)
        );
    }
    
    ImGui::Dummy(ImVec2(width, height));
    
    ImGui::End();
}

// Render debug panel
void RenderDebugPanel() {
    ImGui::Begin("Debug");
    
    // Show some debug info about the music system
    ImGui::Text("Current Intensity: %s", 
        musicSystem.GetIntensityNames().at(musicSystem.GetCurrentIntensity()).c_str());
    
    ImGui::Text("Layers: %zu", musicSystem.GetLayers().size());
    
    ImGui::Text("Music Playing: %s", musicSystem.IsPlaying() ? "Yes" : "No");
    ImGui::Text("Engine Initialized: %s", musicSystem.IsInitialized() ? "Yes" : "No");
    
    ImGui::End();
}

// Main application
int main(int argc, char* argv[]) {
    // Initialize SDL and ImGui
    if (!Initialize()) {
        return -1;
    }
    
    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window)) {
                done = true;
            }
        }
        
        // Update music system
        musicSystem.Update();
        
        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        // Main menu bar
        RenderMainMenuBar();
        
        // ImGui demo window
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }
        
        // Render panels
        RenderLayersPanel();
        RenderControlsPanel();
        RenderVisualizationPanel();
        RenderDebugPanel();
        
        // Render ImGui
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    
    // Cleanup
    Cleanup();
    
    return 0;
} 