set(target "ssao")

file(GLOB_RECURSE srcs *.cpp)
add_executable(${target} ${srcs}) 

find_package(Vulkan REQUIRED)


find_program(GLSLC_EXECUTABLE glslc REQUIRED)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please ensure it is installed and available in your PATH.")
endif()

file(GLOB shaders CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*
)
set (outputShaders "")
foreach(shader ${shaders})
    get_filename_component(shader_name ${shader} NAME)
    get_filename_component(shader_path ${shader} PATH)
    get_filename_component(shader_base ${shader_name} NAME_WE)
    
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${shader_name}.spv
        COMMAND ${GLSLC_EXECUTABLE} ${shader} -o ${CMAKE_CURRENT_BINARY_DIR}/${shader_name}.spv
        DEPENDS ${shader}
    )
    list(APPEND outputShaders ${CMAKE_CURRENT_BINARY_DIR}/${shader_name}.spv)
    
endforeach()

add_custom_target(${target}_compile_shaders 
    COMMENT "Compiling shaders to SPIR-V"
    DEPENDS ${outputShaders}
)

add_library(${target}_shader_target INTERFACE)
add_dependencies(${target}_shader_target ${target}_compile_shaders)


# 动态获取资源文件
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

# 获取相对路径并复制文件
foreach(ASSET_FILE ${ASSET_FILES})
    # 获取相对于 ${target}_assets 目录的路径
    file(RELATIVE_PATH RELATIVE_PATH_TO_ASSET ${CMAKE_CURRENT_SOURCE_DIR}/assets ${ASSET_FILE})

    # 目标路径：构建目录中的 ${target}_assets 子目录
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
add_custom_target(${target}_copy_assets DEPENDS ${COPIED_ASSETS})

add_library(${target}_assets INTERFACE)
add_dependencies(${target}_assets ${target}_copy_assets)


target_link_libraries(${target} ${target}_shader_target ${target}_assets glfwApp glm Vulkan::Vulkan imageLoader objLoader)
