cmake_minimum_required(VERSION 3.20)

# -------------------------------
# Project setup
# -------------------------------
project(JsonPlus
    VERSION 1.0.0
    DESCRIPTION "A lightweight JSON library extension that supports JSON inclusion and composition using nlohmann/json."
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------
# Library target (header-only)
# -------------------------------
add_library(JsonPlus INTERFACE)

# Include directories
target_include_directories(JsonPlus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/json/single_include>
    $<INSTALL_INTERFACE:include>
)

# Optionally specify headers (for IDEs)
# file(GLOB_RECURSE JSONPLUS_HEADERS
#     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
# )
# target_sources(JsonPlus INTERFACE ${JSONPLUS_HEADERS})

# -------------------------------
# Example targets (auto-discovery)
# -------------------------------
file(GLOB EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")
foreach(example_src ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_src} NAME_WE)
    add_executable(${example_name} ${example_src})
    target_link_libraries(${example_name} PRIVATE JsonPlus)
    target_include_directories(${example_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endforeach()

# -------------------------------
# Install rules
# -------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/JsonPlus/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/JsonPlus
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS JsonPlus
    EXPORT JsonPlus_libTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT JsonPlus_libTargets
    FILE JsonPlus_lib-targets.cmake
    NAMESPACE JsonPlus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlus_lib-configVersion.cmake"
    VERSION ${version}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlus_lib-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlus_lib-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlus_lib-configVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

export(EXPORT JsonPlus_libTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/JsonPlus_lib-targets.cmake"
    NAMESPACE JsonPlus::
)

# -------------------------------
# Optional: developer output
# -------------------------------
message(STATUS "Configured JsonPlus v${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Examples found: ${EXAMPLE_SOURCES}")
