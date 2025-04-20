#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

// Platform compatibility layer for Linux builds

#ifdef _WIN32
    #include <windows.h>
    #define __stdcall __stdcall
#else
    // Define Windows types for Linux
    typedef void* LPVOID;
    typedef void* HMODULE;
    typedef long LONG;
    typedef unsigned long DWORD;
    #define __stdcall 
    #define WINAPI
    
    // No-op for Windows-specific declarations
    #define LoadLibraryA(x) nullptr
    #define GetProcAddress(x, y) nullptr
    #define FreeLibrary(x) (0)
#endif

#endif // PLATFORM_COMPAT_H