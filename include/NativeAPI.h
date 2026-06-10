// Author: Dev Jhawar
// Institution: KIIT University

#pragma once // Prevent multiple inclusions of this header file, which causes compilation errors

#include <windows.h> // Include standard Windows API functions needed for OS-level interactions
#include <winternl.h> // Include Windows NT Internal functions for low-level system queries

// Define the NTSTATUS type if not already defined, as it's the return type for Native APIs
#ifndef NTSTATUS // Check if NTSTATUS is not defined yet
#define NTSTATUS LONG // NTSTATUS is a 32-bit integer used by Windows to report success or error codes
#endif // End of the if-not-defined check

// Define the success macro for NTSTATUS, because Native APIs use >= 0 for success
#ifndef NT_SUCCESS // Check if the success macro is not defined yet
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0) // Macro that evaluates to true if the status code is greater than or equal to 0
#endif // End of the success macro check

// Define the system information class for extended handle information (0x40 = 64)
#define SystemExtendedHandleInformation 64 // This specific integer tells the OS we want details about ALL handles in the system

// Structure representing a single handle entry in the system
typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX { // Define a struct to hold details about one specific handle
    PVOID Object; // Pointer to the raw kernel object in memory (like a memory address in a pointer variable)
    ULONG_PTR UniqueProcessId; // The Process ID (PID) of the program that owns this handle (like a unique ID key)
    ULONG_PTR HandleValue; // The actual handle value (like a file descriptor or ticket number)
    ULONG GrantedAccess; // The permissions the process has over this handle (Read, Write, Delete, etc.)
    USHORT CreatorBackTraceIndex; // Internal OS tracking index (we ignore this field)
    USHORT ObjectTypeIndex; // A number telling us what type of object this is (File, Process, Thread, etc.)
    ULONG HandleAttributes; // Attributes of the handle (e.g., if it can be inherited by child processes)
    ULONG Reserved; // Reserved space for future OS use (we don't touch this)
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX; // Define the struct name and a pointer type to it

// Structure representing the entire array of handles returned by the OS
typedef struct _SYSTEM_HANDLE_INFORMATION_EX { // Define a struct to act as the header for our handle table
    ULONG_PTR NumberOfHandles; // The total number of handles currently open in the entire OS (like the size property of a dynamic array)
    ULONG_PTR Reserved; // Reserved space (ignored by us)
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1]; // The beginning of the variable-length array of handle entries (acting as an array head)
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX; // Define the struct name and a pointer type to it

// Structure used to get the name of an object (like the file path)
typedef struct _PUBLIC_OBJECT_NAME_INFORMATION { // Define a struct to hold string data returned by the kernel
    UNICODE_STRING Name; // A Unicode string structure holding the actual name/path of the object
} PUBLIC_OBJECT_NAME_INFORMATION, *PPUBLIC_OBJECT_NAME_INFORMATION; // Define the struct name and a pointer type to it

// Define the function pointer type for NtQuerySystemInformation, a hidden OS function
typedef NTSTATUS (NTAPI *PNtQuerySystemInformation)( // Define a function pointer signature (like a callback function type)
    ULONG SystemInformationClass, // Parameter: What kind of information we want (we will pass 64 here)
    PVOID SystemInformation, // Parameter: The memory buffer where the OS will write the result (our allocated array)
    ULONG SystemInformationLength, // Parameter: The size of our buffer in bytes so the OS doesn't overflow it
    PULONG ReturnLength // Parameter: The OS will write the required buffer size here if our buffer is too small
); // End of function pointer definition

// Define the function pointer type for NtQueryObject, another hidden OS function
typedef NTSTATUS (NTAPI *PNtQueryObject)( // Define another function pointer signature for querying object names
    HANDLE Handle, // Parameter: The handle we want to get information about
    ULONG ObjectInformationClass, // Parameter: What kind of info we want (we pass 1 for ObjectNameInformation)
    PVOID ObjectInformation, // Parameter: The buffer where the OS will write the object's name string
    ULONG ObjectInformationLength, // Parameter: The size of our buffer
    PULONG ReturnLength // Parameter: The OS writes the required size here if our buffer was too small
); // End of function pointer definition
