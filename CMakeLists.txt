cmake_minimum_required(VERSION 3.22)

project(puddin_pool
    VERSION 0.1.0
    DESCRIPTION "Puddin Pool: Modular headless 2D billiards physics engine"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output all binaries and shared libraries into a single bin directory for easy resolution
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Default to build shared libraries for core & bindings
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build CLI tests" ON)

# macOS & Apple platforms rpath configuration
if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_INSTALL_RPATH "@loader_path;@executable_path;@rpath")
endif()

# Platform-specific presets/configs
if(IOS)
    message(STATUS "Targeting iOS Framework")
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
    set(CMAKE_IOS_INSTALL_COMBINED YES)
elseif(ANDROID)
    message(STATUS "Targeting Android NDK (${ANDROID_ABI})")
    add_compile_definitions(ANDROID_PLATFORM)
elseif(APPLE)
    message(STATUS "Targeting macOS Shared Dynamic Library (.dylib)")
endif()

# Subdirectories
add_subdirectory(core)
add_subdirectory(bindings)
add_subdirectory(view)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
