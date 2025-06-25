add_library(src-utils-model-loader STATIC ModelLoader.cpp ModelManager.cpp)
target_include_directories(src-utils-model-loader PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(src-utils-model-loader PRIVATE assimp::assimp)