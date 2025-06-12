



set(target "Engine")
# 将源代码添加到此项目的可执行文件。

file(GLOB srcs CONFIGURE_DEPENDS *.cpp)
add_library(${target} ${srcs})

find_package(Vulkan REQUIRED)

target_include_directories(${target} INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${target} Vulkan::Vulkan)

# TODO: 如有需要，请添加测试并安装目标。
