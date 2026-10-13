cmake_minimum_required(VERSION 3.23)

project(IslandSimulator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Record the actual configuration in runtime/CPU evidence; Debug measurements must
# not be mistaken for the optimized player build. Limit the definition to reporters.
set_source_files_properties(game/src/IslandGame.cpp tools/WildlifeCpuReport.cpp PROPERTIES
    COMPILE_DEFINITIONS "ISLAND_BUILD_CONFIGURATION=\"$<CONFIG>\"")

option(ISLAND_BUILD_GAME "Build the graphical island-simulator application" ON)
option(ISLAND_BUILD_TESTS "Build island-simulator core tests" ON)
option(ISLAND_BUILD_TOOLS "Build island-simulator host development tools" ON)
option(ISLAND_STRICT_DEPENDENCY_REVISIONS
       "Require the dependency revisions recorded in dependencies.lock" ON)
option(ISLAND_ENABLE_SANITIZERS
       "Instrument first-party island-simulator targets with AddressSanitizer and UBSan" OFF)
option(ISLAND_ALLOW_PARALLEL_GPU_TESTS
       "Permit display tests to share a GPU (incident qualification only)" OFF)

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
    game/src/BearMotion.cpp
    game/src/BirdFlock.cpp
    game/src/BirdPerchLayout.cpp
    game/src/BoarHerd.cpp
    game/src/SnakePopulation.cpp
    game/src/SnakeContactShadow.cpp
    game/src/BoarCoatTexture.cpp
    game/src/BoarPose.cpp
    game/src/BoarGroundPose.cpp
    game/src/ButterflyMeadow.cpp
    game/src/CanopyLightField.cpp
    game/src/CanopyProfile.cpp
    game/src/CrabColony.cpp
    game/src/CaveLayout.cpp
    game/src/CaveGroundPatch.cpp
    game/src/CampfireLayout.cpp
    game/src/ResidentRoute.cpp
    game/src/ResidentMotion.cpp
    game/src/DebugFreeCamera.cpp
    game/src/DistantSceneryLayout.cpp
    game/src/ContactShadowPatch.cpp
    game/src/FirstPersonCameraMotion.cpp
    game/src/FirstPersonInput.cpp
    game/src/FlightMotion.cpp
    game/src/FishSchools.cpp
    game/src/ForestFloorSampling.cpp
    game/src/ForestFloorPixels.cpp
    game/src/GrassInteractionModel.cpp
    game/src/GrassBladeProfile.cpp
    game/src/GrassMotionBounds.cpp
    game/src/GrassLayout.cpp
    game/src/GroundCoverField.cpp
    game/src/GrassLod.cpp
    game/src/GroundBroadleaf.cpp
    game/src/SplitGroundLeaf.cpp
    game/src/BushProfile.cpp
    game/src/IslandTerrain.cpp
    game/src/HutLayout.cpp
    game/src/HelicopterLayout.cpp
    game/src/GoatHabitat.cpp
    game/src/SpiderHabitat.cpp
    game/src/SpiderPose.cpp
    game/src/SpiderMesh.cpp
    game/src/SpiderColony.cpp
    game/src/SpiderContactShadow.cpp
    game/src/CanopyBranchProfile.cpp
    game/src/SpiderWebLayout.cpp
    game/src/SpiderWebGeometry.cpp
    game/src/SpiderWebClearance.cpp
    game/src/GoatPose.cpp
    game/src/GoatHerd.cpp
    game/src/GoatMesh.cpp
    game/src/JungleLayout.cpp
    game/src/LeafBladeProfile.cpp
    game/src/UnderstoryLeafProfile.cpp
    game/src/LocomotionPaceToggle.cpp
    game/src/MonkeyTroop.cpp
    game/src/NaturalPropLayout.cpp
    game/src/OceanWaveSpectrum.cpp
    game/src/PerformanceStatistics.cpp
    game/src/PlayerMotion.cpp
    game/src/ProceduralMeshValidation.cpp
    game/src/RockSurface.cpp
    game/src/RenderedTerrainSurface.cpp
    game/src/SurfaceNormals.cpp
    game/src/SeaTurtleGroup.cpp
    game/src/DolphinPod.cpp
    game/src/SharkPatrol.cpp
    game/src/ShipwreckLayout.cpp
    game/src/SoundscapeModel.cpp
    game/src/TouchControls.cpp
    game/src/TidePoolLayout.cpp
    game/src/UserSettings.cpp
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
    add_executable(island_wildlife_cpu_report tools/WildlifeCpuReport.cpp)
    set_target_properties(island_wildlife_cpu_report PROPERTIES OUTPUT_NAME island-wildlife-cpu-report)
    target_link_libraries(island_wildlife_cpu_report PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_wildlife_cpu_report)
    island_enable_first_party_sanitizers(island_wildlife_cpu_report)
    add_executable(island_stream_survey tools/StreamSurvey.cpp)
    set_target_properties(island_stream_survey PROPERTIES OUTPUT_NAME island-stream-survey)
    target_link_libraries(island_stream_survey PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_stream_survey)
    island_enable_first_party_sanitizers(island_stream_survey)
    if(ISLAND_BUILD_TESTS)
        add_test(NAME IslandStreamCourseSurvey COMMAND island_stream_survey)
    endif()
    add_executable(island_goat_stance_audit tools/GoatStanceAudit.cpp)
    set_target_properties(island_goat_stance_audit PROPERTIES OUTPUT_NAME island-goat-stance-audit)
    target_link_libraries(island_goat_stance_audit PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_goat_stance_audit)
    island_enable_first_party_sanitizers(island_goat_stance_audit)
    add_executable(island_goat_mesh_preview tools/GoatMeshPreview.cpp)
    add_executable(island_spider_mesh_preview tools/SpiderMeshPreview.cpp)
    set_target_properties(island_spider_mesh_preview PROPERTIES OUTPUT_NAME island-spider-mesh-preview)
    target_link_libraries(island_spider_mesh_preview PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_spider_mesh_preview)
    island_enable_first_party_sanitizers(island_spider_mesh_preview)
    set_target_properties(island_goat_mesh_preview PROPERTIES OUTPUT_NAME island-goat-mesh-preview)
    target_link_libraries(island_goat_mesh_preview PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_goat_mesh_preview)
    island_enable_first_party_sanitizers(island_goat_mesh_preview)
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

    add_executable(island_grass_grounding_report tools/GrassGroundingReport.cpp)
    set_target_properties(island_grass_grounding_report PROPERTIES OUTPUT_NAME island-grass-grounding-report)
    target_link_libraries(island_grass_grounding_report PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_grass_grounding_report)
    island_enable_first_party_sanitizers(island_grass_grounding_report)
    add_executable(island_forest_floor_preview tools/ForestFloorPreview.cpp)
    set_target_properties(island_forest_floor_preview PROPERTIES OUTPUT_NAME island-forest-floor-preview)
    target_link_libraries(island_forest_floor_preview PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_forest_floor_preview)
    island_enable_first_party_sanitizers(island_forest_floor_preview)
endif()

if(ISLAND_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_executable(island_flight_tests tests/FlightMotionTests.cpp)
    target_link_libraries(island_flight_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_flight_tests)
    island_enable_first_party_sanitizers(island_flight_tests)
    add_test(NAME IslandFlightMotionTests COMMAND island_flight_tests)
    add_executable(island_diagnostic_stream_tests tests/DiagnosticStreamMirrorTests.cpp)
    target_include_directories(island_diagnostic_stream_tests PRIVATE game/include)
    target_compile_features(island_diagnostic_stream_tests PRIVATE cxx_std_20)
    island_enable_first_party_warnings(island_diagnostic_stream_tests)
    island_enable_first_party_sanitizers(island_diagnostic_stream_tests)
    add_test(NAME IslandDiagnosticStreamMirrorTests COMMAND island_diagnostic_stream_tests)
    add_executable(island_monkey_shader_tests tests/MonkeySkinningShaderTests.cpp)
    target_include_directories(island_monkey_shader_tests PRIVATE game/include)
    target_link_libraries(island_monkey_shader_tests PRIVATE IslandSimulator::Core)
    target_compile_features(island_monkey_shader_tests PRIVATE cxx_std_20)
    island_enable_first_party_warnings(island_monkey_shader_tests)
    island_enable_first_party_sanitizers(island_monkey_shader_tests)
    add_test(NAME IslandMonkeySkinningShaderTests COMMAND island_monkey_shader_tests)
    add_executable(island_monkey_gait_tests tests/MonkeyGroundGaitTests.cpp)
    target_include_directories(island_monkey_gait_tests PRIVATE game/include)
    target_compile_features(island_monkey_gait_tests PRIVATE cxx_std_20)
    island_enable_first_party_warnings(island_monkey_gait_tests)
    island_enable_first_party_sanitizers(island_monkey_gait_tests)
    add_test(NAME IslandMonkeyGroundGaitTests COMMAND island_monkey_gait_tests)
    add_executable(island_core_tests tests/CoreTests.cpp)
    target_link_libraries(island_core_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_core_tests)
    island_enable_first_party_sanitizers(island_core_tests)
    add_test(NAME IslandCoreTests COMMAND island_core_tests)
    add_executable(island_dolphin_tests tests/DolphinPodTests.cpp)
    target_link_libraries(island_dolphin_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_dolphin_tests)
    island_enable_first_party_sanitizers(island_dolphin_tests)
    add_test(NAME IslandDolphinPodTests COMMAND island_dolphin_tests)
    add_executable(island_boar_tests tests/BoarHerdTests.cpp)
    target_link_libraries(island_boar_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_boar_tests)
    island_enable_first_party_sanitizers(island_boar_tests)
    add_test(NAME IslandBoarHerdTests COMMAND island_boar_tests)
    add_executable(island_goat_habitat_tests tests/GoatHabitatTests.cpp)
    add_executable(island_spider_habitat_tests tests/SpiderHabitatTests.cpp)
    target_link_libraries(island_spider_habitat_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_spider_habitat_tests)
    island_enable_first_party_sanitizers(island_spider_habitat_tests)
    add_test(NAME IslandSpiderHabitatTests COMMAND island_spider_habitat_tests)
    add_executable(island_spider_pose_tests tests/SpiderPoseTests.cpp)
    target_link_libraries(island_spider_pose_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_spider_pose_tests)
    island_enable_first_party_sanitizers(island_spider_pose_tests)
    add_test(NAME IslandSpiderPoseTests COMMAND island_spider_pose_tests)
    add_executable(island_spider_mesh_tests tests/SpiderMeshTests.cpp)
    target_link_libraries(island_spider_mesh_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_spider_mesh_tests)
    island_enable_first_party_sanitizers(island_spider_mesh_tests)
    add_test(NAME IslandSpiderMeshTests COMMAND island_spider_mesh_tests)
    add_executable(island_branch_anchor_tests tests/BranchAnchorTests.cpp)
    target_link_libraries(island_branch_anchor_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_branch_anchor_tests)
    island_enable_first_party_sanitizers(island_branch_anchor_tests)
    add_test(NAME IslandBranchAnchorTests COMMAND island_branch_anchor_tests)
    add_executable(island_web_layout_tests tests/SpiderWebLayoutTests.cpp)
    target_link_libraries(island_web_layout_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_web_layout_tests)
    island_enable_first_party_sanitizers(island_web_layout_tests)
    add_test(NAME IslandSpiderWebLayoutTests COMMAND island_web_layout_tests)
    add_executable(island_web_geometry_tests tests/SpiderWebGeometryTests.cpp)
    target_link_libraries(island_web_geometry_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_web_geometry_tests)
    island_enable_first_party_sanitizers(island_web_geometry_tests)
    add_test(NAME IslandSpiderWebGeometryTests COMMAND island_web_geometry_tests)
    add_executable(island_spider_colony_tests tests/SpiderColonyTests.cpp)
    target_link_libraries(island_spider_colony_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_spider_colony_tests)
    island_enable_first_party_sanitizers(island_spider_colony_tests)
    add_test(NAME IslandSpiderColonyTests COMMAND island_spider_colony_tests)
    target_link_libraries(island_goat_habitat_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_goat_habitat_tests)
    island_enable_first_party_sanitizers(island_goat_habitat_tests)
    add_test(NAME IslandGoatHabitatTests COMMAND island_goat_habitat_tests)
    add_executable(island_goat_herd_tests tests/GoatHerdTests.cpp)
    target_link_libraries(island_goat_herd_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_goat_herd_tests)
    island_enable_first_party_sanitizers(island_goat_herd_tests)
    add_test(NAME IslandGoatHerdTests COMMAND island_goat_herd_tests)
    add_executable(island_goat_mesh_tests tests/GoatMeshTests.cpp)
    target_link_libraries(island_goat_mesh_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_goat_mesh_tests)
    island_enable_first_party_sanitizers(island_goat_mesh_tests)
    add_test(NAME IslandGoatMeshTests COMMAND island_goat_mesh_tests)
    add_executable(island_snake_tests tests/SnakePopulationTests.cpp)
    target_link_libraries(island_snake_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_snake_tests)
    island_enable_first_party_sanitizers(island_snake_tests)
    add_test(NAME IslandSnakePopulationTests COMMAND island_snake_tests)
    add_executable(island_hut_tests tests/HutLayoutTests.cpp)
    target_link_libraries(island_hut_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_hut_tests)
    island_enable_first_party_sanitizers(island_hut_tests)
    add_test(NAME IslandHutLayoutTests COMMAND island_hut_tests)
    add_executable(island_helicopter_tests tests/HelicopterLayoutTests.cpp)
    target_link_libraries(island_helicopter_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_helicopter_tests)
    island_enable_first_party_sanitizers(island_helicopter_tests)
    add_test(NAME IslandHelicopterLayoutTests COMMAND island_helicopter_tests)
    add_executable(island_cave_tests tests/CaveLayoutTests.cpp)
    target_link_libraries(island_cave_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_cave_tests)
    island_enable_first_party_sanitizers(island_cave_tests)
    add_test(NAME IslandCaveLayoutTests COMMAND island_cave_tests)
    add_executable(island_campfire_tests tests/CampfireTests.cpp)
    target_link_libraries(island_campfire_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_campfire_tests)
    island_enable_first_party_sanitizers(island_campfire_tests)
    add_test(NAME IslandCampfireTests COMMAND island_campfire_tests)
    add_executable(island_resident_tests tests/ResidentTests.cpp)
    target_link_libraries(island_resident_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_resident_tests)
    island_enable_first_party_sanitizers(island_resident_tests)
    add_test(NAME IslandResidentTests COMMAND island_resident_tests)
    add_executable(island_bear_motion_tests tests/BearMotionTests.cpp)
    target_link_libraries(island_bear_motion_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_bear_motion_tests)
    island_enable_first_party_sanitizers(island_bear_motion_tests)
    add_test(NAME IslandBearMotionTests COMMAND island_bear_motion_tests)
    add_executable(island_boar_coat_tests tests/BoarCoatTextureTests.cpp)
    target_link_libraries(island_boar_coat_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_boar_coat_tests)
    island_enable_first_party_sanitizers(island_boar_coat_tests)
    add_test(NAME IslandBoarCoatTextureTests COMMAND island_boar_coat_tests)
    add_executable(island_boar_pose_tests tests/BoarPoseTests.cpp)
    target_link_libraries(island_boar_pose_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_boar_pose_tests)
    island_enable_first_party_sanitizers(island_boar_pose_tests)
    add_test(NAME IslandBoarPoseTests COMMAND island_boar_pose_tests)
    add_executable(island_leaf_tests tests/LeafBladeProfileTests.cpp tests/GroundBroadleafTests.cpp
        tests/SplitGroundLeafTests.cpp tests/BushProfileTests.cpp)
    target_link_libraries(island_leaf_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_leaf_tests)
    island_enable_first_party_sanitizers(island_leaf_tests)
    add_test(NAME IslandLeafBladeTests COMMAND island_leaf_tests)
    add_executable(island_grass_blade_tests tests/GrassBladeProfileTests.cpp
        tests/GrassMotionBoundsTests.cpp tests/GrassVisibilityTests.cpp)
    target_link_libraries(island_grass_blade_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_grass_blade_tests)
    island_enable_first_party_sanitizers(island_grass_blade_tests)
    add_test(NAME IslandGrassBladeTests COMMAND island_grass_blade_tests)

    add_executable(island_rendered_terrain_tests tests/RenderedTerrainSurfaceTests.cpp)
    target_link_libraries(island_rendered_terrain_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_rendered_terrain_tests)
    island_enable_first_party_sanitizers(island_rendered_terrain_tests)
    add_test(NAME IslandRenderedTerrainTests COMMAND island_rendered_terrain_tests)
    add_executable(island_canopy_tests tests/CanopyLightFieldTests.cpp)
    target_link_libraries(island_canopy_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_canopy_tests)
    island_enable_first_party_sanitizers(island_canopy_tests)
    add_test(NAME IslandCanopyLightFieldTests COMMAND island_canopy_tests)
    add_executable(island_ocean_wave_tests tests/OceanWaveSpectrumTests.cpp)
    target_link_libraries(island_ocean_wave_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_ocean_wave_tests)
    island_enable_first_party_sanitizers(island_ocean_wave_tests)
    add_test(NAME IslandOceanWaveSpectrumTests COMMAND island_ocean_wave_tests)
    add_executable(island_forest_floor_tests tests/ForestFloorSamplingTests.cpp)
    target_link_libraries(island_forest_floor_tests PRIVATE IslandSimulator::Core)
    island_enable_first_party_warnings(island_forest_floor_tests)
    island_enable_first_party_sanitizers(island_forest_floor_tests)
    add_test(NAME IslandForestFloorSamplingTests COMMAND island_forest_floor_tests)

    find_program(ISLAND_NODE_EXECUTABLE NAMES node nodejs)
    if(ISLAND_NODE_EXECUTABLE)
        add_test(NAME IslandBrowserFullscreenTests
            COMMAND "${ISLAND_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/BrowserFullscreenTests.mjs")
        add_test(NAME IslandGrassMotionReviewTests
            COMMAND "${ISLAND_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/GrassMotionReviewTests.mjs")
    endif()

    if(TARGET island_world_report)
        add_test(NAME IslandWorldReport
            COMMAND island_world_report
                    --output "${CMAKE_BINARY_DIR}/island-world-report.json")
        set_tests_properties(IslandWorldReport PROPERTIES
            PASS_REGULAR_EXPRESSION "world report valid")
    endif()

    if(TARGET island_grass_grounding_report)
        add_test(NAME IslandGrassGroundingReport
            COMMAND island_grass_grounding_report "${CMAKE_BINARY_DIR}/island-grass-grounding-report.json")
        set_tests_properties(IslandGrassGroundingReport PROPERTIES
            PASS_REGULAR_EXPRESSION "grass grounding report written")
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
        add_test(NAME IslandPresetCoverageTests
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/PresetCoverageTests.py")
        if(TARGET island_world_report)
            add_test(NAME IslandWorldSpiderReportTests
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/WorldSpiderReportTests.py"
                    "$<TARGET_FILE:island_world_report>")
            add_test(NAME IslandWorldGoatReportTests
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/WorldGoatReportTests.py"
                    "$<TARGET_FILE:island_world_report>")
            add_test(NAME IslandWorldSnakeReportTests
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/WorldSnakeReportTests.py"
                    "$<TARGET_FILE:island_world_report>")
        endif()
        add_test(NAME IslandAssetValidation
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_assets.py")
        set_tests_properties(IslandAssetValidation PROPERTIES
            PASS_REGULAR_EXPRESSION "asset validation passed")
        add_test(NAME IslandCaptureValidatorTests
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateCaptureTests.py")
        add_test(NAME IslandTextureTileReviewTests
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/TextureTileReviewTests.py")
        if(TARGET island_forest_floor_preview)
            add_test(NAME IslandForestFloorPreviewTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ForestFloorPreviewTests.py"
                        "$<TARGET_FILE:island_forest_floor_preview>")
            add_test(NAME IslandPreparedMaterialTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/PreparedMaterialTests.py"
                        "$<TARGET_FILE:island_forest_floor_preview>")
        endif()
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_test(NAME IslandGpuQualificationSupervisorTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ValidateGpuQualificationTests.py")
            set_tests_properties(IslandGpuQualificationSupervisorTests PROPERTIES TIMEOUT 30)
            add_test(NAME IslandGlVertexTraceAuditTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/AuditGlVertexTraceTests.py")
            add_test(NAME IslandGlTraceStateTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/InspectGlTraceStateTests.py")
            add_test(NAME IslandGlSkinTraceAuditTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/AuditGlSkinTraceTests.py")
            add_test(NAME IslandSoftwareCapturePreflightTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/SoftwareCaptureTests.py")
            add_test(NAME IslandAndroidSoftwarePreflightTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/AndroidSoftwareTests.py")
            add_test(NAME IslandBrowserSoftwarePreflightTests
                COMMAND "${Python3_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/BrowserSoftwareTests.py")
            set_tests_properties(IslandBrowserSoftwarePreflightTests PROPERTIES TIMEOUT 30)
        endif()
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
        game/src/CanopyLighting.cpp
        game/src/BearRenderer.cpp
        game/src/BirdRenderer.cpp
        game/src/ButterflyRenderer.cpp
        game/src/ContactShadowRenderer.cpp
        game/src/CrabRenderer.cpp
        game/src/BoarRenderer.cpp
        game/src/SnakeRenderer.cpp
        game/src/GoatRenderer.cpp
        game/src/SpiderRenderer.cpp
        game/src/SpiderWebRenderer.cpp
        game/src/DebugOverlay.cpp
        game/src/DistantSceneryRenderer.cpp
        game/src/DolphinRenderer.cpp
        game/src/FishRenderer.cpp
        game/src/ForestFloorMaterial.cpp
        game/src/OpaqueImageLoader.cpp
        game/src/LeafSurfaceMaterial.cpp
        game/src/GpuDiagnostics.cpp
        game/src/GrassRenderer.cpp
        game/src/IslandGame.cpp
        game/src/JungleRenderer.cpp
        game/src/Main.cpp
        game/src/MonkeyRenderer.cpp
        game/src/NaturalPropRenderer.cpp
        game/src/OceanRenderer.cpp
        game/src/StreamWaterRenderer.cpp
        game/src/CaveRenderer.cpp
        game/src/CampfireRenderer.cpp
        game/src/ResidentRenderer.cpp
        game/src/PalmRenderer.cpp
        game/src/RuntimeDiagnostics.cpp
        game/src/RenderedTerrainHeightMap.cpp
        game/src/SeaTurtleRenderer.cpp
        game/src/SharkRenderer.cpp
        game/src/ShipwreckRenderer.cpp
        game/src/HutRenderer.cpp
        game/src/HelicopterRenderer.cpp
        game/src/ShorelineRenderer.cpp
        game/src/SkyRenderer.cpp
        game/src/Soundscape.cpp
        game/src/TerrainRenderer.cpp
        game/src/TouchOverlay.cpp
        game/src/UserSettingsStore.cpp
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

    if(ISLAND_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
        add_executable(island_soundscape_runtime_tests tests/SoundscapeRuntimeTests.cpp
            game/src/Soundscape.cpp game/src/RuntimeDiagnostics.cpp)
        target_link_libraries(island_soundscape_runtime_tests PRIVATE IslandSimulator::Core CNA)
        island_enable_cna_consumer_warnings(island_soundscape_runtime_tests)
        island_enable_first_party_sanitizers(island_soundscape_runtime_tests)
        add_test(NAME IslandSoundscapeRuntimeTests COMMAND island_soundscape_runtime_tests)
        set_tests_properties(IslandSoundscapeRuntimeTests PROPERTIES TIMEOUT 45
            ENVIRONMENT "SDL_AUDIODRIVER=dummy;ISLAND_RUNTIME_LOG_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/audio-regression-logs")
    endif()

    # Host-only area reduction preserves source artwork and the runtime's exact base pixels.
    set(_island_floor_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/forest-floor-albedo-v1.png")
    set(_island_floor_path "Content/terrain/forest-floor.png")
    set(_island_leaf_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/leaf-lamina-v1.png")
    set(_island_leaf_path "Content/vegetation/leaf-lamina.png")
    set(_island_fur_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/monkey-fur-albedo-v2.png")
    set(_island_fur_path "Content/wildlife/monkey-fur.png")
    set(_island_thatch_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/hut-palm-fibre-v1.png")
    set(_island_thatch_path "Content/structures/hut-palm-fibre.png")
    set(_island_goat_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/goat-fur-albedo-v1.png")
    set(_island_spider_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/spider-setae-albedo-v1.png")
    set(_island_goat_path "Content/wildlife/goat-fur.png")
    set(_island_spider_path "Content/wildlife/spider-setae.png")
    set(_island_goat_prepared "${CMAKE_BINARY_DIR}/${_island_goat_path}")
    set(_island_spider_prepared "${CMAKE_BINARY_DIR}/${_island_spider_path}")
    set(_island_boar_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/boar-bristle-albedo-v2.png")
    set(_island_boar_path "Content/wildlife/boar-bristle.png")
    set(_island_boar_prepared "${CMAKE_BINARY_DIR}/${_island_boar_path}")
    set(_island_bear_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/bear-fur-albedo-v2.png")
    set(_island_bear_path "Content/wildlife/bear-fur.png")
    set(_island_bear_prepared "${CMAKE_BINARY_DIR}/${_island_bear_path}")
    set(_island_shipwreck_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/art-studies/shipwreck-weathered-wood-albedo-v1.png")
    set(_island_shipwreck_path "Content/structures/shipwreck-weathered-wood.png")
    set(_island_shipwreck_prepared "${CMAKE_BINARY_DIR}/${_island_shipwreck_path}")
    set(_island_thatch_prepared "${CMAKE_BINARY_DIR}/${_island_thatch_path}")
    set(_island_fur_prepared "${CMAKE_BINARY_DIR}/${_island_fur_path}")
    set(_island_floor_prepared "${CMAKE_BINARY_DIR}/${_island_floor_path}")
    set(_island_leaf_prepared "${CMAKE_BINARY_DIR}/${_island_leaf_path}")
    if(NOT ANDROID)
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        execute_process(COMMAND "${Python3_EXECUTABLE}" -c "from PIL import Image"
            RESULT_VARIABLE _island_pillow_result ERROR_QUIET)
        if(NOT _island_pillow_result EQUAL 0)
            message(FATAL_ERROR "Material preparation needs host Python Pillow (python3-pil).")
        endif()
        set(_island_material_tool "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_material_texture.py")
        add_custom_command(OUTPUT "${_island_floor_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_floor_source}" "${_island_floor_prepared}" --size 512
            DEPENDS "${_island_floor_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_leaf_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_leaf_source}" "${_island_leaf_prepared}" --size 256
            DEPENDS "${_island_leaf_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_fur_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_fur_source}" "${_island_fur_prepared}" --size 512
            DEPENDS "${_island_fur_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_thatch_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_thatch_source}" "${_island_thatch_prepared}" --size 256
            DEPENDS "${_island_thatch_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_goat_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_goat_source}" "${_island_goat_prepared}" --size 256
            DEPENDS "${_island_goat_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_spider_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_spider_source}" "${_island_spider_prepared}" --size 256
            DEPENDS "${_island_spider_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_boar_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_boar_source}" "${_island_boar_prepared}" --size 256
            DEPENDS "${_island_boar_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_bear_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_bear_source}" "${_island_bear_prepared}" --size 128
            DEPENDS "${_island_bear_source}" "${_island_material_tool}" VERBATIM)
        add_custom_command(OUTPUT "${_island_shipwreck_prepared}"
            COMMAND "${Python3_EXECUTABLE}" "${_island_material_tool}"
                "${_island_shipwreck_source}" "${_island_shipwreck_prepared}" --size 256
            DEPENDS "${_island_shipwreck_source}" "${_island_material_tool}" VERBATIM)
        add_custom_target(island_material_assets DEPENDS "${_island_floor_prepared}" "${_island_leaf_prepared}" "${_island_fur_prepared}" "${_island_thatch_prepared}" "${_island_goat_prepared}" "${_island_spider_prepared}" "${_island_boar_prepared}" "${_island_bear_prepared}" "${_island_shipwreck_prepared}")
        add_dependencies(island-simulator island_material_assets)
    endif()

    if(ISLAND_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
        add_executable(island_monkey_sole_tests tests/MonkeySoleFrameTests.cpp)
        target_include_directories(island_monkey_sole_tests PRIVATE game/include)
        target_link_libraries(island_monkey_sole_tests PRIVATE CNA)
        island_enable_cna_consumer_warnings(island_monkey_sole_tests)
        island_enable_first_party_sanitizers(island_monkey_sole_tests)
        add_test(NAME IslandMonkeySoleFrameTests COMMAND island_monkey_sole_tests)
        add_executable(island_canopy_visibility_tests tests/CanopyVisibilityTests.cpp)
        target_include_directories(island_canopy_visibility_tests PRIVATE game/include)
        target_link_libraries(island_canopy_visibility_tests PRIVATE CNA)
        island_enable_cna_consumer_warnings(island_canopy_visibility_tests)
        island_enable_first_party_sanitizers(island_canopy_visibility_tests)
        add_test(NAME IslandCanopyVisibilityTests COMMAND island_canopy_visibility_tests)
        add_executable(island_terrain_material_tests tests/TerrainMaterialTests.cpp)
        target_link_libraries(island_terrain_material_tests PRIVATE CNA IslandSimulator::Core)
        island_enable_cna_consumer_warnings(island_terrain_material_tests)
        island_enable_first_party_sanitizers(island_terrain_material_tests)
        add_test(NAME IslandTerrainMaterialTests COMMAND island_terrain_material_tests)
    endif()

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
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/browser_fullscreen.js"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/web_shell.html" "${_island_floor_prepared}" "${_island_leaf_prepared}" "${_island_fur_prepared}" "${_island_thatch_prepared}" "${_island_goat_prepared}" "${_island_spider_prepared}" "${_island_boar_prepared}" "${_island_bear_prepared}" "${_island_shipwreck_prepared}")
        set_target_properties(island-simulator PROPERTIES SUFFIX ".html")
        target_link_options(island-simulator PRIVATE
            --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/tools/web_shell.html"
            "SHELL:--pre-js '${CMAKE_CURRENT_SOURCE_DIR}/tools/browser_fullscreen.js'"
            "SHELL:--preload-file '${_island_floor_prepared}@/${_island_floor_path}'"
            "SHELL:--preload-file '${_island_leaf_prepared}@/${_island_leaf_path}'"
            "SHELL:--preload-file '${_island_fur_prepared}@/${_island_fur_path}'"
            "SHELL:--preload-file '${_island_thatch_prepared}@/${_island_thatch_path}'"
            "SHELL:--preload-file '${_island_goat_prepared}@/${_island_goat_path}'"
            "SHELL:--preload-file '${_island_spider_prepared}@/${_island_spider_path}'"
            "SHELL:--preload-file '${_island_boar_prepared}@/${_island_boar_path}'"
            "SHELL:--preload-file '${_island_bear_prepared}@/${_island_bear_path}'"
            "SHELL:--preload-file '${_island_shipwreck_prepared}@/${_island_shipwreck_path}'"
            -sALLOW_MEMORY_GROWTH=1
            -sSTACK_SIZE=1048576
            -sINITIAL_MEMORY=134217728
            -sFORCE_FILESYSTEM=1
            -sEXIT_RUNTIME=1
            -sMIN_WEBGL_VERSION=2
            -sMAX_WEBGL_VERSION=2)
    endif()

    # Keep the native release install self-contained with respect to the selected CNA runtime.
    # SDL 3 and SDL_mixer remain documented host runtime dependencies; copying arbitrary system
    # libraries would make the archive distribution-specific and obscure their own licences.
    if(NOT EMSCRIPTEN AND NOT ANDROID)
        set_target_properties(island-simulator PROPERTIES
            INSTALL_RPATH "$ORIGIN/lib")
        install(TARGETS island-simulator
            RUNTIME DESTINATION .
            COMPONENT Runtime)
        install(FILES "${_island_floor_prepared}" DESTINATION Content/terrain
            RENAME forest-floor.png COMPONENT Runtime)
        install(FILES "${_island_leaf_prepared}" DESTINATION Content/vegetation
            RENAME leaf-lamina.png COMPONENT Runtime)
        install(FILES "${_island_fur_prepared}" DESTINATION Content/wildlife
            RENAME monkey-fur.png COMPONENT Runtime)
        install(FILES "${_island_thatch_prepared}" DESTINATION Content/structures
            RENAME hut-palm-fibre.png COMPONENT Runtime)
        install(FILES "${_island_goat_prepared}" DESTINATION Content/wildlife
            RENAME goat-fur.png COMPONENT Runtime)
        install(FILES "${_island_spider_prepared}" DESTINATION Content/wildlife
            RENAME spider-setae.png COMPONENT Runtime)
        install(FILES "${_island_boar_prepared}" DESTINATION Content/wildlife
            RENAME boar-bristle.png COMPONENT Runtime)
        install(FILES "${_island_bear_prepared}" DESTINATION Content/wildlife
            RENAME bear-fur.png COMPONENT Runtime)
        install(FILES "${_island_shipwreck_prepared}" DESTINATION Content/structures
            RENAME shipwreck-weathered-wood.png COMPONENT Runtime)
        if(TARGET cna_shared)
            install(TARGETS cna_shared
                LIBRARY DESTINATION lib
                COMPONENT Runtime)
        endif()
        install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/ASSET_LICENSES.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock"
            DESTINATION share/island-simulator
            COMPONENT Runtime)
        install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/presentation/island-overview.png"
            DESTINATION share/island-simulator/assets/presentation
            COMPONENT Runtime)
        install(FILES "${CNA_ROOT_DIR}/LICENSE"
            DESTINATION share/island-simulator/licenses/CNA
            COMPONENT Runtime)
        install(FILES "${CNA_ROOT_DIR}/NOTICE.md"
            DESTINATION share/island-simulator/licenses/CNA
            COMPONENT Runtime)
        install(FILES "${ISLAND_SHARP_RUNTIME_ROOT}/LICENSE"
            DESTINATION share/island-simulator/licenses/Sharp-Runtime
            COMPONENT Runtime)
        install(FILES "${ISLAND_EASY_GL_ROOT}/LICENSE"
            DESTINATION share/island-simulator/licenses/EasyGL
            COMPONENT Runtime)
        install(FILES "${ISLAND_META_GL_ROOT}/LICENSE"
            DESTINATION share/island-simulator/licenses/MetaGL
            COMPONENT Runtime)
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
                "forest-floor material loaded Content/terrain/forest-floor.png; 512x512, 10 mips")
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
                    "command-line validation passed [(]26 invalid invocations[)]")

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
                "jungle near canopy uses irregular layered lower-crown edges.*jungle near canopy uses irregular trunks and bent support branches.*selected near canopy variants include hanging leafy lianas")
        add_test(NAME IslandGameEastClearingCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-east-clearing.ppm"
                    --viewpoint east)
        set_tests_properties(IslandGameEastClearingCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "clearing floors reuse existing noise fields for broad patch breakup")
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
                "palm near geometry uses fixed-sun facet shading.*selected palm near crowns include sparse hanging dry fronds")
        add_test(NAME IslandGameWaterViewCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-water-capture.ppm"
                    --viewpoint water)
        set_tests_properties(IslandGameWaterViewCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "water view 0[.][1-9][0-9]*, 1 draw, player ripple 0[.][1-9][0-9]*, phase [01]([.][0-9]+)?, splash 0[.][1-9][0-9]*, phase [01]([.][0-9]+)?")
        add_test(NAME IslandGameCurrentViewCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-current-capture.ppm"
                    --viewpoint current)
        set_tests_properties(IslandGameCurrentViewCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "water view 0[.][1-9][0-9]*, 1 draw, player ripple [01]([.][0-9]+)?, phase [01]([.][0-9]+)?, splash [01]([.][0-9]+)?, phase [01]([.][0-9]+)?, current 0[.][1-9][0-9]*, undertow target 0[.][1-9][0-9]*")
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
                "near fallen-log end grain uses restrained radial cracks")
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
            PASS_REGULAR_EXPRESSION
                "fallen frond leaflets use geometric central folds.*[1-9][0-9]*/63 fallen fronds")
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
                "fish near/far geometry 414/12 vertices, single-surface fins and CPU faceted daylight")
        add_test(NAME IslandGameSharkCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-shark-capture.ppm"
                    --viewpoint shark)
        set_tests_properties(IslandGameSharkCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "shark near/far geometry 594/15 vertices, single-surface fins and CPU faceted daylight")
        add_test(NAME IslandGameDolphinCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-dolphin-capture.ppm"
                    --viewpoint dolphins)
        set_tests_properties(IslandGameDolphinCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "dolphin near/far geometry 708/15 vertices, closed fins and CPU faceted daylight")
        add_test(NAME IslandGameBearCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-bear-capture.ppm"
                    --viewpoint bear)
        set_tests_properties(IslandGameBearCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "near bear uses a two-volume tapered muzzle, volumetric nose, paired nostrils, divided mouth crease, and highlighted eyes.*bear capture visible 1; draws 2")
        add_test(NAME IslandGameMonkeyCapture
            COMMAND island-simulator --capture "${CMAKE_BINARY_DIR}/island-monkey-capture.ppm"
                    --viewpoint monkeys)
        set_tests_properties(IslandGameMonkeyCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "calm monkeys use bounded skeletal head attention.*monkey crossfades 0/[1-9][0-9]* active/completed")
        add_test(NAME IslandGameShipwreckCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-shipwreck-capture.ppm"
                    --viewpoint shipwreck)
        set_tests_properties(IslandGameShipwreckCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "shipwreck portable aged wood, waterline, and salt-wear material.*shipwreck 20m broken hull, 12 debris pieces.*shipwreck large-ship silhouette uses deck rails, hull openings, keel, rudder, and a broken foremast.*shipwreck hull uses asymmetric rising sheer, curved keel, and irregular splintered break sections.*shipwreck visible, 12 debris, 1 draw")
        add_test(NAME IslandGameCrabCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-crab-capture.ppm"
                    --viewpoint crabs)
        set_tests_properties(IslandGameCrabCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "beach crabs use rounded carapaces, volumetric jointed legs, eye stalks, and one culled batch.*beach crabs 30 agents.*crabs [1-9][0-9]*/30 visible.*1 draw")
        add_test(NAME IslandGameButterflyCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-butterfly-capture.ppm"
                    --viewpoint butterflies)
        set_tests_properties(IslandGameButterflyCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "clearing butterflies use hinged patterned wings in one culled batch.*clearing butterflies 18 agents.*butterflies [1-9][0-9]*/18 visible.*1 draw")
        add_test(NAME IslandGameSeaTurtleCapture
            COMMAND island-simulator
                    --capture "${CMAKE_BINARY_DIR}/island-turtle-capture.ppm"
                    --viewpoint turtles)
        set_tests_properties(IslandGameSeaTurtleCapture PROPERTIES
            TIMEOUT 30
            LABELS display
            PASS_REGULAR_EXPRESSION
                "sea turtles use faceted shells and articulated flippers in one culled batch.*sea turtles 5 agents.*turtles [1-9]/5 visible.*1 draw")
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
                "movement depenetrate/obstacle/terrain/slide/clamp [0-9]+/[1-9][0-9]*/0/[1-9][0-9]*/0")
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

                add_test(NAME IslandGameSettingsPersistence
                    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_native_settings_smoke.sh"
                            "$<TARGET_FILE:island-simulator>"
                            "${ISLAND_XDOTOOL_EXECUTABLE}")
                set_tests_properties(IslandGameSettingsPersistence PROPERTIES
                    TIMEOUT 40
                    LABELS "display;input;storage"
                    RUN_SERIAL TRUE
                    PASS_REGULAR_EXPRESSION "settings persistence smoke passed")
            endif()
        endif()

        # The 2026-10-08 GPUVM incident occurred while CTest launched eight independent GL
        # clients against one private Xwayland/Weston compositor. Keep ordinary `ctest -jN`
        # useful for CPU tests while making GPU concurrency an explicit incident-qualification
        # choice. This is scheduling containment, not the root-cause fix; controlled stress runs
        # re-enable concurrency after serial kernel-log qualification.
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/runtime-logs")
        get_property(_island_configured_tests DIRECTORY PROPERTY TESTS)
        foreach(_island_test IN LISTS _island_configured_tests)
            get_test_property("${_island_test}" LABELS _island_test_labels)
            if("display" IN_LIST _island_test_labels)
                # A broad name filter (e.g. -R Monkey) must not bypass the retained
                # hardware stop. Contained software supervisors do not use these
                # direct display tests. Never clear the incident latch here.
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/gpu-incident-evidence/QUALIFICATION_STOP.json")
                    set_tests_properties("${_island_test}" PROPERTIES DISABLED TRUE)
                endif()
                set_property(TEST "${_island_test}" APPEND PROPERTY ENVIRONMENT
                    "ISLAND_RUNTIME_LOG_DIRECTORY=${CMAKE_BINARY_DIR}/runtime-logs")
                if(NOT ISLAND_ALLOW_PARALLEL_GPU_TESTS)
                    get_test_property("${_island_test}" RUN_SERIAL _island_run_serial)
                    if(NOT _island_run_serial)
                        set_tests_properties("${_island_test}" PROPERTIES
                            RESOURCE_LOCK island_gpu_display)
                    endif()
                endif()
            endif()
        endforeach()
    endif()
endif()
