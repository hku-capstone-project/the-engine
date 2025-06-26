add_library(src-dotnet STATIC
    RuntimeBridge.cpp
)

target_include_directories(src-dotnet PRIVATE ${vcpkg_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/ ${CMAKE_SOURCE_DIR}/dep/dotnet-runtime-8.0.16)

target_link_libraries(src-dotnet PRIVATE
    "${CMAKE_SOURCE_DIR}/${DOTNET_HOSTING_DIR}/nethost.lib"
)
