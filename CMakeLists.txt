# Author: Dev Jhawar
# Institution: KIIT University

# Specify the minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.10) # We need at least version 3.10 for modern C++ features

# Define the name of our project and the programming language we are using
project(UnstuckUtility CXX) # CXX is the internal CMake identifier for the C++ language

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17) # We use C++17 to get access to modern standard library features
set(CMAKE_CXX_STANDARD_REQUIRED True) # Enforce that C++17 is strictly mandatory, not just optional

# Tell CMake where to look for our header (.h) files
include_directories(include) # Adds the 'include' directory to the compiler's search path so it finds our blueprints

# Define the source files that make up our executable
set(SOURCES # Create a variable named 'SOURCES'
    src/main.cpp # Add the main entry point
    src/SystemHandleScanner.cpp # Add the scanner logic
    src/ProcessKiller.cpp # Add the terminator logic
) # End of list assignment

# Create the final executable target using our source files
add_executable(unstuck_utility ${SOURCES}) # Tells CMake to compile the 'SOURCES' list into an executable file named 'unstuck_utility'

# Link against standard Windows libraries needed for OS-level interactions
# 'ntdll' is specifically needed for the hidden Native APIs (NtQuerySystemInformation) we are using
target_link_libraries(unstuck_utility ntdll) # Instructs the linker to bind ntdll.lib to our program during the final build phase
