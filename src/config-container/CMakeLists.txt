add_library(src-config-container STATIC
        sub-config/ApplicationInfo.cpp
        sub-config/CameraInfo.cpp
        sub-config/DebugInfo.cpp
        sub-config/ImguiManagerInfo.cpp
        sub-config/RendererInfo.cpp
        ConfigContainer.cpp
)

target_include_directories(src-config-container PRIVATE
        ${vcpkg_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src/
)

target_link_libraries(src-config-container PRIVATE
        src-utils-logger
        src-utils-toml-config
        glm::glm
)
