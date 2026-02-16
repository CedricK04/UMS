cmake_minimum_required(VERSION 3.16)
project(ums-core
    VERSION 0.1.0
    DESCRIPTION "Universal Microcontroller Scope - Core Library"
    LANGUAGES C CXX)

# Set C/C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(UMS_BUILD_TESTS "Build unit tests" ON)
option(UMS_ENABLE_VALGRIND "Enable Valgrind memory checking" OFF)
option(UMS_BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Export compile commands for editor integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add module path for custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Create the library
add_subdirectory(src)

# Testing
if(UMS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
include(GNUInstallDirs)

install(DIRECTORY include/ums
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

install(TARGETS ums-core
    EXPORT ums-core-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ums)

# Export CMake targets
install(EXPORT ums-core-targets
    FILE ums-core-targets.cmake
    NAMESPACE ums::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ums-core)

# Create package config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ums-core-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ums-core-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ums-core-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ums-core)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ums-core-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ums-core-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ums-core)
