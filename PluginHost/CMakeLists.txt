cmake_minimum_required(VERSION 3.14)
project(VoicemeeterPluginHost VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_compile_definitions(_WIN32)
    set(PLATFORM_LIBS "")
elseif(APPLE)
    # macOS-specific settings
    add_compile_definitions(__APPLE__)
    find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit REQUIRED)
    set(PLATFORM_LIBS ${COREAUDIO_FRAMEWORK} ${AUDIOUNIT_FRAMEWORK})
else()
    # Linux-specific settings (not fully supported yet)
    message(WARNING "Linux support is limited. Proceeding with minimal configuration.")
    set(PLATFORM_LIBS "")
endif()

# VST3 SDK path - this should be set by the user or detected automatically
set(VST3_SDK_PATH "" CACHE PATH "Path to the VST3 SDK")
if(VST3_SDK_PATH)
    add_compile_definitions(WITH_VST3_SUPPORT)
    include_directories(${VST3_SDK_PATH})
    message(STATUS "VST3 support enabled with SDK at ${VST3_SDK_PATH}")
else()
    message(STATUS "VST3 SDK not found. Limited VST3 support will be available.")
endif()

# AAX SDK path - this should be set by the user or detected automatically
set(AAX_SDK_PATH "" CACHE PATH "Path to the AAX SDK")
if(AAX_SDK_PATH)
    add_compile_definitions(WITH_AAX_SUPPORT)
    include_directories(${AAX_SDK_PATH})
    message(STATUS "AAX support enabled with SDK at ${AAX_SDK_PATH}")
else()
    message(STATUS "AAX SDK not found. Limited AAX support will be available.")
endif()

# Sources for the project
set(SOURCES
    src/main.cpp
    src/plugin_interface.cpp
    src/voicemeeter_integration.cpp
    src/vst3_plugin.cpp
    src/aax_plugin.cpp
    src/aau_plugin.cpp
)

# Headers for the project
set(HEADERS
    include/plugin_interface.h
    include/voicemeeter_integration.h
    include/vst3_plugin.h
    include/aax_plugin.h
    include/aau_plugin.h
    ../VoicemeeterRemote.h
)

# Create executable
add_executable(VoicemeeterPluginHost ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(VoicemeeterPluginHost PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link libraries
if(WIN32)
    target_link_libraries(VoicemeeterPluginHost PRIVATE ${PLATFORM_LIBS})
    
    # Add location of VoicemeeterRemote.dll
    target_compile_definitions(VoicemeeterPluginHost PRIVATE 
        -DVOICEMEETER_REMOTE_DLL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../VoicemeeterRemote64.dll"
    )
    
    # Set resource file for Windows
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.rc")
        target_sources(VoicemeeterPluginHost PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.rc")
    endif()
else()
    target_link_libraries(VoicemeeterPluginHost PRIVATE ${PLATFORM_LIBS})
    
    # For macOS, we need to link against the system frameworks
    if(APPLE)
        target_link_libraries(VoicemeeterPluginHost PRIVATE "-framework CoreAudio" "-framework AudioUnit")
    endif()
endif()

# Installation rules
install(TARGETS VoicemeeterPluginHost
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
)

# Copy required DLLs on Windows
if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../VoicemeeterRemote64.dll")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../VoicemeeterRemote64.dll" DESTINATION bin)
    endif()
endif()

# Create a directory for log files
install(DIRECTORY DESTINATION bin/logs)

# Print status message
message(STATUS "Configured ${PROJECT_NAME} ${PROJECT_VERSION}")