add_executable(run main.cpp)

target_include_directories(run PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

target_link_libraries(run PUBLIC
    daxa::daxa
    src-app
)
