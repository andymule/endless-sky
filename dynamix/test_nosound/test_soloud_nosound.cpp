#include <iostream>
#include "soloud.h"

int main() {
    std::cout << "[TEST] Starting SoLoud nosound test..." << std::endl;
    
    SoLoud::Soloud soloud;
    std::cout << "[TEST] Created SoLoud instance" << std::endl;
    
    int result = soloud.init();
    std::cout << "[TEST] SoLoud init result: " << result << std::endl;
    
    if (result == 0) {
        std::cout << "[TEST] SoLoud initialized successfully!" << std::endl;
        soloud.deinit();
        std::cout << "[TEST] SoLoud deinitialized" << std::endl;
    } else {
        std::cout << "[TEST] SoLoud initialization failed!" << std::endl;
    }
    
    std::cout << "[TEST] Test completed" << std::endl;
    return 0;
} 