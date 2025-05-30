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

# glm::glm
find_package(glm CONFIG REQUIRED)

# EnTT::EnTT
find_package(EnTT CONFIG REQUIRED)

# spdlog::spdlog
find_package(spdlog CONFIG REQUIRED)

# volk::volk volk::volk_headers
find_package(volk CONFIG REQUIRED)

# Vulkan::Headers GPUOpen::VulkanMemoryAllocator
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# imgui::imgui
find_package(imgui CONFIG REQUIRED)

# implot::implot
find_package(implot CONFIG REQUIRED)

# PkgConfig::tomlplusplus
find_package(PkgConfig REQUIRED)

# unofficial::shaderc::shaderc
find_package(unofficial-shaderc CONFIG REQUIRED)

# assimp::assimp
find_package(assimp CONFIG REQUIRED)

set(vcpkg_INCLUDE_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/")
message(STATUS "vcpkg include dir: ${vcpkg_INCLUDE_DIR}")

add_subdirectory(src/)
add_subdirectory(apps/)
