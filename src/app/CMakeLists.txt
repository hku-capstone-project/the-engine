add_library(src-app STATIC Application.cpp)

target_include_directories(src-app PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(src-app PRIVATE
        src-renderer
        src-vulkan-wrapper
        src-window
        src-imgui-manager
        src-app-context
        src-config-container
        src-utils-io
        src-utils-logger
        src-utils-fps-sink
        src-utils-model-loader
        src-utils-shader-compiler
        src-dotnet
        glm::glm
)
