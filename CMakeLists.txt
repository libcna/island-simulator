cmake_minimum_required(VERSION 3.23)

project(IslandSimulator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ISLAND_BUILD_GAME "Build the graphical island-simulator application" ON)
option(ISLAND_BUILD_TESTS "Build island-simulator core tests" ON)
option(ISLAND_STRICT_DEPENDENCY_REVISIONS
       "Require the dependency revisions recorded in dependencies.lock" ON)

set(CNA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/dependencies/cna-next" CACHE PATH
    "Read-only CNA next source tree")
set(ISLAND_SHARP_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../sharp-runtime" CACHE PATH
    "Read-only Sharp Runtime next checkout")
set(ISLAND_EASY_GL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../easy-gl" CACHE PATH
    "Read-only EasyGL develop checkout")
set(ISLAND_META_GL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../meta-gl" CACHE PATH
    "Read-only MetaGL develop checkout")

include(cmake/VerifyDependencies.cmake)
island_verify_dependencies()

add_library(island_core STATIC
    game/src/GrassLayout.cpp
    game/src/IslandTerrain.cpp
    game/src/PlayerMotion.cpp
    game/src/VegetationLayout.cpp)
add_library(IslandSimulator::Core ALIAS island_core)

target_include_directories(island_core PUBLIC game/include)
target_compile_features(island_core PUBLIC cxx_std_23)

