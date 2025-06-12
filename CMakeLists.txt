cmake_minimum_required (VERSION 3.10)

# 设置 C++23 标准（或 C++20）
set(CMAKE_CXX_STANDARD 23)  # 或 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 禁用编译器扩展（如 GNU 扩展）



project ("RenderingEngine")

add_subdirectory(src)
add_subdirectory(samples)
