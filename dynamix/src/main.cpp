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
#include "music_tester_api.h"

// Global controller instance for external API
static Dynamix::AudioController* g_controller = nullptr;

// ============================================================================
// ROBUST EXTERNAL API IMPLEMENTATION WITH COMPREHENSIVE ERROR HANDLING
// ============================================================================

// Global error state for API
static DynamixErrorCode g_lastErrorCode = DYNAMIX_SUCCESS;
static std::string g_lastErrorMessage = "";
static const size_t MAX_PARAM_LENGTH = 1024;

// Helper function to set API error state
static DynamixErrorCode setApiError(DynamixErrorCode code, const std::string& message) {
    g_lastErrorCode = code;
    g_lastErrorMessage = message;
    LOG_ERROR("Dynamix API Error: " + message);
    return code;
}

// Helper function to validate string parameters
static DynamixErrorCode validateStringParam(const char* param, const char* paramName) {
    if (param == nullptr) {
        return setApiError(DYNAMIX_ERROR_NULL_PARAM, 
                          std::string("Parameter '") + paramName + "' is NULL");
    }
    if (strlen(param) == 0) {
        return setApiError(DYNAMIX_ERROR_EMPTY_PARAM, 
                          std::string("Parameter '") + paramName + "' is empty");
    }
    if (strlen(param) > MAX_PARAM_LENGTH) {
        return setApiError(DYNAMIX_ERROR_PARAMETER_TOO_LONG, 
                          std::string("Parameter '") + paramName + "' exceeds maximum length of " + std::to_string(MAX_PARAM_LENGTH));
    }
    return DYNAMIX_SUCCESS;
}

// Helper function to check if controller is initialized
static DynamixErrorCode checkControllerInitialized() {
    if (g_controller == nullptr) {
        return setApiError(DYNAMIX_ERROR_NOT_INITIALIZED, 
                          "Dynamix not initialized. Call dynamix_initialize() first.");
    }
    return DYNAMIX_SUCCESS;
}

// External C API for game engine integration
extern "C" {

// Error reporting functions
const char* dynamix_getLastError(void) {
    return g_lastErrorMessage.empty() ? nullptr : g_lastErrorMessage.c_str();
}

DynamixErrorCode dynamix_getLastErrorCode(void) {
    return g_lastErrorCode;
}

void dynamix_clearError(void) {
    g_lastErrorCode = DYNAMIX_SUCCESS;
    g_lastErrorMessage.clear();
}

// Core API functions with robust error handling
DynamixErrorCode dynamix_triggerSongEvent(const char* songName, const char* eventName) {
    // Clear previous error
    dynamix_clearError();
    
    // Validate parameters
    DynamixErrorCode result = validateStringParam(songName, "songName");
    if (result != DYNAMIX_SUCCESS) return result;
    
    result = validateStringParam(eventName, "eventName");
    if (result != DYNAMIX_SUCCESS) return result;
    
    // Check initialization
    result = checkControllerInitialized();
    if (result != DYNAMIX_SUCCESS) return result;
    
    try {
        g_controller->triggerSongEvent(songName, eventName);
        return DYNAMIX_SUCCESS;
    } catch (const std::exception& e) {
        return setApiError(DYNAMIX_ERROR_INTERNAL, 
                          std::string("Internal error in triggerSongEvent: ") + e.what());
    }
}

DynamixErrorCode dynamix_triggerMasterEvent(const char* eventName) {
    // Clear previous error
    dynamix_clearError();
    
    // Validate parameters
    DynamixErrorCode result = validateStringParam(eventName, "eventName");
    if (result != DYNAMIX_SUCCESS) return result;
    
    // Check initialization
    result = checkControllerInitialized();
    if (result != DYNAMIX_SUCCESS) return result;
    
    try {
        g_controller->triggerMasterEvent(eventName);
        return DYNAMIX_SUCCESS;
    } catch (const std::exception& e) {
        return setApiError(DYNAMIX_ERROR_INTERNAL, 
                          std::string("Internal error in triggerMasterEvent: ") + e.what());
    }
}

DynamixErrorCode dynamix_triggerEvent(const char* eventName) {
    // Clear previous error
    dynamix_clearError();
    
    // Validate parameters
    DynamixErrorCode result = validateStringParam(eventName, "eventName");
    if (result != DYNAMIX_SUCCESS) return result;
    
    // Check initialization
    result = checkControllerInitialized();
    if (result != DYNAMIX_SUCCESS) return result;
    
    try {
        g_controller->triggerEvent(eventName);
        return DYNAMIX_SUCCESS;
    } catch (const std::exception& e) {
        return setApiError(DYNAMIX_ERROR_INTERNAL, 
                          std::string("Internal error in triggerEvent: ") + e.what());
    }
}

DynamixErrorCode dynamix_loadSongsFromDirectory(const char* directory) {
    // Clear previous error
    dynamix_clearError();
    
    // Validate parameters
    DynamixErrorCode result = validateStringParam(directory, "directory");
    if (result != DYNAMIX_SUCCESS) return result;
    
    // Check initialization
    result = checkControllerInitialized();
    if (result != DYNAMIX_SUCCESS) return result;
    
    // Validate directory exists
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        return setApiError(DYNAMIX_ERROR_INVALID_DIRECTORY, 
                          std::string("Directory does not exist or is not a directory: ") + directory);
    }
    
    try {
        g_controller->setMusicDirectory(directory);
        g_controller->loadMusicFromDirectory();
        return DYNAMIX_SUCCESS;
    } catch (const std::exception& e) {
        return setApiError(DYNAMIX_ERROR_INTERNAL, 
                          std::string("Internal error in loadSongsFromDirectory: ") + e.what());
    }
}

