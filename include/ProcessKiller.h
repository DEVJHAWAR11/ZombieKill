// Author: Dev Jhawar
// Institution: KIIT University

#pragma once // Prevent duplicate inclusion to ensure clean builds

#include "SystemHandleScanner.h" // Include our scanner header because we need the LockInfo struct defined there
#include <string> // Include string for passing file paths
#include <vector> // Include vector to take a list of locks

// The class responsible for neutralizing the processes or handles holding our file hostage
class ProcessKiller { // Define the class blueprint
public: // Accessible interface for this class
    // Tries to forcefully terminate the processes holding the file
    static bool killProcesses(const std::vector<LockInfo>& locks); // Takes our discovered locks and kills the owning programs (Static means we don't need to create an object to use it)

    // Deletes the file once it is unlocked
    static bool deleteUnlockedFile(const std::wstring& filePath); // Deletes the target file from the filesystem
}; // End of the class definition
