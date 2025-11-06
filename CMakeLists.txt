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

# Install the header-only target
install(TARGETS JsonPlus
    EXPORT JsonPlusTargets
)

# Install header files
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY Vendor/json/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nlohmann)

# -------------------------------
# Export configuration for find_package()
# -------------------------------
install(EXPORT JsonPlusTargets
    FILE JsonPlusTargets.cmake
    NAMESPACE JsonPlus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JsonPlus
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlusConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/JsonPlusConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlusConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JsonPlus
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlusConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/JsonPlusConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JsonPlus
)

# -------------------------------
# Optional: developer output
# -------------------------------
message(STATUS "Configured JsonPlus v${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Examples found: ${EXAMPLE_SOURCES}")
