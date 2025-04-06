add_library(vengine SHARED Application.cpp)

target_include_directories(vengine PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/vengine/src/)
