############################################################
# CMakeLists for the symbolic library.
#
# Copyright 2020. All Rights Reserved.
#
# Created: May 7, 2020
# Authors: Toki Migimatsu
############################################################

cmake_minimum_required(VERSION 3.11)

# Define project.
project(symbolic
    VERSION 1.0.3
    DESCRIPTION "Library for manipulating PDDL symbols"
    LANGUAGES CXX)
string(TOUPPER ${PROJECT_NAME} LIB_CMAKE_NAME)
set(LIB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Include custom CMake utilities.
include(cmake/utils.cmake)

# Detect whether this project is built by itself or included as a subdirectory.
lib_dependent_option(${LIB_CMAKE_NAME}_MAIN_BUILD
    "Build ${PROJECT_NAME} as a main project."
    "CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME"
)

# Generate a compilation database for autocompletion.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define CMake options.
lib_option(BUILD_DOCS "Build docs." OFF)
lib_option(BUILD_EXAMPLES "Build examples." ON)
lib_option(BUILD_PYTHON "Build Python library." OFF)
lib_option(BUILD_TESTING "Build tests." OFF)
lib_option(CLANG_TIDY "Perform clang-tidy checks." OFF)

# Set default build type to release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Standardize install directories.
include(GNUInstallDirs)

# Build the library.
add_subdirectory(src)

# Ensure shared library is built
set(BUILD_SHARED_LIBS ON)

# Generate and install CMake package config files
include(CMakePackageConfigHelpers)
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/symbolic")

install(TARGETS symbolic
    # EXPORT symbolicTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export the target file for find_package()
install(EXPORT symbolicTargets
    FILE symbolicTargets.cmake
    NAMESPACE symbolic::
    DESTINATION ${CONFIG_INSTALL_DIR})

# Create a config file for find_package()
configure_package_config_file(
    cmake/symbolicConfig.cmake.in
    ${CMAKE_BINARY_DIR}/symbolicConfig.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)

install(FILES
    ${CMAKE_BINARY_DIR}/symbolicConfig.cmake
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# Build examples.
if(${LIB_CMAKE_NAME}_BUILD_EXAMPLES)
    add_subdirectory(apps)
endif()

# Build tests.
if(${LIB_CMAKE_NAME}_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Build docs.
if(${LIB_CMAKE_NAME}_BUILD_DOCS)
    find_package(Doxygen)
    if(NOT Doxygen_FOUND)
        message(WARNING "Unable to find Doxygen for ${PROJECT_NAME} docs.")
    else()
        add_subdirectory(docs)
    endif()
endif()

