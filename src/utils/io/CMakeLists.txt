add_library(src-utils-io FileReader.cpp)
target_include_directories(src-utils-io PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-utils-io PRIVATE src-utils-logger)
