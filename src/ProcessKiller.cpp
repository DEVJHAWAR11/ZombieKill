// Author: Dev Jhawar
// Institution: KIIT University

#include "ProcessKiller.h" // Include the header file that defines the ProcessKiller class blueprint
#include <iostream> // Include the standard library for printing text to the console screen
#include <windows.h> // Include standard Windows API functions for process termination and file deletion

// Tries to forcefully terminate the processes holding the file
bool ProcessKiller::killProcesses(const std::vector<LockInfo>& locks) { // Function definition taking a constant reference to a dynamic array (vector) of LockInfo structures
    // If the list of locks is empty, there's nothing to kill
    if (locks.empty()) { // Check if the size of the vector is 0 (O(1) operation)
        std::wcout << L"No locks provided to kill." << std::endl; // Print a message indicating no action is needed
        return true; // Technically a success since nothing is blocking us anymore
    } // End of the if condition block

    // Keep track of overall success
    bool allKilled = true; // Initialize a boolean flag to true, which we will flip to false if any termination fails

    // Loop over every lock we found (O(N) iteration, where N is the number of locks)
    for (size_t i = 0; i < locks.size(); ++i) { // Standard for-loop from index 0 to the end of the vector
        // Get the current lock information
        const LockInfo& lock = locks[i]; // Create a constant reference to the current item to avoid expensive memory copying
        
        std::wcout << L"Attempting to terminate process ID: " << lock.processId << std::endl; // Log our intention to kill this specific Process ID (PID)
        
        // Open a handle to the target process with termination privileges
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, lock.processId); // Call OS function to get a control handle, requesting only the right to terminate
        
        // If we successfully opened the process
        if (hProcess != NULL) { // Check if the handle returned by the OS is valid (not null)
            // Ask the OS to kill the process forcefully (exit code 1)
            BOOL result = TerminateProcess(hProcess, 1); // Send a lethal kernel-level signal to instantly stop the process
            
            // Check if termination succeeded
            if (result) { // If the TerminateProcess function returned true
                std::wcout << L"Successfully terminated process " << lock.processId << std::endl; // Log that we successfully killed it
            } else { // If the function returned false
                std::wcout << L"Failed to terminate process " << lock.processId << L". Error: " << GetLastError() << std::endl; // Log the failure and ask the OS for the specific error code
                allKilled = false; // Mark our overall success flag as false because one failed
            } // End of the termination success check
            
            // Clean up the process handle
            CloseHandle(hProcess); // Always release OS handles to prevent memory/resource leaks in our utility
        } else { // If OpenProcess failed (maybe it's an anti-virus or critical system process)
            std::wcout << L"Could not open process " << lock.processId << L" to terminate. Error: " << GetLastError() << std::endl; // Log that we couldn't even get access to it
            allKilled = false; // Mark our overall success flag as false
        } // End of the OpenProcess success check
    } // End of the for-loop
    
    // Return whether we successfully killed all blocking processes
    return allKilled; // Return our boolean flag back to the caller
} // End of the killProcesses function

// Deletes the target file from the filesystem
bool ProcessKiller::deleteUnlockedFile(const std::wstring& filePath) { // Function definition taking the file path as a string
    // Try to delete the file using the standard Windows API
    BOOL result = DeleteFileW(filePath.c_str()); // Call the OS deletion function, converting our C++ string to a raw C-style character pointer
    
    // If deletion succeeded
    if (result) { // If the OS returned true
        std::wcout << L"Successfully deleted file: " << filePath << std::endl; // Log that the file is finally gone
        return true; // Return true for success
    } else { // If the OS returned false (maybe another process grabbed it, or we lack permissions)
        std::wcout << L"Failed to delete file. Error: " << GetLastError() << std::endl; // Log the failure and the OS error code
        return false; // Return false for failure
    } // End of the deletion success check
} // End of the deleteUnlockedFile function
