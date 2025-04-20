cmake_minimum_required(VERSION 3.14)
project(vmr_client VERSION 1.0.0 LANGUAGES C)

# Source files
set(SOURCES
    vmr_client.c
)

# Header files
set(HEADERS
    vmr_client.h
)

# Resource files for Windows
if(WIN32)
    set(RESOURCES
        vmr_client.rc
    )
endif()

# Define the executable
add_executable(vmr_client ${SOURCES} ${HEADERS} ${RESOURCES})

# Include directories
target_include_directories(vmr_client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_SOURCE_DIR}
)

# Platform-specific settings
if(WIN32)
    # Windows-specific libraries
    target_link_libraries(vmr_client PRIVATE winmm.lib user32.lib gdi32.lib)
    
    # For Visual Studio, set the startup project and working directory
    if(MSVC)
        set_property(TARGET vmr_client PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()
elseif(APPLE)
    # macOS-specific libraries
    find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox REQUIRED)
    target_link_libraries(vmr_client PRIVATE ${COREAUDIO_LIBRARY} ${AUDIOTOOLBOX_LIBRARY})
else()
    # Linux-specific libraries
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA REQUIRED alsa)
    target_include_directories(vmr_client PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(vmr_client PRIVATE ${ALSA_LIBRARIES} dl pthread)
endif()

# Installation rules
install(TARGETS vmr_client
    RUNTIME DESTINATION bin
)