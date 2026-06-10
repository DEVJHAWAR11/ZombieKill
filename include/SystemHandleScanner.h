// Author: Dev Jhawar
// Institution: KIIT University

#pragma once

#include <string>
#include <vector>
#include <windows.h>

// A structure to hold information about a file lock we discover
struct LockInfo {
    DWORD processId;
    std::wstring processName;
    HANDLE handleValue;
};

// The class responsible for scanning the entire OS to find open file handles
class SystemHandleScanner {
public:
    SystemHandleScanner();
    ~SystemHandleScanner();

    // The main algorithm function: finds all processes holding a lock on a specific file path
    std::vector<LockInfo> findLocksForFile(const std::wstring& targetFilePath);

private:
    HMODULE ntdllHandle;
    void* ntQuerySystemInformationPtr;
    void* ntQueryObjectPtr;

    // Helper function to resolve the target file path to a Windows Native path format
    std::wstring getNativeFilePath(const std::wstring& dosPath);
};

