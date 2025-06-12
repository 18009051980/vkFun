set(target "objLoader")

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp)

add_library(${target} ${srcs})

target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Vulkan REQUIRED)

target_link_libraries(${target} tinyobjloader glm Vulkan::Vulkan Vertex)
