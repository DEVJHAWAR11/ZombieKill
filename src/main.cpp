// Author: Dev Jhawar
// Institution: KIIT University

#include <iostream>
#include <string>
#include "SystemHandleScanner.h"
#include "ProcessKiller.h"

// The main entry point of the C++ program (Execution starts here)
int wmain(int argc, wchar_t* argv[]) {
    // Check if the user provided a file path as an argument (like dragging and dropping a file onto the exe)
    if (argc != 2) {
        std::wcout << L"Usage: Drag and drop a locked file onto this executable." << std::endl;
        std::wcout << L"Or run from terminal: unstuck_utility.exe <path_to_file>" << std::endl;
        return 1;
    }

    // Extract the target file path from the command line arguments array
    std::wstring targetFile = argv[1];
    
    std::wcout << L"--- The Unstuck Forced File Unlocker ---" << std::endl;
    std::wcout << L"Target file: " << targetFile << std::endl;
    std::wcout << L"Scanning system for locking processes... (This may take a moment)" << std::endl;

    // Create an instance of our handle scanner class
    SystemHandleScanner scanner;
    
    // Call the main algorithm to find all locks on our file
    std::vector<LockInfo> locks = scanner.findLocksForFile(targetFile);

    // Check if we found any processes holding the file
    if (locks.empty()) {
        std::wcout << L"No locks found for this file. It might not be locked, or we lack admin privileges." << std::endl;
        
        // Try deleting it anyway, just in case it wasn't actually locked by a process
        std::wcout << L"Attempting to delete it anyway..." << std::endl;
        ProcessKiller::deleteUnlockedFile(targetFile);
        return 0;
    }

    // Print out how many locks we found
    std::wcout << L"Found " << locks.size() << L" process(es) locking the file." << std::endl;
    
    // Attempt to terminate all processes we found in the array
    bool killSuccess = ProcessKiller::killProcesses(locks);

    // If we killed the processes successfully
    if (killSuccess) {
        std::wcout << L"All blocking processes terminated." << std::endl;
        // Now try to delete the file, as it should be unlocked and free
        ProcessKiller::deleteUnlockedFile(targetFile);
    } else {
        std::wcout << L"Failed to terminate some processes. File may still be locked." << std::endl;
        // Try deleting anyway, it might work if the handle was released despite the error
        ProcessKiller::deleteUnlockedFile(targetFile);
    }

    std::wcout << L"Done." << std::endl;
    // Return 0 to indicate successful program execution to the OS
    return 0;
}

