// Author: Dev Jhawar
// Institution: KIIT University

#include "SystemHandleScanner.h"
#include "NativeAPI.h"
#include <iostream>
#include <algorithm>

// Constructor: Initializes the scanner object and links to the hidden Windows APIs
SystemHandleScanner::SystemHandleScanner() {
    // Load the 'ntdll.dll' library into our process memory, which contains the low-level kernel APIs
    ntdllHandle = LoadLibraryA("ntdll.dll");
    
    // Check if the library loaded successfully
    if (ntdllHandle) {
        // Find the memory address of the NtQuerySystemInformation function inside the loaded DLL
        ntQuerySystemInformationPtr = (void*)GetProcAddress(ntdllHandle, "NtQuerySystemInformation");
        
        // Find the memory address of the NtQueryObject function
        ntQueryObjectPtr = (void*)GetProcAddress(ntdllHandle, "NtQueryObject");
    } else {
        ntQuerySystemInformationPtr = nullptr;
        ntQueryObjectPtr = nullptr;
    }
}

// Destructor: Cleans up memory and resources when the scanner object is destroyed
SystemHandleScanner::~SystemHandleScanner() {
    // Check if we previously loaded the ntdll library
    if (ntdllHandle) {
        FreeLibrary(ntdllHandle);
    }
}

