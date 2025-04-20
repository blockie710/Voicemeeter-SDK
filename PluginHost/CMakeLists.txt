cmake_minimum_required(VERSION 3.10)

project(VoicemeeterPluginHost VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++17 (more compatible with MinGW)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/plugin_interface.cpp
    src/voicemeeter_integration.cpp
    src/vst3_plugin.cpp
    src/aax_plugin.cpp
    src/VMPluginHost.cpp
)

# Platform-specific sources and libraries
if(WIN32)
    list(APPEND SOURCES
        src/win32_platform.cpp
    )
    set(PLATFORM_LIBS
        winmm
        comctl32
        user32
        gdi32
        shell32
        ole32
        uuid
    )
elseif(APPLE)
    list(APPEND SOURCES
        src/macos_platform.cpp
        src/aau_plugin.cpp
    )
    set(PLATFORM_LIBS
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework Cocoa"
    )
else() # Linux
    list(APPEND SOURCES
        src/linux_platform.cpp
    )
    set(PLATFORM_LIBS
        dl
        pthread
        rt
    )
endif()

# Define the executable
add_executable(VoicemeeterPluginHost ${SOURCES})

# Link libraries
target_link_libraries(VoicemeeterPluginHost PRIVATE ${PLATFORM_LIBS})

# Windows-specific settings
if(WIN32)
    # Set Windows subsystem
    set_target_properties(VoicemeeterPluginHost PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
    
    # Add Windows resource files
    set(RC_FILE "installer/windows/resources.rc")
    target_sources(VoicemeeterPluginHost PRIVATE ${RC_FILE})
    
    # Add icon
    set_property(SOURCE ${RC_FILE} APPEND PROPERTY OBJECT_DEPENDS
        "installer/windows/icon.ico"
    )
endif()

# Install target
install(TARGETS VoicemeeterPluginHost
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Create platform-specific installer
include(CPack)
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "Voicemeeter Plugin Host")
    set(CPACK_PACKAGE_VENDOR "VB-Audio")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plugin Host for Voicemeeter")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/icon.ico")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/icon.ico")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/installer/macos/icon.icns")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "VB-Audio <support@vb-audio.com>")
endif()