function(island_enable_first_party_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

function(island_enable_cna_consumer_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        # Sharp Runtime intentionally exposes compiler-provided __int128 in a public header.
        # Keep warnings fatal for the game while avoiding -Wpedantic on that supported API.
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    endif()
endfunction()

island_enable_first_party_warnings(island_core)

if(ISLAND_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_executable(island_core_tests tests/CoreTests.cpp)
    target_link_libraries(island_core_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_core_tests)
    add_test(NAME IslandCoreTests COMMAND island_core_tests)
endif()

if(ISLAND_BUILD_GAME)
    if(NOT EXISTS "${CNA_ROOT_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "CNA next was not found at '${CNA_ROOT_DIR}'. Run\n"
            "  tools/materialize_cna_next.sh\n"
            "or configure with -DCNA_ROOT_DIR=/path/to/a/read-only/cna-next checkout.")
    endif()

    set(CNA_BUILD_TESTS OFF CACHE BOOL "Build CNA tests" FORCE)
    set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "Build CNA examples" FORCE)
    set(CNA_ENABLE_NET OFF CACHE BOOL "Build CNA networking" FORCE)
    set(CNA_ENABLE_VIDEO OFF CACHE STRING "Build CNA video playback" FORCE)
    set(CNA_ENABLE_DRACO OFF CACHE BOOL "Build Draco mesh decoding" FORCE)
    set(CNA_ENABLE_FONT_PIPELINE OFF CACHE STRING "Build the font pipeline" FORCE)
    set(CNA_CNB_ZSTD OFF CACHE STRING "Build Zstandard CNB support" FORCE)
    # Portable multi-language ShaderEffect packages are an opt-in CNA extension. Ocean and grass
    # keep explicit GLSL Desktop and GLSL ES variants for Linux, WebGL 2, and GLES 3 parity.
    set(CNA_CNAEXT ON CACHE BOOL "Enable CNA's portable shader infrastructure" FORCE)
    set(SHARP_RUNTIME_BUILD_TESTS OFF CACHE BOOL "Build Sharp Runtime tests" FORCE)
    set(EASYGL_BUILD_TESTS OFF CACHE BOOL "Build EasyGL tests" FORCE)
    set(EASYGL_BUILD_EXAMPLES OFF CACHE BOOL "Build EasyGL examples" FORCE)

    if(NOT DEFINED CNA_GRAPHICS_RENDERER OR CNA_GRAPHICS_RENDERER STREQUAL "")
        if(EMSCRIPTEN)
            set(CNA_GRAPHICS_RENDERER WEBGL2 CACHE STRING "CNA renderer" FORCE)
        elseif(ANDROID)
            set(CNA_GRAPHICS_RENDERER OPENGLES3 CACHE STRING "CNA renderer" FORCE)
        else()
            set(CNA_GRAPHICS_RENDERER OPENGL33 CACHE STRING "CNA renderer" FORCE)
        endif()
    endif()

    set(CNA_SHARP_RUNTIME_ROOT "${ISLAND_SHARP_RUNTIME_ROOT}" CACHE PATH
        "Sharp Runtime checkout used by CNA" FORCE)
    set(CNA_EASYGL_ROOT "${ISLAND_EASY_GL_ROOT}" CACHE PATH
        "EasyGL checkout used by CNA" FORCE)

    # Keep configure-time vendored SDL outputs out of the immutable CNA export. This is needed by
    # Web and Android; native Linux uses the system packages configured below.
    if(NOT DEFINED CNA_SDL_PREBUILT_ROOT)
        if(EMSCRIPTEN)
            set(_island_sdl_cache_target emscripten)
        elseif(ANDROID)
            set(_island_sdl_cache_target "android-${ANDROID_ABI}")
        else()
            set(_island_sdl_cache_target native)
        endif()
        set(CNA_SDL_PREBUILT_ROOT
            "${CMAKE_CURRENT_SOURCE_DIR}/build/cna-sdl-prebuilt-${_island_sdl_cache_target}"
            CACHE PATH "Writable CNA SDL build cache outside the read-only CNA export")
    endif()

    # Load CNA's own component list before it adds Sharp Runtime. This avoids pulling optional
    # Sharp Runtime modules into cross-target builds without duplicating framework knowledge.
    set(_island_cna_sharp_consumption
        "${CNA_ROOT_DIR}/cmake/SharpRuntimeConsumption.cmake")
    if(EXISTS "${_island_cna_sharp_consumption}")
        include("${_island_cna_sharp_consumption}")
        if(NOT DEFINED SHARP_RUNTIME_COMPONENTS)
            set(SHARP_RUNTIME_COMPONENTS "${CNA_SHARP_RUNTIME_DEFAULT_COMPONENTS}"
                CACHE STRING "Sharp Runtime components required by CNA")
        endif()
    endif()

    # The host provides SDL 3, SDL_image 3, and SDL_mixer 3. Cross-target builds use CNA's
    # platform packaging instead and can override this cache entry.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND NOT DEFINED CNA_USE_SYSTEM_SDL)
        set(CNA_USE_SYSTEM_SDL ON CACHE BOOL "Use system SDL packages" FORCE)
    endif()

    # CNA is a dependency, not part of this project's default build surface. Its linked runtime
    # targets are still built transitively, while unrelated offline tools stay out of ordinary
    # `cmake --build` iterations.
    add_subdirectory("${CNA_ROOT_DIR}" CNA EXCLUDE_FROM_ALL)

    add_executable(island-simulator
        game/src/GrassRenderer.cpp
        game/src/IslandGame.cpp
        game/src/Main.cpp
        game/src/OceanRenderer.cpp
        game/src/PalmRenderer.cpp)
    target_include_directories(island-simulator PRIVATE game/include)
    target_link_libraries(island-simulator PRIVATE IslandSimulator::Core CNA)
    island_enable_cna_consumer_warnings(island-simulator)

    if(COMMAND cna_copy_renderer_runtime)
        cna_copy_renderer_runtime(island-simulator)
    endif()
    if(TARGET SDL3::SDL3main)
        target_link_libraries(island-simulator PRIVATE SDL3::SDL3main)
    endif()

    if(EMSCRIPTEN)
        if(TARGET CNA::EmscriptenAsyncify)
            target_link_libraries(island-simulator PRIVATE CNA::EmscriptenAsyncify)
        endif()
        set_target_properties(island-simulator PROPERTIES SUFFIX ".html")
        target_link_options(island-simulator PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sSTACK_SIZE=1048576
            -sINITIAL_MEMORY=134217728
            -sFORCE_FILESYSTEM=1
            -sEXIT_RUNTIME=1
            -sMIN_WEBGL_VERSION=2
            -sMAX_WEBGL_VERSION=2)
    endif()

    if(ISLAND_BUILD_TESTS AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME IslandGameSmoke COMMAND island-simulator --smoke-test)
        set_tests_properties(IslandGameSmoke PROPERTIES TIMEOUT 30 LABELS display)
        add_test(NAME IslandGameCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-capture.ppm")
        set_tests_properties(IslandGameCapture PROPERTIES TIMEOUT 30 LABELS display)
        add_test(NAME IslandGameBeachCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-beach-capture.ppm"
                    --viewpoint beach)
        set_tests_properties(IslandGameBeachCapture PROPERTIES TIMEOUT 30 LABELS display)
    endif()
endif()
