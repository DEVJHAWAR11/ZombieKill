// Author: Dev Jhawar
// Institution: KIIT University

#include "SystemHandleScanner.h" // Include the corresponding header file for this class
#include "NativeAPI.h" // Include our custom definitions for hidden Windows NT APIs
#include <iostream> // Include input/output stream for printing debug messages
#include <algorithm> // Include algorithms library for transforming strings (like converting to lowercase)

// Constructor: Initializes the scanner object and links to the hidden Windows APIs
SystemHandleScanner::SystemHandleScanner() { // Scope resolution operator to define the constructor
    // Load the 'ntdll.dll' library into our process memory, which contains the low-level kernel APIs
    ntdllHandle = LoadLibraryA("ntdll.dll"); // We use LoadLibraryA to load a DLL dynamically at runtime
    
    // Check if the library loaded successfully
    if (ntdllHandle) { // If the handle is not null, it worked
        // Find the memory address of the NtQuerySystemInformation function inside the loaded DLL
        ntQuerySystemInformationPtr = (void*)GetProcAddress(ntdllHandle, "NtQuerySystemInformation"); // Like finding a specific page in a book
        
        // Find the memory address of the NtQueryObject function
        ntQueryObjectPtr = (void*)GetProcAddress(ntdllHandle, "NtQueryObject"); // GetProcAddress returns the exact function pointer
    } else { // If we failed to load ntdll
        ntQuerySystemInformationPtr = nullptr; // Set pointers to null so we don't crash later
        ntQueryObjectPtr = nullptr; // Set pointers to null to be safe
    } // End of initialization block
} // End of constructor

// Destructor: Cleans up memory and resources when the scanner object is destroyed
SystemHandleScanner::~SystemHandleScanner() { // Tilde denotes the destructor
    // Check if we previously loaded the ntdll library
    if (ntdllHandle) { // If the handle is valid
        FreeLibrary(ntdllHandle); // Release the DLL from memory to prevent resource leaks
    } // End of check
} // End of destructor

// Helper: Converts a regular file path (C:\folder\file.txt) to a native OS path (\Device\HarddiskVolume...\file.txt)
std::wstring SystemHandleScanner::getNativeFilePath(const std::wstring& dosPath) { // Function taking the standard DOS path
    // Create an empty string to hold the output native path
    std::wstring nativePath = L""; // Initialize to an empty wide string
    
    // Open a handle to the file itself just to ask the OS for its native name
    HANDLE hFile = CreateFileW( // Open the file using the wide-character version of CreateFile
        dosPath.c_str(), // Convert our C++ string to a raw C-style pointer
        0, // We request 0 access (no read/write), just metadata access
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // Allow other programs to still use it while we check
        nullptr, // No special security attributes
        OPEN_EXISTING, // Only open if the file actually exists
        FILE_FLAG_BACKUP_SEMANTICS, // Required flag to open directories or complex system files
        nullptr // No template file
    ); // End of CreateFileW call

    // If we successfully got a handle to the file
    if (hFile != INVALID_HANDLE_VALUE) { // INVALID_HANDLE_VALUE means opening failed
        // Prepare a buffer to store the native file path
        std::vector<wchar_t> buffer(MAX_PATH * 4); // Allocate a large array of wide characters (like a char array in DSA)
        
        // Ask the OS for the final, resolved native path of this file handle
        DWORD result = GetFinalPathNameByHandleW(hFile, buffer.data(), buffer.size(), VOLUME_NAME_NT); // VOLUME_NAME_NT requests the \Device\ path format
        
        // If the function succeeded, it returns the length of the string
        if (result > 0 && result < buffer.size()) { // Check bounds to avoid buffer overflow
            nativePath = std::wstring(buffer.data(), result); // Convert the raw character array into a clean C++ std::wstring
        } // End of string length check
        // Always close handles when done to prevent resource leaks
        CloseHandle(hFile); // Release our temporary handle to the file
    } // End of file open check
    
    // Return the converted path, or an empty string if it failed
    return nativePath; // Return the result to the caller
} // End of getNativeFilePath

