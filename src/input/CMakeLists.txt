set(INPUT_SOURCES
    InputSystem.cpp
)

set(INPUT_HEADERS
    InputSystem.hpp
)

add_library(input STATIC ${INPUT_SOURCES} ${INPUT_HEADERS})

target_include_directories(input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 只链接必要的库，不链接window（避免循环依赖）
target_link_libraries(input
    PRIVATE
        glfw
) 