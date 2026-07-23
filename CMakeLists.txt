cmake_minimum_required(VERSION 3.20)
project(particle-simulation VERSION 1.0.0)

find_package(OpenGL REQUIRED)

add_subdirectory(libs/glfw-3.4)
add_subdirectory(libs/glm)

set(SOURCES
    src/main.cpp
    src/glad.c
    src/core/shaderUtils.cpp
    src/simulations/clothSimulation.cpp
    src/application.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_impl_glfw.cpp
    libs/imgui/imgui_impl_opengl3.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui.cpp
    src/core/simulation2D.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/libs)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL glm::glm-header-only)