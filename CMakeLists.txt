cmake_minimum_required(VERSION 3.25)
project(TomatoEngine LANGUAGES CXX)

include(FetchContent)

set(DOUBLE_PRECISION OFF)
# When turning this option on, the library will be compiled with debug symbols
set(GENERATE_DEBUG_SYMBOLS ON)
# When turning this option on, the library will override the default CMAKE_CXX_FLAGS_RELEASE values, otherwise they will use the platform defaults
set(OVERRIDE_CXX_FLAGS ON)
# When turning this option on, the library will be compiled in such a way to attempt to keep the simulation deterministic across platforms
set(CROSS_PLATFORM_DETERMINISTIC OFF)
# When turning this option on, the library will be compiled with interprocedural optimizations enabled, also known as link-time optimizations or link-time code generation.
set(INTERPROCEDURAL_OPTIMIZATION ON)
# When turning this on, in Debug and Release mode, the library will emit extra code to ensure that the 4th component of a 3-vector is kept the same as the 3rd component
# Note that this currently only works using MSVC. Clang turns Float2 into a SIMD vector sometimes causing floating point exceptions (the option is ignored).
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
# This adds some overhead and Jolt doesn't use exceptions so by default it is off.
set(CPP_EXCEPTIONS_ENABLED OFF)
# This adds some overhead and Jolt doesn't use RTTI so by default it is off.
set(CPP_RTTI_ENABLED OFF)
# Jolt defaults this to ON, which would also turn on -Werror for our own project
# and Jolt itself has warnings (e.g. array-bounds on GCC 15) that break the build with it enabled.
set(ENABLE_ALL_WARNINGS OFF)
# Number of bits to use in ObjectLayer. Can be 16 or 32.
set(OBJECT_LAYER_BITS 16)

# Select X86 processor features to use, by default the library compiles with AVX2, if everything is off it will be SSE2 compatible.
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "v5.7.0"
        SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    # 64 bit architecture
    set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64")

    # Set runtime library
    if (USE_STATIC_MSVC_RUNTIME_LIBRARY)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    # Set general compiler flags
    add_compile_options(/Zc:__cplusplus /MP /nologo /diagnostics:classic /FC /fp:except- /Zc:inline)

    # Enable warnings
    if (ENABLE_ALL_WARNINGS)
        add_compile_options(/Wall /WX)
    endif()

    # Optionally generate debug symbols
    if (GENERATE_DEBUG_SYMBOLS)
        add_compile_options(/Zi)
    endif()

    if (NOT CPP_RTTI_ENABLED)
        # Disable RTTI
        add_compile_options(/GR-)
    else()
        # Enable RTTI
        add_compile_options(/GR)
    endif()

    if (NOT CPP_EXCEPTIONS_ENABLED)
        # Remove any existing compiler flag that enables exceptions
        string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

        # Disable warning about STL and compiler-generated types using noexcept when exceptions are disabled
        add_compile_options(/wd4577)
    else()
        # Enable exceptions
        add_compile_options(/EHsc)
    endif()

    if (OVERRIDE_CXX_FLAGS)
        # No buffer security checks, enable function-level linking, enable intrinsic functions, favor fast code
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GS- /Gy /Oi /Ot")
    endif()

    # Set linker flags
    if (GENERATE_DEBUG_SYMBOLS)
        add_link_options(/DEBUG)
    endif()
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if (CROSS_PLATFORM_DETERMINISTIC)
            add_compile_options(/fp:precise)
        else()
            add_compile_options(/fp:fast) # Clang doesn't use fast math because it cannot be turned off inside a single compilation unit
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(/showFilenames -Qunused-arguments) # Clang emits warnings about unused arguments such as /MP and /GL
    endif()
else()
    # Enable warnings
    if (ENABLE_ALL_WARNINGS)
        add_compile_options(-Wall -Werror)
    endif()

    # Optionally generate debug symbols
    if (GENERATE_DEBUG_SYMBOLS)
        add_compile_options(-g${JPH_DEBUG_SYMBOL_FORMAT})
    endif()

    if (NOT CPP_RTTI_ENABLED)
        # Disable RTTI
        add_compile_options(-fno-rtti)
    else()
        # Enable RTTI
        add_compile_options(-frtti)
    endif()

    if (NOT CPP_EXCEPTIONS_ENABLED)
        # Disable exception-handling
        add_compile_options(-fno-exceptions)
    else()
        # Enable exception-handling
        add_compile_options(-fexceptions)
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # Also disable -Wstringop-overflow or it will generate false positives that can't be disabled from code when link-time optimizations are enabled
        # Also disable -Wno-psabi to avoid messages of the form note: parameter passing for argument of type '...' changed in GCC 7.1
        add_compile_options(-Wno-stringop-overflow -Wno-psabi)
    endif()

    # Do not use -ffast-math since it creates too many inaccuracies under clang
    # FMA is not compatible with cross platform determinism
    if (CROSS_PLATFORM_DETERMINISTIC OR NOT USE_FMADD OR EMSCRIPTEN)
        add_compile_options(-ffp-contract=off)
    else()
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^x86")
            # On x86 our FMA intrinsics are preserved
            add_compile_options(-ffp-contract=fast)
        else()
            # On ARM (and maybe other platforms) using 'fast' mode actually replaces a FMA intrinsic in Debug / when using LTO (?) with a 'mul' and an 'add' causing us to lose precision
            add_compile_options(-ffp-contract=on)
        endif()
    endif()

    # See https://github.com/jrouwe/JoltPhysics/issues/922. When compiling with DOUBLE_PRECISION=YES and CMAKE_OSX_DEPLOYMENT_TARGET=10.12 clang triggers a warning that we silence here.
    if ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        add_compile_options(-faligned-allocation)
    endif()

    # Set compiler flags for various configurations
    set(CMAKE_CXX_FLAGS_DISTRIBUTION "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS_DISTRIBUTION "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

# Enable link time optimization in Release and Distribution mode if requested and available
SET_INTERPROCEDURAL_OPTIMIZATION()

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
        src/gameplay/physics.cpp
        src/gameplay/physics.hpp
)

if (EMSCRIPTEN)
    # Increase stack and memory size when running with emscripten
    target_link_options(TomatoEngine PUBLIC -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=134217728)
endif()

target_link_libraries(TomatoEngine PRIVATE SDL3::SDL3)
target_link_libraries(TomatoEngine PRIVATE glm::glm)
target_link_libraries(TomatoEngine PRIVATE Jolt::Jolt)