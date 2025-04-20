cmake_minimum_required(VERSION 3.14)
project(Voicemeeter-SDK VERSION 1.0.0)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# Options for components to build
option(BUILD_PLUGIN_HOST "Build the Plugin Host component" ON)
option(BUILD_EXAMPLE0 "Build Example0 client" ON)
option(BUILD_MATRIX8X8 "Build Matrix 8x8 component" ON)
option(BUILD_VMR_OSD "Build VMR OSD component" ON)
option(BUILD_VMR_PLAY "Build VMR Play component" ON)
option(BUILD_VMR_STREAMER "Build VMR Streamer component" ON)

# Make sure to use C11 and C++23 standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific settings
if(WIN32)
    add_compile_definitions(_WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(APPLE)
    add_compile_definitions(__APPLE__)
else()
    # Linux-specific settings
    add_compile_definitions(__linux__)
endif()

# Add the individual components as subdirectories
if(BUILD_PLUGIN_HOST)
    add_subdirectory(PluginHost)
endif()

if(BUILD_EXAMPLE0)
    add_subdirectory(example0)
endif()

if(BUILD_MATRIX8X8)
    add_subdirectory(matrix8x8)
endif()

if(BUILD_VMR_OSD)
    add_subdirectory(vmr_osd)
endif()

if(BUILD_VMR_PLAY)
    add_subdirectory(vmr_play)
endif()

if(BUILD_VMR_STREAMER)
    add_subdirectory(vmr_streamer)
endif()

# Installation rules
install(FILES 
    README.md
    LICENSE
    VoicemeeterRemote.h
    VoicemeeterRemoteAPI.pdf
    DESTINATION share/voicemeeter-sdk)

# CPack configuration for creating distribution packages
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_VENDOR "VB-Audio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Voicemeeter SDK - Development tools for audio processing and plugins")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "your-email@example.com")

# Include CPack to enable packaging commands
include(CPack)