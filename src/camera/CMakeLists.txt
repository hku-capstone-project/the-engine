add_library(src-camera Camera.cpp)
target_include_directories(src-camera PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-camera PRIVATE
    src-utils-logger
    src-window
    src-config-container
    glm::glm
)
