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
}

// Get the directory where the executable is located
std::string getExecutableDirectory() {
// On macOS, we can use _NSGetExecutablePath
#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::filesystem::path exePath(path);
        return exePath.parent_path().string();
    }
#elif defined(_WIN32)
    // On Windows, use GetModuleFileName to get the executable path
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) != 0) {
        std::filesystem::path exePath(path);
        return exePath.parent_path().string();
    }
#endif

    // Fallback: use a simple approach without filesystem operations
    return ".";
}

int main() {
#ifdef _WIN32
    MessageBoxA(NULL, "main() reached", "Dynamix", MB_OK);
#endif
    std::cout << "[LOG] Entered main()" << std::endl;
    // Get the executable directory for proper path resolution
    std::string exeDir = getExecutableDirectory();
    std::cout << "[LOG] Got executable directory: " << exeDir << std::endl;
    LOG_INFO("Executable directory: " + exeDir);

    std::cout << "[LOG] Initializing SDL" << std::endl;
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("Error initializing SDL: " + std::string(SDL_GetError()));
        std::cout << "[LOG] SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "[LOG] SDL initialized" << std::endl;

    // For MacOS, use OpenGL 3.2 Core Profile
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    std::cout << "[LOG] Creating SDL window" << std::endl;
    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window* window =
        SDL_CreateWindow("Dynamix", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        LOG_ERROR("Error creating SDL window: " + std::string(SDL_GetError()));
        std::cout << "[LOG] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "[LOG] SDL window created" << std::endl;

    // Load and set window icon
    SDL_Surface* iconSurface = SDL_LoadBMP("assets/icon.bmp");
    if (iconSurface) {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_FreeSurface(iconSurface);
    } else {
        // Try PNG format (requires SDL2_image)
        LOG_INFO(
            "Note: No icon.bmp found. For PNG support, install SDL2_image and use SDL_image.h");
    }

    std::cout << "[LOG] Creating OpenGL context" << std::endl;
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        LOG_ERROR("Error creating OpenGL context: " + std::string(SDL_GetError()));
        std::cout << "[LOG] SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    std::cout << "[LOG] OpenGL context created" << std::endl;

    // Create MVC components
    std::cout << "[LOG] Creating AudioController and MainView" << std::endl;
    Dynamix::AudioController controller;
    MainView view;
    std::cout << "[LOG] AudioController and MainView created" << std::endl;

    // Store global controller reference for external API
    g_controller = &controller;

    // Wire up the MVC architecture
    view.SetController(&controller);

    std::cout << "[LOG] Initializing AudioController" << std::endl;
    // Initialize components with executable directory
    if (!controller.initialize(exeDir)) {
        LOG_ERROR("Failed to initialize AudioController");
        std::cout << "[LOG] AudioController initialization failed" << std::endl;
        return 1;
    }
    std::cout << "[LOG] AudioController initialized" << std::endl;

    std::cout << "[LOG] Initializing MainView" << std::endl;
    if (!view.Initialize(window, glContext)) {
        LOG_ERROR("Failed to initialize TesterView");
        std::cout << "[LOG] MainView initialization failed" << std::endl;
        return 1;
    }
    std::cout << "[LOG] MainView initialized" << std::endl;

    // Initialize timing for deltaTime calculation
    Uint32 lastTime = SDL_GetTicks();

    // Main loop
    std::cout << "[LOG] Entering main loop" << std::endl;
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
    std::cout << "[LOG] Exited main loop" << std::endl;

    // Cleanup
    g_controller = nullptr; // Clear global reference
    controller.cleanup();

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "[LOG] Program exiting normally" << std::endl;
    return 0;
}