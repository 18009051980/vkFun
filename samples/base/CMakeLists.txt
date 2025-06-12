set(target "Base")

file(GLOB_RECURSE srcs *.cpp)
add_executable(${target} ${srcs}) 

find_package(Vulkan REQUIRED)


find_program(GLSLC_EXECUTABLE glslc REQUIRED)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please ensure it is installed and available in your PATH.")
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
    COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
    COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
)

add_custom_target(compile_shaders 
    COMMENT "Compiling shaders to SPIR-V"
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vert.spv ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
)


add_library(shader_assets INTERFACE)
add_dependencies(shader_assets compile_shaders)


# 动态获取资源文件
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

# 获取相对路径并复制文件
foreach(ASSET_FILE ${ASSET_FILES})
    # 获取相对于 assets 目录的路径
    file(RELATIVE_PATH RELATIVE_PATH_TO_ASSET ${CMAKE_CURRENT_SOURCE_DIR}/assets ${ASSET_FILE})

    # 目标路径：构建目录中的 assets 子目录
    set(DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets/${RELATIVE_PATH_TO_ASSET}")

    # 添加复制命令
    add_custom_command(
        OUTPUT ${DEST_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSET_FILE} ${DEST_FILE}
        DEPENDS ${ASSET_FILE}
        COMMENT "Copying asset ${RELATIVE_PATH_TO_ASSET} to build directory"
    )

    # 将目标文件添加到列表中
    list(APPEND COPIED_ASSETS ${DEST_FILE})
endforeach()

# 创建一个目标来复制所有资源文件
add_custom_target(copy_assets DEPENDS ${COPIED_ASSETS})

add_library(assets INTERFACE)
add_dependencies(assets copy_assets)


target_link_libraries(${target} shader_assets assets glfwApp glm Vulkan::Vulkan imageLoader objLoader)
