add_library(src-window STATIC Window.cpp)
target_include_directories(src-window PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-window PRIVATE
        src-utils-logger
        src-utils-event-dispatcher
        glfw
        volk::volk
        volk::volk_headers
)