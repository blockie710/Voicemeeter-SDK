cmake_minimum_required(VERSION 3.10)
project(VoicemeeterPluginHostTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/PluginHost/src)

# Define M_PI if needed (Windows)
add_definitions(-D_USE_MATH_DEFINES)

# Collect source files for the plugin host implementation
file(GLOB PLUGIN_HOST_SOURCES 
    "${CMAKE_SOURCE_DIR}/PluginHost/src/*.cpp"
)

# Add the test executable
add_executable(test_plugin_host 
    test_existing_plugin_host.cpp
    ${PLUGIN_HOST_SOURCES}
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(test_plugin_host PRIVATE user32 gdi32)
endif()