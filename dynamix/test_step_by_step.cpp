#include <iostream>

// Test 1: Basic includes
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Test 2: Filesystem
#include <filesystem>

// Test 3: SDL2
#include <SDL2/SDL.h>

// Test 4: ImGui
#include "imgui.h"

// Test 5: SoLoud
#include "soloud.h"

int main() {
    std::cout << "[TEST] Entered main()" << std::endl;
    
    // Test basic functionality
    std::cout << "[TEST] Basic C++ working" << std::endl;
    
    // Test filesystem
    std::cout << "[TEST] Filesystem working" << std::endl;
    
    // Test SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "[ERROR] SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    std::cout << "[TEST] SDL2 working" << std::endl;
    SDL_Quit();
    
    // Test ImGui
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    if (ctx) {
        std::cout << "[TEST] ImGui working" << std::endl;
        ImGui::DestroyContext(ctx);
    } else {
        std::cout << "[ERROR] ImGui failed" << std::endl;
        return 1;
    }
    
    // Test SoLoud
    SoLoud::Soloud soloud;
    if (soloud.init() == SoLoud::SO_NO_ERROR) {
        std::cout << "[TEST] SoLoud working" << std::endl;
        soloud.deinit();
    } else {
        std::cout << "[ERROR] SoLoud failed" << std::endl;
        return 1;
    }
    
    std::cout << "[TEST] All tests passed!" << std::endl;
    return 0;
} 