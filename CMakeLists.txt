cmake_minimum_required(VERSION 3.25)
project(TomatoEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

add_executable(
        TomatoEngine
        src/renderer/renderer.cpp
        src/renderer/gpu.cpp
        src/renderer/mesh.cpp
        src/renderer/pipelines.cpp
        src/main.cpp
        src/renderer/texture.cpp
        src/renderer/model.cpp
        src/renderer/model.hpp
)

target_link_libraries(TomatoEngine PRIVATE SDL3::SDL3)
target_link_libraries(TomatoEngine PRIVATE glm::glm)