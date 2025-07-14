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

int main() {
#ifdef _WIN32
    MessageBoxA(NULL, "main() reached", "Dynamix", MB_OK);
#endif
    std::cout << "[LOG] Entered main()" << std::endl;
    
    // Test basic functionality
    std::cout << "[LOG] Basic C++ working" << std::endl;
    
    // Test filesystem
    std::cout << "[LOG] Filesystem working" << std::endl;
    
    std::cout << "[LOG] Minimal main completed successfully!" << std::endl;
    return 0;
} 