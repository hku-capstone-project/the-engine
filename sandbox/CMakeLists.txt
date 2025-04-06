add_executable(sandbox main.cpp)

target_include_directories(sandbox PRIVATE ${CMAKE_SOURCE_DIR}/vengine/)

# target_link_libraries(run PRIVATE
# src-utils-logger
# src-app
# )

target_link_libraries(sandbox PRIVATE
    vengine
)
