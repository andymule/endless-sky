#include <iostream>
#include <filesystem>

int main() {
    std::cout << "Starting crash test..." << std::endl;
    
    try {
        // Test 1: Basic filesystem operations
        std::cout << "Test 1: Current path" << std::endl;
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::cout << "Current path: " << currentPath.string() << std::endl;
        
        // Test 2: Executable directory (simulate what main.cpp does)
        std::cout << "Test 2: Executable directory" << std::endl;
        std::string exeDir = currentPath.string();
        std::cout << "Executable directory: " << exeDir << std::endl;
        
        // Test 3: Music directory creation
        std::cout << "Test 3: Music directory" << std::endl;
        const char* userProfile = getenv("USERPROFILE");
        std::string musicDir;
        if (userProfile) {
            musicDir = std::string(userProfile) + "\\Music\\Dynamix";
        } else {
            musicDir = ".\\Music\\Dynamix";
        }
        std::cout << "Music directory: " << musicDir << std::endl;
        
        // Test 4: Check if directory exists
        std::cout << "Test 4: Directory exists check" << std::endl;
        bool exists = std::filesystem::exists(musicDir);
        std::cout << "Directory exists: " << (exists ? "yes" : "no") << std::endl;
        
        // Test 5: Create directory if it doesn't exist
        if (!exists) {
            std::cout << "Test 5: Creating directory" << std::endl;
            std::filesystem::create_directories(musicDir);
            std::cout << "Directory created successfully" << std::endl;
        }
        
        std::cout << "All tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
    
    return 0;
} 