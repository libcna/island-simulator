cmake_minimum_required(VERSION 3.23)

project(IslandSimulator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ISLAND_BUILD_GAME "Build the graphical island-simulator application" ON)
option(ISLAND_BUILD_TESTS "Build island-simulator core tests" ON)
option(ISLAND_BUILD_TOOLS "Build island-simulator host development tools" ON)
option(ISLAND_STRICT_DEPENDENCY_REVISIONS
       "Require the dependency revisions recorded in dependencies.lock" ON)
option(ISLAND_ENABLE_SANITIZERS
       "Instrument first-party island-simulator targets with AddressSanitizer and UBSan" OFF)

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
    game/src/AquaticLightingModel.cpp
    game/src/AudioAnalysis.cpp
    game/src/BenchmarkTraversalRoute.cpp
    game/src/BirdFlock.cpp
    game/src/DebugFreeCamera.cpp
    game/src/DistantSceneryLayout.cpp
    game/src/FirstPersonCameraMotion.cpp
    game/src/FirstPersonInput.cpp
    game/src/FishSchools.cpp
    game/src/GrassInteractionModel.cpp
    game/src/GrassLayout.cpp
    game/src/GrassLod.cpp
    game/src/IslandTerrain.cpp
    game/src/JungleLayout.cpp
    game/src/LocomotionPaceToggle.cpp
    game/src/MonkeyTroop.cpp
    game/src/NaturalPropLayout.cpp
    game/src/PerformanceStatistics.cpp
    game/src/PlayerMotion.cpp
    game/src/ProceduralMeshValidation.cpp
    game/src/SharkPatrol.cpp
    game/src/SoundscapeModel.cpp
    game/src/TouchControls.cpp
    game/src/VegetationLayout.cpp
    game/src/WaterRippleModel.cpp
    game/src/WaterViewModel.cpp
    game/src/WorldIdentity.cpp
    game/src/WorldCollisionGrid.cpp
    game/src/WildlifeSimulation.cpp)
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

function(island_enable_first_party_sanitizers target)
    if(NOT ISLAND_ENABLE_SANITIZERS)
        return()
    endif()
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "ISLAND_ENABLE_SANITIZERS is supported only for host builds")
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR
            "ISLAND_ENABLE_SANITIZERS requires a GCC- or Clang-compatible compiler")
    endif()

    target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer)
    get_target_property(_island_target_type ${target} TYPE)
    if(NOT _island_target_type STREQUAL "STATIC_LIBRARY")
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

island_enable_first_party_warnings(island_core)
island_enable_first_party_sanitizers(island_core)

if(ISLAND_BUILD_TOOLS AND NOT CMAKE_CROSSCOMPILING)
    add_executable(island_audio_report tools/AudioReport.cpp)
    set_target_properties(island_audio_report PROPERTIES OUTPUT_NAME island-audio-report)
    target_link_libraries(island_audio_report PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_audio_report)
    island_enable_first_party_sanitizers(island_audio_report)

    add_executable(island_world_report tools/WorldReport.cpp)
    set_target_properties(island_world_report PROPERTIES OUTPUT_NAME island-world-report)
    target_link_libraries(island_world_report PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_world_report)
    island_enable_first_party_sanitizers(island_world_report)
endif()

