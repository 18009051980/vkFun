set(target "imageLoader")

file(GLOB_RECURSE srcs *.cpp)
add_library(${target} ${srcs})

target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${target} stb )
