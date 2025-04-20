cmake_minimum_required(VERSION 3.10)
project(VoicemeeterPluginHostTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest QUIET)
if(NOT GTEST_FOUND)
    # Download and build googletest if not found
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Add include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/PluginHost/src)
include_directories(${CMAKE_SOURCE_DIR}/PluginHost/include)

# Define M_PI if needed (Windows)
add_definitions(-D_USE_MATH_DEFINES)

# Collect source files for the plugin host implementation
file(GLOB PLUGIN_HOST_SOURCES 
    "${CMAKE_SOURCE_DIR}/PluginHost/src/*.cpp"
)

# Add the existing test executable
add_executable(test_plugin_host 
    test_existing_plugin_host.cpp
    ${PLUGIN_HOST_SOURCES}
)
target_link_libraries(test_plugin_host VMPluginHost)

# Add the new simplified test executable
add_executable(simple_plugin_test
    simple_plugin_test.cpp
    ${PLUGIN_HOST_SOURCES}
)
target_link_libraries(simple_plugin_test VMPluginHost)

# Add new unit tests using Google Test
add_executable(plugin_utils_test 
    plugin_utils_test.cpp
)
target_link_libraries(plugin_utils_test
    gtest
    gtest_main
    VMPluginHost
)

# Add plugin scanner test executable
add_executable(plugin_scanner_test plugin_scanner_test.cpp)
target_link_libraries(plugin_scanner_test VMPluginHost)

# Platform-specific settings
if(WIN32)
    target_link_libraries(test_plugin_host PRIVATE user32 gdi32)
    target_link_libraries(simple_plugin_test PRIVATE user32 gdi32)
endif()

# Create test runner script
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
    @ONLY
)

# Make the script executable
if(UNIX)
    execute_process(
        COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/run_tests.sh
    )
endif()

# Install test script
install(FILES test_plugin_host.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)