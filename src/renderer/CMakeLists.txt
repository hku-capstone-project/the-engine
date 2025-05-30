add_library(src-renderer STATIC
    Renderer.cpp
)

target_include_directories(src-renderer PRIVATE
    ${vcpkg_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/resources/shaders/include/
)

target_link_libraries(src-renderer PRIVATE
    src-camera
)
