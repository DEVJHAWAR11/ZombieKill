// Author: Dev Jhawar
// Institution: KIIT University

#pragma once

#include <windows.h>
#include <winternl.h>

// Define the NTSTATUS type if not already defined, as it's the return type for Native APIs
#ifndef NTSTATUS
#define NTSTATUS LONG
#endif

// Define the success macro for NTSTATUS, because Native APIs use >= 0 for success
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Define the system information class for extended handle information (0x40 = 64)
#define SystemExtendedHandleInformation 64

// Structure representing a single handle entry in the system
typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX {
    PVOID Object;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR HandleValue;
    ULONG GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

// Structure representing the entire array of handles returned by the OS
typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

// Structure used to get the name of an object (like the file path)
typedef struct _PUBLIC_OBJECT_NAME_INFORMATION {
    UNICODE_STRING Name;
} PUBLIC_OBJECT_NAME_INFORMATION, *PPUBLIC_OBJECT_NAME_INFORMATION;

// PUBLIC_OBJECT_TYPE_INFORMATION is already defined in <winternl.h>

// Define the function pointer type for NtQuerySystemInformation, a hidden OS function
typedef NTSTATUS (NTAPI *PNtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

// Define the function pointer type for NtQueryObject, another hidden OS function
typedef NTSTATUS (NTAPI *PNtQueryObject)(
    HANDLE Handle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
);

