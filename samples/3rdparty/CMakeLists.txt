add_subdirectory(glfw)
add_subdirectory(glm)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/stb
)

add_subdirectory(tinyobjloader)