cmake_minimum_required(VERSION 3.22)

project(VOIDWORM VERSION 1.0.0 LANGUAGES CXX)

option(VOIDWORM_BUILD_PERF_AUDIT "Build the non-shipping CPU performance audit tool" OFF)
option(VOIDWORM_ENABLE_PERF_STAGE_TIMERS "Enable stage timers for the CPU audit tool" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(vendor/JUCE)

juce_add_binary_data(VOIDWORM_Assets
    HEADER_NAME VoidwormAssets.h
    NAMESPACE VoidwormAssets
    SOURCES
        Source/Assets/industrial_metal_v2.png
        Source/Assets/extracted_reference_faceplate_v1.png
        Source/Assets/hellforge_neon_faceplate_v1.png)

juce_add_plugin(VOIDWORM
    COMPANY_NAME "LWNX DSP"
    BUNDLE_ID "io.github.lew0nn.voidworm"
    PLUGIN_MANUFACTURER_CODE Vdwm
    PLUGIN_CODE VdWm
    FORMATS VST3 Standalone
    PRODUCT_NAME "VOIDWORM"
    DESCRIPTION "Source-reactive parallel industrial audio processor"
    VST3_CATEGORIES Fx Distortion Dynamics
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(VOIDWORM PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ParameterLayout.cpp
    Source/ParameterLayout.h
    Source/PresetDefinitions.h
    Source/PresetTransition.h
    Source/UserPresetStore.cpp
    Source/UserPresetStore.h
    Source/DSP/OutputTone.cpp
    Source/DSP/OutputTone.h
    Source/DSP/SourceAnalyzer.cpp
    Source/DSP/SourceAnalyzer.h
    Source/DSP/OversamplingSystem.cpp
    Source/DSP/OversamplingSystem.h
    Source/DSP/MassChain.cpp
    Source/DSP/MassChain.h
    Source/DSP/FurnaceChain.cpp
    Source/DSP/FurnaceChain.h
    Source/DSP/ReactorRack.cpp
    Source/DSP/ReactorRack.h
    Source/DSP/ReactorCharacter.h
    Source/DSP/RealtimeDiagnostics.h
    Source/DSP/ReactorPreEq.cpp
    Source/DSP/ReactorPreEq.h
    Source/DSP/Dynamics.cpp
    Source/DSP/Dynamics.h
    Source/DSP/WeldProcessor.cpp
    Source/DSP/WeldProcessor.h
    Source/DSP/FinalLimiter.cpp
    Source/DSP/FinalLimiter.h
    Source/DSP/InputNoiseGate.cpp
    Source/DSP/InputNoiseGate.h
    Source/DSP/NonlinearStage.cpp
    Source/DSP/NonlinearStage.h
    Source/DSP/ArcChain.cpp
    Source/DSP/ArcChain.h
    Source/DSP/FeedbackChain.cpp
    Source/DSP/FeedbackChain.h
    Source/DSP/TearProcessor.cpp
    Source/DSP/TearProcessor.h
    Source/DSP/VoidEngine.cpp
    Source/DSP/VoidEngine.h
    Source/UI/VoidLookAndFeel.cpp
    Source/UI/VoidLookAndFeel.h
    Source/UI/ThemePalette.h
    Source/UI/FooterAnimation.h
    Source/UI/EditorLayout.h
    Source/UI/EqGraphInteraction.h
    Source/UI/HeaderPanelModel.h
    Source/UI/ReactorWorkspaceState.h)

target_compile_definitions(VOIDWORM PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if (VOIDWORM_BUILD_PERF_AUDIT AND VOIDWORM_ENABLE_PERF_STAGE_TIMERS)
    target_compile_definitions(VOIDWORM PRIVATE VOIDWORM_ENABLE_PERF_PROFILING=1)
endif()

# Keep the PNG as JUCE's master cross-platform artwork. Windows executable
# resources include the supplied multi-resolution ICO verbatim.
if (WIN32)
    target_compile_definitions(VOIDWORM PUBLIC
        JUCE_USER_DEFINED_RC_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Source/Windows/VOIDWORM.rc"
        VOIDWORM_APP_ICON_PATH="${CMAKE_CURRENT_SOURCE_DIR}/VOIDWORM_Windows_Icon.ico")
else()
    set_target_properties(VOIDWORM PROPERTIES
        JUCE_ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/VOIDWORM_Master_Artwork_512.png")
endif()

if (VOIDWORM_BUILD_PERF_AUDIT)
    juce_add_console_app(VOIDWORM_PerformanceAudit
        PRODUCT_NAME "VOIDWORM Performance Audit")
    target_sources(VOIDWORM_PerformanceAudit PRIVATE
        Tools/PerformanceAudit.cpp)
    target_link_libraries(VOIDWORM_PerformanceAudit PRIVATE
        VOIDWORM
        juce::juce_audio_utils
        juce::juce_gui_extra)
    target_compile_definitions(VOIDWORM_PerformanceAudit PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        VOIDWORM_ENABLE_PERF_PROFILING=$<BOOL:${VOIDWORM_ENABLE_PERF_STAGE_TIMERS}>)
    juce_generate_juce_header(VOIDWORM_PerformanceAudit)
endif()

target_link_libraries(VOIDWORM PRIVATE
    VOIDWORM_Assets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

juce_generate_juce_header(VOIDWORM)

target_sources(VOIDWORM_Standalone PRIVATE
    Source/Standalone/VoidwormStandalone.cpp)
target_compile_definitions(VOIDWORM_Standalone PRIVATE
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

if (WIN32)
    # Keep the optional Explorer icon stored beside the VST3 bundle in sync with
    # the exact supplied multi-resolution ICO. This is a byte-for-byte copy.
    add_custom_command(TARGET VOIDWORM_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/VOIDWORM_Windows_Icon.ico"
            "$<TARGET_FILE_DIR:VOIDWORM_VST3>/../../Plugin.ico"
        VERBATIM)
endif()
