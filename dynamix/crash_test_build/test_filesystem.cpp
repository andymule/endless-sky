#include <iostream>

int main() {
    std::cout << "Starting filesystem test..." << std::endl;
    
    try {
        std::cout << "Test 1: Basic iostream" << std::endl;
        std::cout << "This works" << std::endl;
        
        std::cout << "Test 2: About to include filesystem" << std::endl;
        // Don't include filesystem yet - let's see if the issue is with the include itself
        
        std::cout << "Test 2 passed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
} 