DynamixErrorCode dynamix_loadAndPlaySong(const char* songName) {
    // Clear previous error
    dynamix_clearError();
    
    // Validate parameters
    DynamixErrorCode result = validateStringParam(songName, "songName");
    if (result != DYNAMIX_SUCCESS) return result;
    
    // Check initialization
    result = checkControllerInitialized();
    if (result != DYNAMIX_SUCCESS) return result;
    
    try {
        g_controller->setCurrentSong(songName);
        return DYNAMIX_SUCCESS;
    } catch (const std::exception& e) {
        return setApiError(DYNAMIX_ERROR_INTERNAL, 
                          std::string("Internal error in loadAndPlaySong: ") + e.what());
    }
}

DynamixErrorCode dynamix_initialize(const char* executableDirectory) {
    // Clear previous error
    dynamix_clearError();
    
    // Don't validate executableDirectory - it's optional (can be NULL)
    if (executableDirectory != nullptr) {
        DynamixErrorCode result = validateStringParam(executableDirectory, "executableDirectory");
        if (result != DYNAMIX_SUCCESS) return result;
    }
    
    try {
        // Create global controller if not already exists
        if (g_controller == nullptr) {
            static Dynamix::AudioController controller; // Static to keep it alive
            g_controller = &controller;
        }
        
        // Initialize with directory
        std::string directory = executableDirectory ? executableDirectory : "";
        if (!g_controller->initialize(directory)) {
            g_controller = nullptr;
            return setApiError(DYNAMIX_ERROR_INTERNAL, "Failed to initialize Dynamix system");
        }
        
        return DYNAMIX_SUCCESS;
    } catch (const std::exception& e) {
        g_controller = nullptr;
        return setApiError(DYNAMIX_ERROR_INTERNAL, 
                          std::string("Internal error in initialize: ") + e.what());
    }
}

void dynamix_shutdown(void) {
    dynamix_clearError();
    
    if (g_controller) {
        try {
            g_controller->cleanup();
            g_controller = nullptr;
        } catch (const std::exception& e) {
            LOG_ERROR("Error during shutdown: " + std::string(e.what()));
        }
    }
}

int dynamix_isInitialized(void) {
    return (g_controller != nullptr) ? 1 : 0;
}

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