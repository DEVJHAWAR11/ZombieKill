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

// ---- SAFE QUERY WRAPPER ----
// To prevent the entire application from hanging when NtQueryObject gets stuck on a Named Pipe,
// we spawn a worker thread. If it doesn't return in 20 milliseconds, we forcefully terminate it.
struct QueryData {
    HANDLE h;
    unsigned char* buf;
    ULONG bufSize;
    ULONG retLen;
    NTSTATUS status;
    PNtQueryObject fn;
};

DWORD WINAPI QueryThreadProc(LPVOID param) {
    QueryData* data = (QueryData*)param;
    data->status = data->fn(data->h, 1, data->buf, data->bufSize, &data->retLen);
    return 0;
}

NTSTATUS CallNtQueryObjectWithTimeout(PNtQueryObject queryObj, HANDLE hDup, std::vector<unsigned char>& buffer, ULONG& returnLength) {
    QueryData qd;
    qd.h = hDup;
    qd.buf = buffer.data();
    qd.bufSize = buffer.size();
    qd.retLen = 0;
    qd.status = -1;
    qd.fn = queryObj;

    HANDLE hThread = CreateThread(NULL, 0, QueryThreadProc, &qd, 0, NULL);
    if (hThread) {
        // Wait up to 20 milliseconds
        if (WaitForSingleObject(hThread, 20) == WAIT_TIMEOUT) {
            TerminateThread(hThread, 0); // Force kill the hanging thread!
            CloseHandle(hThread);
            return 0xC0000034; // Return a dummy error code so we skip this handle
        }
        CloseHandle(hThread);
        returnLength = qd.retLen;
        return qd.status;
    }
    return -1;
}
// ---- END SAFE WRAPPER ----

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

    // ---- DYNAMIC FILE TYPE DISCOVERY (Step 1: Create a dummy file BEFORE taking the OS snapshot) ----
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    wchar_t tempFile[MAX_PATH];
    GetTempFileNameW(tempPath, L"ZMB", 0, tempFile);
    HANDLE hDummy = CreateFileW(tempFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);

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

    // ---- DYNAMIC FILE TYPE DISCOVERY (Step 2: Find our dummy file in the OS snapshot) ----
    
    USHORT fileTypeIndex = 0;
    DWORD myPid = GetCurrentProcessId();

    for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; ++i) {
        SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = handleInfo->Handles[i];
        if (entry.UniqueProcessId == myPid && (HANDLE)entry.HandleValue == hDummy) {
            fileTypeIndex = entry.ObjectTypeIndex;
            break;
        }
    }
    
    if (hDummy != INVALID_HANDLE_VALUE) {
        CloseHandle(hDummy); // This will also delete the temp file due to FILE_FLAG_DELETE_ON_CLOSE
    }

    if (fileTypeIndex == 0) {
        std::wcout << L"Error: Could not dynamically determine Windows File Object Type." << std::endl;
        return locks;
    }
    // ---- END DISCOVERY ----

    // Cache process handles to drastically speed up the scan (avoids calling OpenProcess 100,000 times)
    HANDLE hCurrentProcess = NULL;
    ULONG_PTR currentPid = 0xFFFFFFFF; // Start with an invalid PID

    // Iterate through every single handle currently open in the entire OS (O(N) traversal)
    for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; ++i) {
        
        // Print a progress indicator every 10,000 handles so the user knows it's not frozen!
        if (i > 0 && i % 10000 == 0) {
            std::wcout << L"Scanned " << i << L" out of " << handleInfo->NumberOfHandles << L" handles..." << std::endl;
        }

        // Get a reference to the current handle entry
        SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = handleInfo->Handles[i];
        
        // CRITICAL: Only process File handles! This bypasses 80% of handles and entirely prevents ALPC/Pipe hangs!
        if (entry.ObjectTypeIndex != fileTypeIndex) {
            continue;
        }
        
        // Skip System process (PID 4) handles as querying them often causes the OS to hang indefinitely
        if (entry.UniqueProcessId == 4) {
            continue;
        }

        // Skip handles with specific access rights that could cause the system to hang if queried
        if (entry.GrantedAccess == 0x0012019f || entry.GrantedAccess == 0x001A019F || entry.GrantedAccess == 0x120189) {
            continue;
        }

        // We need to duplicate the handle into our own process to query its name safely
        // If the PID changed, we need to open a new process handle
        if (entry.UniqueProcessId != currentPid) {
            if (hCurrentProcess != NULL) {
                CloseHandle(hCurrentProcess); // Close the old one
            }
            currentPid = entry.UniqueProcessId;
            hCurrentProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)currentPid);
        }
        
        // If we couldn't open the process (maybe it's a highly protected system process)
        if (!hCurrentProcess) {
            continue;
        }

        // Variable to hold our duplicate of the handle
        HANDLE hDup = nullptr;
        
        // Ask the OS to copy the handle from the target process into our process
        BOOL dupSuccess = DuplicateHandle(
            hCurrentProcess,
            (HANDLE)entry.HandleValue,
            GetCurrentProcess(),
            &hDup,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
        );

        // If duplication succeeded
        if (dupSuccess && hDup != nullptr) {
            // Prepare a buffer to receive the name of the object this handle points to
            ULONG nameBufferSize = 1024;
            std::vector<unsigned char> nameBuffer(nameBufferSize);
            
            // Call our safe wrapper that uses a thread and timeout to PREVENT HANGING on pipes!
            status = CallNtQueryObjectWithTimeout(queryObject, hDup, nameBuffer, returnLength);
            
            // If the buffer was too small for the path string
            if (status == 0xC0000004) {
                nameBuffer.resize(returnLength);
                // Ask again with the larger, correct buffer size
                status = CallNtQueryObjectWithTimeout(queryObject, hDup, nameBuffer, returnLength);
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
    }
    
    // Close the very last process handle if we opened one
    if (hCurrentProcess != NULL) {
        CloseHandle(hCurrentProcess);
    }
    
    // Return the array containing all the locks we found
    return locks;
}

