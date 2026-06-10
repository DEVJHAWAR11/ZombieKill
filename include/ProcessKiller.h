// Author: Dev Jhawar
// Institution: KIIT University

#pragma once

#include "SystemHandleScanner.h"
#include <string>
#include <vector>

// The class responsible for neutralizing the processes or handles holding our file hostage
class ProcessKiller {
public:
    // Tries to forcefully terminate the processes holding the file
    static bool killProcesses(const std::vector<LockInfo>& locks);

    // Deletes the file once it is unlocked
    static bool deleteUnlockedFile(const std::wstring& filePath);
};

