cmake_minimum_required(VERSION 3.20)
project(particle-simulation VERSION 1.0.0)

find_package(OpenGL REQUIRED)

add_subdirectory(libs/glfw-3.4)

add_executable(${PROJECT_NAME} src/glad.c src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/libs)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)