if(ISLAND_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_executable(island_core_tests tests/CoreTests.cpp)
    target_link_libraries(island_core_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_core_tests)
    island_enable_first_party_sanitizers(island_core_tests)
    add_test(NAME IslandCoreTests COMMAND island_core_tests)

    if(TARGET island_world_report)
        add_test(NAME IslandWorldReport
            COMMAND island_world_report
                    --output "${CMAKE_BINARY_DIR}/island-world-report.json")
        set_tests_properties(IslandWorldReport PROPERTIES
            PASS_REGULAR_EXPRESSION "world report valid")
    endif()

    if(TARGET island_audio_report)
        add_test(NAME IslandAudioReport
            COMMAND island_audio_report
                    --output "${CMAKE_BINARY_DIR}/island-audio-report.json")
        set_tests_properties(IslandAudioReport PROPERTIES
            PASS_REGULAR_EXPRESSION "audio report valid")
    endif()

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME IslandAssetValidation
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_assets.py")
        set_tests_properties(IslandAssetValidation PROPERTIES
            PASS_REGULAR_EXPRESSION "asset validation passed")
        add_test(NAME IslandCaptureValidatorTests
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateCaptureTests.py")
        add_test(NAME IslandWebPackageValidation
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateWebPackageTests.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/package_web_release.sh")
        set_tests_properties(IslandWebPackageValidation PROPERTIES
            PASS_REGULAR_EXPRESSION "web package validation passed")
        if(TARGET island_audio_report)
            add_test(NAME IslandAudioPreviewExport
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateAudioPreviewTests.py"
                        "$<TARGET_FILE:island_audio_report>")
            set_tests_properties(IslandAudioPreviewExport PROPERTIES
                PASS_REGULAR_EXPRESSION "audio preview export validation passed")
        endif()
    endif()
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
    # Portable multi-language ShaderEffect packages are an opt-in CNA extension. Terrain, ocean,
    # palms, and grass keep explicit GLSL Desktop and GLSL ES variants for Linux, WebGL 2, and
    # GLES 3 parity.
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

    set(_island_game_sources
        game/src/BirdRenderer.cpp
        game/src/ContactShadowRenderer.cpp
        game/src/DebugOverlay.cpp
        game/src/DistantSceneryRenderer.cpp
        game/src/FishRenderer.cpp
        game/src/GrassRenderer.cpp
        game/src/IslandGame.cpp
        game/src/JungleRenderer.cpp
        game/src/Main.cpp
        game/src/MonkeyRenderer.cpp
        game/src/NaturalPropRenderer.cpp
        game/src/OceanRenderer.cpp
        game/src/PalmRenderer.cpp
        game/src/RuntimeDiagnostics.cpp
        game/src/SharkRenderer.cpp
        game/src/ShorelineRenderer.cpp
        game/src/SkyRenderer.cpp
        game/src/Soundscape.cpp
        game/src/TerrainRenderer.cpp
        game/src/TouchOverlay.cpp
        game/src/WaterViewOverlay.cpp
        game/src/WorldDebugRenderer.cpp)

    # SDLActivity loads an Android game as libmain.so. Keep the public CMake target name stable so
    # presets and developer tooling build `island-simulator` on every platform.
    if(ANDROID)
        add_library(island-simulator SHARED ${_island_game_sources})
        set_target_properties(island-simulator PROPERTIES OUTPUT_NAME main)
    else()
        add_executable(island-simulator ${_island_game_sources})
    endif()
    target_include_directories(island-simulator PRIVATE game/include)
    target_link_libraries(island-simulator PRIVATE IslandSimulator::Core CNA)
    island_enable_cna_consumer_warnings(island-simulator)
    island_enable_first_party_sanitizers(island-simulator)

    if(COMMAND cna_copy_renderer_runtime)
        cna_copy_renderer_runtime(island-simulator)
    endif()
    if(TARGET SDL3::SDL3main)
        target_link_libraries(island-simulator PRIVATE SDL3::SDL3main)
    endif()

    if(ANDROID AND TARGET SDL3::SDL3)
        target_link_libraries(island-simulator PRIVATE SDL3::SDL3 log)
    endif()

    if(EMSCRIPTEN)
        if(TARGET CNA::EmscriptenAsyncify)
            target_link_libraries(island-simulator PRIVATE CNA::EmscriptenAsyncify)
        endif()
        set_property(TARGET island-simulator APPEND PROPERTY LINK_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/web_shell.html")
        set_target_properties(island-simulator PROPERTIES SUFFIX ".html")
        target_link_options(island-simulator PRIVATE
            --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/tools/web_shell.html"
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
        set_tests_properties(IslandGameSmoke PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "fixed sunny sky with layered horizontal high wisps, portable shader")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/isolated-runtime-cwd")
        add_test(NAME IslandGameRelocatableWorkingDirectory
            COMMAND island-simulator --smoke-test --overview)
        set_tests_properties(IslandGameRelocatableWorkingDirectory PROPERTIES
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/isolated-runtime-cwd"
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "island-simulator: smoke test rendered 4 frames")
        add_test(NAME IslandGameStartupReport
            COMMAND island-simulator --smoke-test --overview
                    --startup-report "${CMAKE_BINARY_DIR}/island-startup-report.json")
        set_tests_properties(IslandGameStartupReport PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION "startup report .* first draw recorded")
        if(Python3_Interpreter_FOUND)
            add_test(NAME IslandCommandLineValidation
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateCommandLineTests.py"
                        "$<TARGET_FILE:island-simulator>")
            set_tests_properties(IslandCommandLineValidation PROPERTIES
                TIMEOUT 15
                PASS_REGULAR_EXPRESSION
                    "command-line validation passed [(]8 invalid invocations[)]")

            add_test(NAME IslandGameStartupReportValidation
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateStartupReportTests.py"
                        "${CMAKE_BINARY_DIR}/island-startup-report.json")
            set_tests_properties(IslandGameStartupReportValidation PROPERTIES
                DEPENDS IslandGameStartupReport
                PASS_REGULAR_EXPRESSION "startup report validation passed")
        endif()
        add_test(NAME IslandGameCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-capture.ppm")
        set_tests_properties(IslandGameCapture PROPERTIES TIMEOUT 30 LABELS display)
        if(Python3_Interpreter_FOUND)
            add_test(NAME IslandGameCaptureValidation
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_capture.py"
                        "${CMAKE_BINARY_DIR}/island-capture.ppm"
                        --expect-width 1280
                        --expect-height 720
                        --output "${CMAKE_BINARY_DIR}/island-capture-report.json")
            set_tests_properties(IslandGameCaptureValidation PROPERTIES
                DEPENDS IslandGameCapture
                PASS_REGULAR_EXPRESSION "capture validation passed")
        endif()
        add_test(NAME IslandGameBeachCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-beach-capture.ppm"
                    --viewpoint beach)
        set_tests_properties(IslandGameBeachCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "terrain authored primary footpath material")
        add_test(NAME IslandGameJungleCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-jungle-capture.ppm"
                    --viewpoint jungle)
        set_tests_properties(IslandGameJungleCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "jungle fixed-sun two-sided geometric facet lighting")
        add_test(NAME IslandGameEastClearingCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-east-clearing.ppm"
                    --viewpoint east)
        set_tests_properties(IslandGameEastClearingCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "terrain distinct authored east/west clearing materials")
        add_test(NAME IslandGameWestClearingCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-west-clearing.ppm"
                    --viewpoint west)
        set_tests_properties(IslandGameWestClearingCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "clearing landmarks 5 rocks and 2 fallen logs, shared authored placement")
        add_test(NAME IslandGameGroundPlantCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-ground-capture.ppm"
                    --viewpoint ground)
        set_tests_properties(IslandGameGroundPlantCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "ground QA per-instance near LOD [1-9][0-9]* plants")
        add_test(NAME IslandGameGrassCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-grass-capture.ppm"
                    --viewpoint grass)
        set_tests_properties(IslandGameGrassCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "grass [1-9][0-9]*/18556 patches in [1-9][0-9]* cells, [1-9][0-9]* draws, interaction 1, custom shader 1, biome tint 1, LOD near [1-9][0-9]*, middle [1-9][0-9]*, far [1-9][0-9]*, mixed cells [1-9][0-9]*")
        add_test(NAME IslandGamePalmCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-palm-capture.ppm"
                    --viewpoint palms)
        set_tests_properties(IslandGamePalmCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "palm near root wedges ground trunk bases")
        add_test(NAME IslandGameWaterViewCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-water-capture.ppm"
                    --viewpoint water)
        set_tests_properties(IslandGameWaterViewCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "water view 0[.][1-9][0-9]*, 1 draw, player ripple 0[.][1-9][0-9]*, phase [01]([.][0-9]+)?, splash 0[.][1-9][0-9]*, phase 0[.][0-9]+")
        add_test(NAME IslandGameCurrentViewCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-current-capture.ppm"
                    --viewpoint current)
        set_tests_properties(IslandGameCurrentViewCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "water view 0[.][1-9][0-9]*, 1 draw, player ripple [01]([.][0-9]+)?, phase [01]([.][0-9]+)?, current 0[.][1-9][0-9]*, undertow target 0[.][1-9][0-9]*")
        add_test(NAME IslandGameDistantSceneryCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-distant-capture.ppm"
                    --viewpoint distant)
        set_tests_properties(IslandGameDistantSceneryCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION "distant 5 islets, 1 draw")
        add_test(NAME IslandGamePropCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-prop-capture.ppm"
                    --viewpoint props)
        set_tests_properties(IslandGamePropCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "contact shadows [1-9][0-9]*, [1-9][0-9]* triangles, 1 draw")
        add_test(NAME IslandGameDriftwoodCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-driftwood-capture.ppm"
                    --viewpoint driftwood)
        set_tests_properties(IslandGameDriftwoodCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "driftwood QA per-instance near LOD [1-9][0-9]* pieces")
        add_test(NAME IslandGameFallenFrondCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-frond-capture.ppm"
                    --viewpoint fronds)
        set_tests_properties(IslandGameFallenFrondCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION "[1-9][0-9]*/63 fallen fronds")
        add_test(NAME IslandGameReefCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-reef-capture.ppm"
                    --viewpoint reef)
        set_tests_properties(IslandGameReefCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION "[1-9][0-9]*/[1-9][0-9]* reef outcrops")
        add_test(NAME IslandGameBirdCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-bird-capture.ppm"
                    --viewpoint birds)
        set_tests_properties(IslandGameBirdCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "bird flight uses slow radial drift, banking, and intermittent glide")
        add_test(NAME IslandGameFishCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-fish-capture.ppm"
                    --viewpoint fish)
        set_tests_properties(IslandGameFishCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "fish near/far geometry 51/12 vertices, single-surface fins and CPU faceted daylight")
        add_test(NAME IslandGameSharkCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-shark-capture.ppm"
                    --viewpoint shark)
        set_tests_properties(IslandGameSharkCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "shark near/far geometry 51/15 vertices, single-surface fins and CPU faceted daylight")
        add_test(NAME IslandGameMonkeyCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-monkey-capture.ppm"
                    --viewpoint monkeys)
        set_tests_properties(IslandGameMonkeyCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "calm monkeys use bounded skeletal head attention.*monkey crossfades 0/[1-9][0-9]* active/completed")
        add_test(NAME IslandGameDebugOverlayCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-debug-overlay.ppm"
                    --viewpoint beach --debug-overlay)
        set_tests_properties(IslandGameDebugOverlayCapture PROPERTIES TIMEOUT 30 LABELS display)
        add_test(NAME IslandGameWorldDebugCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-world-debug.ppm"
                    --viewpoint overview --world-debug)
        set_tests_properties(IslandGameWorldDebugCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "world debug frame ground samples 9, supports [1-9].*audio zones 6, candidates [1-2]")
        add_test(NAME IslandGameGroundContactCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-ground-contact.ppm"
                    --viewpoint contact --world-debug)
        set_tests_properties(IslandGameGroundContactCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "world debug frame ground samples 9, supports [1-9]")
        add_test(NAME IslandGameMovementTraceCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-movement-trace.ppm"
                    --viewpoint movement --world-debug)
        set_tests_properties(IslandGameMovementTraceCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "movement obstacle/terrain/slide/clamp [1-9][0-9]*/0/[1-9][0-9]*/0")
        add_test(NAME IslandGameWildlifeNavigationCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-wildlife-navigation.ppm"
                    --viewpoint monkeys --world-debug)
        set_tests_properties(IslandGameWildlifeNavigationCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "wildlife markers [1-9][0-9]*, targets [1-9][0-9]*, homes [1-7]; bird routes 3, spawns 24, radial targets [1-9][0-9]*")
        add_test(NAME IslandGameBenchmark
            COMMAND island-simulator
                    --benchmark "${CMAKE_BINARY_DIR}/island-benchmark-jungle.json"
                    --viewpoint jungle)
        set_tests_properties(IslandGameBenchmark PROPERTIES
            TIMEOUT 45
            LABELS display
            PASS_REGULAR_EXPRESSION "benchmark completed 600 samples")
        add_test(NAME IslandGameTraversalBenchmark
            COMMAND island-simulator
                    --benchmark "${CMAKE_BINARY_DIR}/island-benchmark-traversal.json"
                    --viewpoint traversal)
        set_tests_properties(IslandGameTraversalBenchmark PROPERTIES
            TIMEOUT 45
            LABELS display
            PASS_REGULAR_EXPRESSION "benchmark traversal rail .* benchmark completed 600 samples")

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            find_program(ISLAND_XDOTOOL_EXECUTABLE xdotool)
            if(ISLAND_XDOTOOL_EXECUTABLE)
                add_test(NAME IslandGameFreeCameraInput
                    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_native_free_camera_smoke.sh"
                            "$<TARGET_FILE:island-simulator>"
                            "${ISLAND_XDOTOOL_EXECUTABLE}")
                set_tests_properties(IslandGameFreeCameraInput PROPERTIES
                    TIMEOUT 20
                    LABELS "display;input"
                    RUN_SERIAL TRUE
                    PASS_REGULAR_EXPRESSION "free-camera input smoke passed")

                add_test(NAME IslandGamePlayerInput
                    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_native_player_input_smoke.sh"
                            "$<TARGET_FILE:island-simulator>"
                            "${ISLAND_XDOTOOL_EXECUTABLE}")
                set_tests_properties(IslandGamePlayerInput PROPERTIES
                    TIMEOUT 30
                    LABELS "display;input"
                    RUN_SERIAL TRUE
                    PASS_REGULAR_EXPRESSION "player input smoke passed")
            endif()
        endif()
    endif()
endif()