// Main algorithm: Scans all OS handles to find which process is locking our target file
std::vector<LockInfo> SystemHandleScanner::findLocksForFile(const std::wstring& targetFilePath) { // Function taking the target path and returning an array of locks
    // Create an empty dynamic array (vector) to store the locks we find
    std::vector<LockInfo> locks; // This vector is our final return value
    
    // Safety check: ensure our hidden API pointers were loaded correctly in the constructor
    if (!ntQuerySystemInformationPtr || !ntQueryObjectPtr) { // If either pointer is null
        return locks; // Return the empty list immediately because we can't perform the scan without these APIs
    } // End of safety check

    // Cast our raw void pointer into a callable function pointer of the correct type
    PNtQuerySystemInformation querySysInfo = (PNtQuerySystemInformation)ntQuerySystemInformationPtr; // Type casting
    // Cast the other raw pointer as well
    PNtQueryObject queryObject = (PNtQueryObject)ntQueryObjectPtr; // Type casting

    // Get the native path of the target file because internal OS handles use native paths
    std::wstring nativeTargetPath = getNativeFilePath(targetFilePath); // Call our helper function
    // If the file doesn't exist or we couldn't get its path
    if (nativeTargetPath.empty()) { // Check for empty string
        return locks; // Exit early, there is nothing to scan for
    } // End of path check

    // Convert the target path to lowercase for case-insensitive comparison later
    std::transform(nativeTargetPath.begin(), nativeTargetPath.end(), nativeTargetPath.begin(), ::towlower); // std::transform is an algorithm like a map() function in DSA

    // Start with an initial buffer size of 1 Megabyte for the OS handle table
    ULONG bufferSize = 1024 * 1024; // 1 MB is usually enough, but we might need more memory
    // Allocate raw memory for the buffer
    std::vector<unsigned char> buffer(bufferSize); // Vector of bytes acts as our contiguous memory block
    
    // Variable to hold the actual required size if 1MB is not enough
    ULONG returnLength = 0; // Initialize to zero
    // Variable to hold the status code returned by the OS
    NTSTATUS status; // NTSTATUS variable stores the result code

    // Loop until we provide a large enough buffer to the OS
    do { // Start of do-while loop
        // Call the hidden OS function to get all extended handle information
        status = querySysInfo(SystemExtendedHandleInformation, buffer.data(), bufferSize, &returnLength); // Pass our memory buffer to the kernel
        
        // If the OS says our buffer is too small (STATUS_INFO_LENGTH_MISMATCH)
        if (status == 0xC0000004) { // This specific hex code means "buffer too small"
            bufferSize = returnLength + (1024 * 1024); // Increase the buffer size by the required amount plus 1MB padding just in case handles are rapidly opening
            buffer.resize(bufferSize); // Resize the dynamic array to the new larger size
        } // End of buffer size check
    } while (status == 0xC0000004); // Keep trying until we have enough memory and the OS stops complaining

    // If the query failed for some other reason
    if (!NT_SUCCESS(status)) { // Check our success macro defined in NativeAPI.h
        return locks; // Return empty list on failure
    } // End of success check

    // Cast our raw byte buffer into the structured array format defined in our header
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)buffer.data(); // Pointer cast to struct header

    // Iterate through every single handle currently open in the entire OS (O(N) traversal)
    for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; ++i) { // Loop from 0 to total handles
        // Get a reference to the current handle entry
        SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = handleInfo->Handles[i]; // Array access syntax
        
        // Skip handles with specific access rights that could cause the system to hang if queried
        if (entry.GrantedAccess == 0x0012019f || entry.GrantedAccess == 0x001A019F || entry.GrantedAccess == 0x120189) { // These magic access masks often mean the object is synchronous or blocking pipe
            continue; // Skip this iteration and go to the next handle to avoid freezing our own program
        } // End of blocking handle check

        // We need to duplicate the handle into our own process to query its name safely
        // First, open a handle to the process that owns this handle
        HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)entry.UniqueProcessId); // Request duplication and query rights from the OS
        
        // If we couldn't open the process (maybe it's a highly protected system process)
        if (!hProcess) { // Check for null
            continue; // Skip to next handle as we cannot investigate this one
        } // End of process handle check

        // Variable to hold our duplicate of the handle
        HANDLE hDup = nullptr; // Initialize to null pointer
        
        // Ask the OS to copy the handle from the target process into our process
        BOOL dupSuccess = DuplicateHandle( // Call the Windows duplication API
            hProcess, // The process that currently owns the handle
            (HANDLE)entry.HandleValue, // The value of the handle in that process
            GetCurrentProcess(), // Our own process (the destination)
            &hDup, // The memory location where the OS will store the copied handle
            0, // Request same access rights as the original
            FALSE, // Do not make it inheritable
            DUPLICATE_SAME_ACCESS // Flag indicating we want identical permissions
        ); // End of DuplicateHandle call

        // If duplication succeeded
        if (dupSuccess && hDup != nullptr) { // Check boolean flag and pointer validity
            // Prepare a buffer to receive the name of the object this handle points to
            ULONG nameBufferSize = 1024; // Start with 1KB for the name string
            std::vector<unsigned char> nameBuffer(nameBufferSize); // Allocate dynamic array
            
            // Ask the OS for the object's name (like the file path)
            status = queryObject(hDup, 1, nameBuffer.data(), nameBufferSize, &returnLength); // 1 means ObjectNameInformation
            
            // If the buffer was too small for the path string
            if (status == 0xC0000004) { // Buffer too small code
                nameBuffer.resize(returnLength); // Resize vector to exact required size
                // Ask again with the larger, correct buffer size
                status = queryObject(hDup, 1, nameBuffer.data(), returnLength, &returnLength); // Retry query
            } // End of size check

            // If we successfully got the name
            if (NT_SUCCESS(status)) { // Check success macro
                // Cast the buffer to the name structure
                PPUBLIC_OBJECT_NAME_INFORMATION nameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)nameBuffer.data(); // Pointer cast
                
                // If the name length is greater than zero and buffer is not null
                if (nameInfo->Name.Length > 0 && nameInfo->Name.Buffer != nullptr) { // Ensure string is valid memory
                    // Extract the wide string from the Unicode structure
                    std::wstring handleName(nameInfo->Name.Buffer, nameInfo->Name.Length / sizeof(wchar_t)); // Construct C++ string from kernel bytes
                    
                    // Convert the handle's name to lowercase for comparison
                    std::wstring lowerHandleName = handleName; // Copy the string
                    std::transform(lowerHandleName.begin(), lowerHandleName.end(), lowerHandleName.begin(), ::towlower); // Lowercase algorithm
                    
                    // Check if this handle's name exactly matches our target file's native path
                    if (lowerHandleName == nativeTargetPath) { // String equality check (like comparing strings in DSA)
                        // We found a match! Record the information
                        LockInfo info; // Create a struct instance
                        info.processId = (DWORD)entry.UniqueProcessId; // Store the PID
                        info.handleValue = (HANDLE)entry.HandleValue; // Store the handle value
                        
                        // Try to get the executable name of the process (optional, for logging)
                        info.processName = L"Unknown"; // Default to unknown as getting process name requires more API calls
                        
                        // Add the found lock info to our results array
                        locks.push_back(info); // Append to vector (O(1) amortized insertion)
                    } // End of path match check
                } // End of valid string check
            } // End of queryObject success check
            // Close our duplicate handle so we don't leak kernel resources
            CloseHandle(hDup); // Always clean up OS handles when finished
        } // End of duplication success check
        // Close our handle to the target process
        CloseHandle(hProcess); // Release process handle
    } // End of O(N) handle iteration loop
    
    // Return the array containing all the locks we found
    return locks; // Return by value (modern C++ uses move semantics to avoid copying the whole array)
} // End of findLocksForFile
