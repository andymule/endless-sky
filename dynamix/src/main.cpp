#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef _WIN32
#undef min
#undef max
#endif
#include <filesystem>
#include <iostream>

// macOS specific includes
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Dear ImGui includes
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "AudioController.h"
#include "EventSystem.h"
#include "Logger.h"
#include "MainView.h"

// Global controller instance for external API
static Dynamix::AudioController* g_controller = nullptr;

// External C API for game engine integration
extern "C" {
void dynamix_triggerSongEvent(const char* songName, const char* eventName) {
    if (g_controller) {
        g_controller->triggerSongEvent(songName, eventName);
    }
}

void dynamix_triggerMasterEvent(const char* eventName) {
    if (g_controller) {
        g_controller->triggerMasterEvent(eventName);
    }
}

// Searches both songs and master events for the event name
void dynamix_triggerEvent(const char* eventName) {
    if (g_controller) {
        g_controller->triggerEvent(eventName);
    }
}

// Additional utility functions
void dynamix_loadSongsFromDirectory(const char* directory) {
    if (g_controller) {
        g_controller->setMusicDirectory(directory);
    }
}

// TODO loadAndPlaySong()
}

// Get the directory where the executable is located
std::string getExecutableDirectory() {
#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::filesystem::path exePath(path);
        return exePath.parent_path().string();
    }
#elif defined(_WIN32)
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) != 0) {
        std::filesystem::path exePath(path);
        return exePath.parent_path().string();
    }
#endif

    return ".";
}

// Windows-specific SDL main handling
#ifdef _WIN32
#define SDL_MAIN_HANDLED
#include <SDL2/SDL_main.h>
#endif

int main(int argc, char* argv[]) {
    LOG_INFO("Starting Dynamix application");
    
    // Get the executable directory for proper path resolution
    std::string exeDir = getExecutableDirectory();
    LOG_INFO("Executable directory: " + exeDir);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("Error initializing SDL: " + std::string(SDL_GetError()));
        return 1;
    }

    // Configure OpenGL context for macOS
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window* window =
        SDL_CreateWindow("Dynamix", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        LOG_ERROR("Error creating SDL window: " + std::string(SDL_GetError()));
        SDL_Quit();
        return 1;
    }

    // Load and set window icon
    SDL_Surface* iconSurface = SDL_LoadBMP("assets/icon.bmp");
    if (iconSurface) {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_FreeSurface(iconSurface);
    } else {
        LOG_INFO("Note: No icon.bmp found. For PNG support, install SDL2_image and use SDL_image.h");
    }

    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        LOG_ERROR("Error creating OpenGL context: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Create MVC components
    Dynamix::AudioController controller;
    MainView view;
    
    // Store global controller reference for external API
    g_controller = &controller;

    // Wire up the MVC architecture
    view.SetController(&controller);

    // Initialize components with executable directory
    if (!controller.initialize(exeDir)) {
        LOG_ERROR("Failed to initialize AudioController");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!view.Initialize(window, glContext)) {
        LOG_ERROR("Failed to initialize MainView");
        controller.cleanup();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize timing for deltaTime calculation
    Uint32 lastTime = SDL_GetTicks();

    // Main loop
    while (view.IsRunning()) {
        // Calculate deltaTime
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert to seconds
        lastTime = currentTime;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            view.ProcessEvents(event);
            if (event.type == SDL_QUIT)
                view.SetRunning(false);
        }

        // Update synchronization (call this regularly to prevent drift)
        controller.updateSync();

        // Update event system transitions
        controller.updateEvents(deltaTime);

        view.Render();
    }

    // Cleanup
    g_controller = nullptr; // Clear global reference
    controller.cleanup();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    LOG_INFO("Program exiting normally");
    return 0;
} 