add_library(src-utils-image-loader STATIC ImageLoader.cpp)
target_include_directories(src-utils-image-loader PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)