// Helper: Converts a regular file path (C:\folder\file.txt) to a native OS path (\Device\HarddiskVolume...\file.txt)
std::wstring SystemHandleScanner::getNativeFilePath(const std::wstring& dosPath) {
    // Create an empty string to hold the output native path
    std::wstring nativePath = L"";
    
    // Open a handle to the file itself just to ask the OS for its native name
    HANDLE hFile = CreateFileW(
        dosPath.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    // If we successfully got a handle to the file
    if (hFile != INVALID_HANDLE_VALUE) {
        // Prepare a buffer to store the native file path
        std::vector<wchar_t> buffer(MAX_PATH * 4);
        
        // Ask the OS for the final, resolved native path of this file handle
        DWORD result = GetFinalPathNameByHandleW(hFile, buffer.data(), buffer.size(), VOLUME_NAME_NT);
        
        // If the function succeeded, it returns the length of the string
        if (result > 0 && result < buffer.size()) {
            nativePath = std::wstring(buffer.data(), result);
        }
        // Always close handles when done to prevent resource leaks
        CloseHandle(hFile);
    }
    
    // Return the converted path, or an empty string if it failed
    return nativePath;
}

// Main algorithm: Scans all OS handles to find which process is locking our target file
std::vector<LockInfo> SystemHandleScanner::findLocksForFile(const std::wstring& targetFilePath) {
    // Create an empty dynamic array (vector) to store the locks we find
    std::vector<LockInfo> locks;
    
    // Safety check: ensure our hidden API pointers were loaded correctly in the constructor
    if (!ntQuerySystemInformationPtr || !ntQueryObjectPtr) {
        return locks;
    }

    // Cast our raw void pointer into a callable function pointer of the correct type
    PNtQuerySystemInformation querySysInfo = (PNtQuerySystemInformation)ntQuerySystemInformationPtr;
    // Cast the other raw pointer as well
    PNtQueryObject queryObject = (PNtQueryObject)ntQueryObjectPtr;

    // Get the native path of the target file because internal OS handles use native paths
    std::wstring nativeTargetPath = getNativeFilePath(targetFilePath);
    // If the file doesn't exist or we couldn't get its path
    if (nativeTargetPath.empty()) {
        return locks;
    }

    // Convert the target path to lowercase for case-insensitive comparison later
    std::transform(nativeTargetPath.begin(), nativeTargetPath.end(), nativeTargetPath.begin(), ::towlower);

    // Start with an initial buffer size of 1 Megabyte for the OS handle table
    ULONG bufferSize = 1024 * 1024;
    // Allocate raw memory for the buffer
    std::vector<unsigned char> buffer(bufferSize);
    
    // Variable to hold the actual required size if 1MB is not enough
    ULONG returnLength = 0;
    // Variable to hold the status code returned by the OS
    NTSTATUS status;

    // Loop until we provide a large enough buffer to the OS
    do {
        // Call the hidden OS function to get all extended handle information
        status = querySysInfo(SystemExtendedHandleInformation, buffer.data(), bufferSize, &returnLength);
        
        // If the OS says our buffer is too small (STATUS_INFO_LENGTH_MISMATCH)
        if (status == 0xC0000004) {
            bufferSize = returnLength + (1024 * 1024);
            buffer.resize(bufferSize);
        }
    } while (status == 0xC0000004);

    // If the query failed for some other reason
    if (!NT_SUCCESS(status)) {
        return locks;
    }

    // Cast our raw byte buffer into the structured array format defined in our header
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)buffer.data();

    // Iterate through every single handle currently open in the entire OS (O(N) traversal)
    for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; ++i) {
        // Get a reference to the current handle entry
        SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = handleInfo->Handles[i];
        
        // Skip System process (PID 4) handles as querying them often causes the OS to hang indefinitely
        if (entry.UniqueProcessId == 4) {
            continue;
        }

        // Skip handles with specific access rights that could cause the system to hang if queried
        if (entry.GrantedAccess == 0x0012019f || entry.GrantedAccess == 0x001A019F || entry.GrantedAccess == 0x120189) {
            continue;
        }

        // We need to duplicate the handle into our own process to query its name safely
        // First, open a handle to the process that owns this handle
        HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)entry.UniqueProcessId);
        
        // If we couldn't open the process (maybe it's a highly protected system process)
        if (!hProcess) {
            continue;
        }

        // Variable to hold our duplicate of the handle
        HANDLE hDup = nullptr;
        
        // Ask the OS to copy the handle from the target process into our process
        BOOL dupSuccess = DuplicateHandle(
            hProcess,
            (HANDLE)entry.HandleValue,
            GetCurrentProcess(),
            &hDup,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
        );

        // If duplication succeeded
        if (dupSuccess && hDup != nullptr) {
            // PREVENT HANGS: Query the object TYPE first (Class 2). This never hangs.
            ULONG typeBufferSize = 1024;
            std::vector<unsigned char> typeBuffer(typeBufferSize);
            status = queryObject(hDup, 2, typeBuffer.data(), typeBufferSize, &returnLength);
            
            if (status == 0xC0000004) {
                typeBuffer.resize(returnLength);
                status = queryObject(hDup, 2, typeBuffer.data(), returnLength, &returnLength);
            }
            
            bool isFile = false;
            if (NT_SUCCESS(status)) {
                PPUBLIC_OBJECT_TYPE_INFORMATION typeInfo = (PPUBLIC_OBJECT_TYPE_INFORMATION)typeBuffer.data();
                if (typeInfo->TypeName.Length > 0 && typeInfo->TypeName.Buffer != nullptr) {
                    std::wstring typeName(typeInfo->TypeName.Buffer, typeInfo->TypeName.Length / sizeof(wchar_t));
                    if (typeName == L"File") {
                        isFile = true; // Folders are also considered "File" at the kernel level
                    }
                }
            }
            
            // If it is NOT a File, skip querying its name because querying named pipes/ports will HANG the thread forever!
            if (!isFile) {
                CloseHandle(hDup);
                CloseHandle(hProcess);
                continue;
            }

            // Prepare a buffer to receive the name of the object this handle points to
            ULONG nameBufferSize = 1024;
            std::vector<unsigned char> nameBuffer(nameBufferSize);
            
            // Ask the OS for the object's name (like the file path)
            status = queryObject(hDup, 1, nameBuffer.data(), nameBufferSize, &returnLength);
            
            // If the buffer was too small for the path string
            if (status == 0xC0000004) {
                nameBuffer.resize(returnLength);
                // Ask again with the larger, correct buffer size
                status = queryObject(hDup, 1, nameBuffer.data(), returnLength, &returnLength);
            }

            // If we successfully got the name
            if (NT_SUCCESS(status)) {
                // Cast the buffer to the name structure
                PPUBLIC_OBJECT_NAME_INFORMATION nameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)nameBuffer.data();
                
                // If the name length is greater than zero and buffer is not null
                if (nameInfo->Name.Length > 0 && nameInfo->Name.Buffer != nullptr) {
                    // Extract the wide string from the Unicode structure
                    std::wstring handleName(nameInfo->Name.Buffer, nameInfo->Name.Length / sizeof(wchar_t));
                    
                    // Convert the handle's name to lowercase for comparison
                    std::wstring lowerHandleName = handleName;
                    std::transform(lowerHandleName.begin(), lowerHandleName.end(), lowerHandleName.begin(), ::towlower);
                    
                    // Check if this handle's name exactly matches our target file's native path
                    if (lowerHandleName == nativeTargetPath) {
                        // We found a match! Record the information
                        LockInfo info;
                        info.processId = (DWORD)entry.UniqueProcessId;
                        info.handleValue = (HANDLE)entry.HandleValue;
                        
                        // Try to get the executable name of the process (optional, for logging)
                        info.processName = L"Unknown";
                        
                        // Add the found lock info to our results array
                        locks.push_back(info);
                    }
                }
            }
            // Close our duplicate handle so we don't leak kernel resources
            CloseHandle(hDup);
        }
        // Close our handle to the target process
        CloseHandle(hProcess);
    }
    
    // Return the array containing all the locks we found
    return locks;
}

