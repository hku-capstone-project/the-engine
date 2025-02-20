# cmake guidelines: https://cliutils.gitlab.io/modern-cmake/
cmake_minimum_required(VERSION 3.10.0)
project(the_engine)
set(CMAKE_CXX_STANDARD 20)

if(WITH_PORTABLE_RESOURCES)
    message(STATUS "Portable resources folder is used")
    add_definitions(-DPORTABLE_RESOURCES_FOLDER)
endif()

configure_file(${CMAKE_SOURCE_DIR}/src/config/RootDir.h.in ${CMAKE_SOURCE_DIR}/src/config/RootDir.h)

# glfw
find_package(glfw3 CONFIG REQUIRED)

# volk::volk volk::volk_headers
find_package(volk CONFIG REQUIRED)

# Vulkan::Headers GPUOpen::VulkanMemoryAllocator
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

set(vcpkg_INCLUDE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/")
message(STATUS "vcpkg include dir: ${vcpkg_INCLUDE_DIR}")

# add_subdirectory(src/)
add_subdirectory(apps/)

# copy compile_commands.json to .vscode folder for clangd to use, this is because the build dir for debug and release mode is different
add_custom_target(copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/.vscode
)
