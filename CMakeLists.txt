# Author: Dev Jhawar
# Institution: KIIT University

# Specify the minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.10)

# Define the name of our project and the programming language we are using
project(UnstuckUtility CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Tell CMake where to look for our header (.h) files
include_directories(include)

# Define the source files that make up our executable
set(SOURCES
    src/main.cpp
    src/SystemHandleScanner.cpp
    src/ProcessKiller.cpp
)

# Create the final executable target using our source files
add_executable(unstuck_utility ${SOURCES})

# Link against standard Windows libraries needed for OS-level interactions
# 'ntdll' is specifically needed for the hidden Native APIs (NtQuerySystemInformation) we are using
target_link_libraries(unstuck_utility ntdll)

