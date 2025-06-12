set(target "pbr")

file(GLOB srcs *.cpp)

add_executable(${target} ${srcs})

find_package(Vulkan REQUIRED)

find_program(GLSL_EXEC glslc REQUIRED)


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




target_link_libraries(${target} glfwApp glm Vulkan::Vulkan ${target}_shader_target objLoader)