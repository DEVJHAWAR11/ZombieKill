// Author: Dev Jhawar
// Institution: KIIT University

#include <iostream> // Include for standard console input and output (cin/cout)
#include <string> // Include for string manipulation classes
#include "SystemHandleScanner.h" // Include our scanner logic to find locked files
#include "ProcessKiller.h" // Include our process killer logic to terminate them

// The main entry point of the C++ program (Execution starts here)
int wmain(int argc, wchar_t* argv[]) { // wmain is used instead of main for wide-character (Unicode) command line arguments on Windows
    // Check if the user provided a file path as an argument (like dragging and dropping a file onto the exe)
    if (argc != 2) { // argc is argument count. It should be 2: [program_name, target_file]
        std::wcout << L"Usage: Drag and drop a locked file onto this executable." << std::endl; // Print simple usage instructions
        std::wcout << L"Or run from terminal: unstuck_utility.exe <path_to_file>" << std::endl; // Print terminal usage instructions
        return 1; // Return a non-zero integer to indicate an error state to the Operating System
    } // End of argument check

    // Extract the target file path from the command line arguments array
    std::wstring targetFile = argv[1]; // argv[1] contains the first argument provided by the user (index 1 in the string array)
    
    std::wcout << L"--- The Unstuck Forced File Unlocker ---" << std::endl; // Print our program banner
    std::wcout << L"Target file: " << targetFile << std::endl; // Print the path of the file we are targeting
    std::wcout << L"Scanning system for locking processes... (This may take a moment)" << std::endl; // Print a status message because scanning is slow

    // Create an instance of our handle scanner class
    SystemHandleScanner scanner; // Instantiates the object on the stack (automatic memory management, no 'new' keyword needed)
    
    // Call the main algorithm to find all locks on our file
    std::vector<LockInfo> locks = scanner.findLocksForFile(targetFile); // Run the O(N) system-wide scan and store results in a dynamic array

    // Check if we found any processes holding the file
    if (locks.empty()) { // If the returned dynamic array is empty (size is 0)
        std::wcout << L"No locks found for this file. It might not be locked, or we lack admin privileges." << std::endl; // Print information message
        
        // Try deleting it anyway, just in case it wasn't actually locked by a process
        std::wcout << L"Attempting to delete it anyway..." << std::endl; // Log our intention
        ProcessKiller::deleteUnlockedFile(targetFile); // Call the static deletion method
        return 0; // Exit successfully, as the job is done
    } // End of empty locks check

    // Print out how many locks we found
    std::wcout << L"Found " << locks.size() << L" process(es) locking the file." << std::endl; // Output the size property of the vector
    
    // Attempt to terminate all processes we found in the array
    bool killSuccess = ProcessKiller::killProcesses(locks); // Call the static killer method passing our locks vector

    // If we killed the processes successfully
    if (killSuccess) { // Check the boolean result
        std::wcout << L"All blocking processes terminated." << std::endl; // Print success message
        // Now try to delete the file, as it should be unlocked and free
        ProcessKiller::deleteUnlockedFile(targetFile); // Call the deletion method
    } else { // If we failed to kill some processes
        std::wcout << L"Failed to terminate some processes. File may still be locked." << std::endl; // Print failure message
        // Try deleting anyway, it might work if the handle was released despite the error
        ProcessKiller::deleteUnlockedFile(targetFile); // Attempt deletion just in case
    } // End of kill success check

    std::wcout << L"Done." << std::endl; // Print final completion message
    // Return 0 to indicate successful program execution to the OS
    return 0; // Standard success exit code in C++
} // End of the wmain function
