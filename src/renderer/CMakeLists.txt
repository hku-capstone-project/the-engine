add_library(src-renderer STATIC
    Renderer.cpp
)

target_include_directories(src-renderer PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(src-renderer PRIVATE
)
