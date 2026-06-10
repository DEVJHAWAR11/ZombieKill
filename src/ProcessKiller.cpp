// Author: Dev Jhawar
// Institution: KIIT University

#include "ProcessKiller.h"
#include <iostream>
#include <windows.h>

// Tries to forcefully terminate the processes holding the file
bool ProcessKiller::killProcesses(const std::vector<LockInfo>& locks) {
    // If the list of locks is empty, there's nothing to kill
    if (locks.empty()) {
        std::wcout << L"No locks provided to kill." << std::endl;
        return true;
    }

    // Keep track of overall success
    bool allKilled = true;

    // Loop over every lock we found (O(N) iteration, where N is the number of locks)
    for (size_t i = 0; i < locks.size(); ++i) {
        // Get the current lock information
        const LockInfo& lock = locks[i];
        
        std::wcout << L"Attempting to terminate process ID: " << lock.processId << std::endl;
        
        // Open a handle to the target process with termination privileges
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, lock.processId);
        
        // If we successfully opened the process
        if (hProcess != NULL) {
            // Ask the OS to kill the process forcefully (exit code 1)
            BOOL result = TerminateProcess(hProcess, 1);
            
            // Check if termination succeeded
            if (result) {
                std::wcout << L"Successfully terminated process " << lock.processId << std::endl;
            } else {
                std::wcout << L"Failed to terminate process " << lock.processId << L". Error: " << GetLastError() << std::endl;
                allKilled = false;
            }
            
            // Clean up the process handle
            CloseHandle(hProcess);
        } else {
            std::wcout << L"Could not open process " << lock.processId << L" to terminate. Error: " << GetLastError() << std::endl;
            allKilled = false;
        }
    }
    
    // Return whether we successfully killed all blocking processes
    return allKilled;
}

// Deletes the target file from the filesystem
bool ProcessKiller::deleteUnlockedFile(const std::wstring& filePath) {
    // Try to delete the file using the standard Windows API
    BOOL result = DeleteFileW(filePath.c_str());
    
    // If deletion succeeded
    if (result) {
        std::wcout << L"Successfully deleted file: " << filePath << std::endl;
        return true;
    } else {
        std::wcout << L"Failed to delete file. Error: " << GetLastError() << std::endl;
        return false;
    }
}

