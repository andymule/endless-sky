cmake_minimum_required(VERSION 3.24)
project(dynamix_minimal VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Add warning flags
add_compile_options(-Wall -Wextra)

# Add the executable target
add_executable(dynamix_minimal ../src/main_minimal.cpp) 