// Author: Dev Jhawar
// Institution: KIIT University

#pragma once // Prevent multiple inclusions to avoid redefinition errors during compilation

#include <string> // Include the C++ string class for text manipulation
#include <vector> // Include the vector container (like a dynamic array in DSA)
#include <windows.h> // Include Windows definitions needed for HANDLE and DWORD types

// A structure to hold information about a file lock we discover
struct LockInfo { // Define a custom data structure (like a struct in C/DSA)
    DWORD processId; // A 32-bit integer holding the PID of the process locking the file
    std::wstring processName; // A wide string holding the name of the executable
    HANDLE handleValue; // The specific memory ticket (handle) the process has for our file
}; // End of the struct definition

// The class responsible for scanning the entire OS to find open file handles
class SystemHandleScanner { // Define our main scanner class blueprint
public: // Public members can be called from outside the class
    SystemHandleScanner(); // Constructor: initializes the scanner and loads hidden APIs
    ~SystemHandleScanner(); // Destructor: cleans up resources when the object is destroyed

    // The main algorithm function: finds all processes holding a lock on a specific file path
    std::vector<LockInfo> findLocksForFile(const std::wstring& targetFilePath); // Returns a dynamic array of lock information structs

private: // Private members are hidden, encapsulating the complex OS interactions
    HMODULE ntdllHandle; // A handle to the ntdll.dll system library
    void* ntQuerySystemInformationPtr; // A raw pointer to the NtQuerySystemInformation function
    void* ntQueryObjectPtr; // A raw pointer to the NtQueryObject function

    // Helper function to resolve the target file path to a Windows Native path format
    std::wstring getNativeFilePath(const std::wstring& dosPath); // Converts standard paths to OS internal paths
}; // End of the class definition
