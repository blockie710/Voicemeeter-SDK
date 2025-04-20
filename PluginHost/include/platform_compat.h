/**
 * platform_compat.h
 * 
 * Platform compatibility layer for cross-platform development
 * Handles Windows/Linux/macOS specific differences
 */

#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_NAME "Windows"
#elif defined(__linux__) || defined(__linux)
    #define PLATFORM_LINUX 1
    #define PLATFORM_NAME "Linux"
#elif defined(__APPLE__)
    #define PLATFORM_MACOS 1
    #define PLATFORM_NAME "macOS"
#else
    #error "Unsupported platform"
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X64 1
    #define ARCH_NAME "x64"
#elif defined(__i386) || defined(_M_IX86)
    #define ARCH_X86 1
    #define ARCH_NAME "x86"
#elif defined(__arm__) || defined(_M_ARM)
    #define ARCH_ARM 1
    #define ARCH_NAME "ARM"
#elif defined(__aarch64__)
    #define ARCH_ARM64 1
    #define ARCH_NAME "ARM64"
#else
    #define ARCH_UNKNOWN 1
    #define ARCH_NAME "Unknown"
#endif

// Windows-specific compatibility
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    
    // Define __stdcall for function pointers if not on Windows
    #ifndef __stdcall
        #define __stdcall __stdcall
    #endif
    
    // Windows DLL export/import
    #ifdef BUILDING_DLL
        #define DLL_EXPORT __declspec(dllexport)
    #else
        #define DLL_EXPORT __declspec(dllimport)
    #endif
    
    // Windows-specific path separator
    #define PATH_SEPARATOR '\\'
    #define PATH_SEPARATOR_STR "\\"
    
#else // macOS & Linux
    #include <dlfcn.h>
    
    // Define these Windows types for Unix platforms
    #ifndef __stdcall
        #define __stdcall
    #endif
    
    #define DLL_EXPORT
    
    // Unix-specific path separator 
    #define PATH_SEPARATOR '/'
    #define PATH_SEPARATOR_STR "/"
    
    // Windows types on Unix platforms
    #ifndef DWORD
        typedef unsigned long DWORD;
    #endif
    
    #ifndef LONG
        typedef long LONG;
    #endif
    
    #ifndef BOOL
        typedef int BOOL;
    #endif
    
    #ifndef UINT
        typedef unsigned int UINT;
    #endif
    
    #ifndef TRUE
        #define TRUE 1
    #endif
    
    #ifndef FALSE
        #define FALSE 0
    #endif
#endif

// Helper functions for cross-platform dynamic library loading
#ifdef PLATFORM_WINDOWS
    inline void* LoadDynamicLibrary(const char* path) {
        return LoadLibraryA(path);
    }
    
    inline void* GetFunctionAddress(void* handle, const char* name) {
        return GetProcAddress((HMODULE)handle, name);
    }
    
    inline void UnloadDynamicLibrary(void* handle) {
        FreeLibrary((HMODULE)handle);
    }
#else
    inline void* LoadDynamicLibrary(const char* path) {
        return dlopen(path, RTLD_NOW);
    }
    
    inline void* GetFunctionAddress(void* handle, const char* name) {
        return dlsym(handle, name);
    }
    
    inline void UnloadDynamicLibrary(void* handle) {
        dlclose(handle);
    }
#endif

#endif // PLATFORM_COMPAT_H