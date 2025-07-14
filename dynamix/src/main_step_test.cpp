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

// Test 1: Basic includes only (this works)
// Test 2: Add SDL2
#include <SDL2/SDL.h>

// Test 3: Add ImGui
#include "imgui.h"

// Test 4: Add SoLoud
#include "soloud.h"

int main() {
#ifdef _WIN32
    MessageBoxA(NULL, "main() reached", "Dynamix", MB_OK);
#endif
    std::cout << "[LOG] Entered main()" << std::endl;
    
    // Test basic functionality
    std::cout << "[LOG] Basic C++ working" << std::endl;
    
    // Test filesystem
    std::cout << "[LOG] Filesystem working" << std::endl;
    
    // Test SDL2
    std::cout << "[LOG] Testing SDL2..." << std::endl;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "[ERROR] SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "[LOG] SDL2 working" << std::endl;
    SDL_Quit();
    
    // Test ImGui
    std::cout << "[LOG] Testing ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    if (ctx) {
        std::cout << "[LOG] ImGui working" << std::endl;
        ImGui::DestroyContext(ctx);
    } else {
        std::cout << "[ERROR] ImGui failed" << std::endl;
        return 1;
    }
    
    // Test SoLoud
    std::cout << "[LOG] Testing SoLoud..." << std::endl;
    SoLoud::Soloud soloud;
    if (soloud.init() == SoLoud::SO_NO_ERROR) {
        std::cout << "[LOG] SoLoud working" << std::endl;
        soloud.deinit();
    } else {
        std::cout << "[ERROR] SoLoud failed" << std::endl;
        return 1;
    }
    
    std::cout << "[LOG] All tests passed!" << std::endl;
    return 0;
} 