# The Unstuck Forced File Unlocker

![Folder In Use Error](download.png)

## Understanding the Problem
As Windows users and developers, we constantly encounter the dreaded "File In Use" or "Folder In Use" error. You try to delete a file, but the Operating System blocks you, stating that another program is using it. But Windows rarely tells you *which* program is the culprit. Sometimes, a background process (a "Zombie" process) crashes or hangs while keeping a file handle open, holding your file hostage indefinitely.

## The Solution
"The Unstuck Forced File Unlocker" is a standalone C++ command-line utility designed to break these zombie file locks. By dragging and dropping a locked file onto this executable, the utility acts as a kernel-level investigator and executioner.

### How it Works
1. **Low-Level Native APIs:** Standard Windows APIs do not provide a way to find which process holds a specific file. This utility uses undocumented, low-level Windows NT Internal APIs (`NtQuerySystemInformation` and `NtQueryObject`) bypassing standard OS restrictions.
2. **Global System Handle Scanning:** It queries the OS kernel for the `SystemExtendedHandleInformation` structure, receiving an array of every single open handle across the entire operating system.
3. **Handle Duplication & Resolution:** It iterates through these thousands of handles (O(N) traversal). For each handle, it safely duplicates it using `DuplicateHandle` and queries its actual native file path in the filesystem.
4. **Target Matching:** If the handle's native path matches your locked file, the utility captures the rogue Process ID (PID).
5. **Termination & Deletion:** Using `TerminateProcess`, it forcefully kills the zombie process holding the lock and instantly deletes the freed file.

## Build Instructions
This project uses CMake and requires a standard C++ compiler (like MSVC from Visual Studio or MinGW).

1. Open a terminal in the project directory.
2. Generate build files:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
3. Compile the project:
   ```bash
   cmake --build . --config Release
   ```

## Execution & Safe Testing
**Note: You must run this utility with Administrator privileges to scan system-wide handles.**

1. Open an Administrator Command Prompt or PowerShell.
2. Run the utility by passing the locked file as an argument:
   ```bash
   unstuck_utility.exe "C:\path\to\your\locked_file.txt"
   ```
   *Alternatively, in a GUI environment, you can simply drag and drop the locked file directly onto the `unstuck_utility.exe` icon.*

**To Test Safely:**
Create a dummy text file. Write a simple Python or C++ script that opens the file in an infinite loop without closing it. Try to delete the file normally (it will fail). Then, use this utility to unlock and delete it. Never test on critical system files (like `C:\Windows\System32` contents).
