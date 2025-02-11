add_library(src-app STATIC Application.cpp)

target_include_directories(src-app PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(src-app PRIVATE
    daxa::daxa
    src-window
)
