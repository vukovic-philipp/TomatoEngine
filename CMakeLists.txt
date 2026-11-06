cmake_minimum_required(VERSION 3.25)
project(TomatoEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

add_executable(TomatoEngine src/main.cpp
        src/gpu.cpp
        src/gpu.hpp
        src/mesh.cpp
        src/mesh.hpp
        src/pipelines.cpp
        src/pipelines.hpp)
target_link_libraries(TomatoEngine PRIVATE SDL3::SDL3)
target_link_libraries(TomatoEngine PRIVATE glm::glm)