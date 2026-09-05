# AERIFORM - oscillator-free physical-modelling synthesizer (JUCE / CMake)
cmake_minimum_required(VERSION 3.22)

project(Aeriform VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(AERIFORM_BUILD_TESTS "Build the AERIFORM automated test executable" ON)
option(AERIFORM_BUILD_HOST_CHECK "Build the VST3 host-load validation tool" ON)
option(AERIFORM_COPY_PLUGIN "Copy the built plug-in into the user VST3 folder after build" OFF)
option(AERIFORM_STATIC_RUNTIME "Statically link the compiler runtime (MinGW)" ON)

# ---------------------------------------------------------------------------
# JUCE: prefer a vendored checkout in external/JUCE, fall back to FetchContent.
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)
else()
    message(STATUS "external/JUCE not found - fetching JUCE 7.0.12 with FetchContent")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plug-in target
# ---------------------------------------------------------------------------
set(AERIFORM_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND AERIFORM_FORMATS AU)
endif()

juce_add_plugin(Aeriform
    PRODUCT_NAME                "EXP_Aeriform"
    COMPANY_NAME                "Aeriform Audio"
    COMPANY_WEBSITE             "https://example.invalid/aeriform"
    BUNDLE_ID                   "com.aeriformaudio.exp-aeriform"
    PLUGIN_MANUFACTURER_CODE    Aerf
    PLUGIN_CODE                 Exaf
    FORMATS                     ${AERIFORM_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     ${AERIFORM_COPY_PLUGIN}
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_AUTO_MANIFEST          FALSE)

juce_generate_juce_header(Aeriform)

set(AERIFORM_SOURCES
    Source/State/PatchStateManager.cpp
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.cpp
    Source/Params/ParameterLayout.cpp
    Source/DSP/SynthEngine.cpp
    Source/DSP/SympatheticBank.cpp
    Source/DSP/ModularFilters.cpp
    Source/DSP/Voice.cpp
    Source/DSP/Exciter.cpp
    Source/DSP/Exciters/ExciterSlot.cpp
    Source/DSP/Resonator.cpp
    Source/DSP/ResonatorNetwork.cpp
    Source/DSP/ModMatrix.cpp
    Source/DSP/Effects/Chorus.cpp
    Source/DSP/Effects/Delay.cpp
    Source/DSP/Effects/Reverb.cpp
    Source/DSP/Effects/OutputStage.cpp
    Source/Presets/PresetManager.cpp
    Source/Presets/FactoryPresets.cpp
    Source/MIDI/MidiLearn.cpp
    Source/GUI/LookAndFeel.cpp
    Source/GUI/Knob.cpp
    Source/GUI/Visualizer.cpp
    Source/GUI/PresetBar.cpp
    Source/GUI/ModMatrixPanel.cpp
    Source/GUI/Displays.cpp
    Source/GUI/SympatheticPage.cpp
    Source/GUI/ContactPage.cpp
    Source/GUI/FiltersPage.cpp
    Source/GUI/PerformancePage.cpp
    Source/GUI/Pages.cpp
    Source/GUI/Panels/ExciterSlotPanel.cpp
    Source/GUI/Panels/ExcitersOverviewPanel.cpp
    Source/GUI/Panels/ShapingPanels.cpp
    Source/GUI/Panels/NetworkPanels.cpp
    Source/GUI/Panels/ResonatorPanel.cpp
    Source/GUI/Panels/MotionPanel.cpp
    Source/GUI/Panels/SpacePanel.cpp
    Source/GUI/Panels/MasterPanel.cpp)

target_sources(Aeriform PRIVATE ${AERIFORM_SOURCES})
target_include_directories(Aeriform PRIVATE Source)

target_compile_definitions(Aeriform PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_ASIO=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Aeriform,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Aeriform,JUCE_VERSION>"
    AERIFORM_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(Aeriform
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Compiler / linker tuning
# ---------------------------------------------------------------------------
if(MINGW)
    # Large JUCE translation units exceed the default COFF section limit in Debug.
    target_compile_options(Aeriform PUBLIC -Wa,-mbig-obj)
    if(AERIFORM_STATIC_RUNTIME)
        # Make the VST3 / standalone self-contained (no libstdc++/libgcc/winpthread DLL deps).
        foreach(t IN ITEMS Aeriform_VST3 Aeriform_Standalone)
            if(TARGET ${t})
                target_link_options(${t} PRIVATE -static -static-libgcc -static-libstdc++)
            endif()
        endforeach()
    endif()
endif()

if(MSVC)
    target_compile_options(Aeriform PUBLIC /bigobj /utf-8)
endif()

# ---------------------------------------------------------------------------
# VST3 moduleinfo.json: JUCE's built-in step chains nested cmd.exe calls that
# break on paths containing '^'. Generate the manifest with a single VERBATIM
# command instead (moduleinfo.json is optional for hosts but speeds up scanning).
# ---------------------------------------------------------------------------
if(TARGET Aeriform_VST3)
    _juce_add_vst3_manifest_helper_target()
    add_dependencies(Aeriform_VST3 juce_vst3_helper)
    if(MINGW AND AERIFORM_STATIC_RUNTIME)
        target_link_options(juce_vst3_helper PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    get_target_property(AERIFORM_VST3_BUNDLE Aeriform_VST3 JUCE_PLUGIN_ARTEFACT_FILE)
    add_custom_command(TARGET Aeriform_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${AERIFORM_VST3_BUNDLE}/Contents/Resources"
        COMMAND juce_vst3_helper -create -version "${PROJECT_VERSION}" -path "${AERIFORM_VST3_BUNDLE}"
                -output "${AERIFORM_VST3_BUNDLE}/Contents/Resources/moduleinfo.json"
        COMMENT "Generating VST3 moduleinfo.json"
        VERBATIM)
endif()

# Fast-math is deliberately NOT enabled: NaN/Inf detection relies on IEEE semantics.

# ---------------------------------------------------------------------------
# Tests: link the plug-in shared-code static library. JUCE re-exports the
# module definitions publicly; include paths are copied from the target so the
# JUCE modules are compiled exactly once.
# ---------------------------------------------------------------------------
if(AERIFORM_BUILD_TESTS)
    enable_testing()
    add_executable(AeriformTests
        Tests/TestMain.cpp
        Tests/TestFramework.h
        Tests/DspTests.cpp
        Tests/ParameterTests.cpp
        Tests/StateTests.cpp
        Tests/VoiceTests.cpp
        Tests/SidechainTests.cpp
        Tests/ExciterTests.cpp
        Tests/FolderTests.cpp
        Tests/NetworkTests.cpp
        Tests/MigrationTests.cpp
        Tests/EditorTests.cpp
        Tests/ProfileTests.cpp
        Tests/PresetLevelTests.cpp
        Tests/ReliabilityTests.cpp
        Tests/PatchToolsTests.cpp
        Tests/KnobModulationTests.cpp
        Tests/UserRegressionTests.cpp
        Tests/SympatheticTests.cpp
        Tests/StereoNetworkTests.cpp
        Tests/CollisionTests.cpp
        Tests/ModularFilterTests.cpp
        Tests/SmokeTests.cpp)
    target_include_directories(AeriformTests PRIVATE
        Source Tests
        $<TARGET_PROPERTY:Aeriform,INCLUDE_DIRECTORIES>)
    target_compile_definitions(AeriformTests PRIVATE
        JucePlugin_Build_Standalone=1
        JucePlugin_Build_VST3=0
        JucePlugin_Build_AU=0
        JucePlugin_Build_AUv3=0
        JucePlugin_Build_AAX=0
        JucePlugin_Build_Unity=0
        JucePlugin_Build_LV2=0
        JucePlugin_Build_VST=0
        JucePlugin_Name="AERIFORM"
        JucePlugin_Desc="AERIFORM"
        JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_Version=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        JucePlugin_VersionCode=0x10000
        JucePlugin_Enable_IAA=0
        JucePlugin_MaxNumInputChannels=2
        JucePlugin_MaxNumOutputChannels=2
        JucePlugin_PreferredChannelConfigurations=
        AERIFORM_TESTS=1)
    target_link_libraries(AeriformTests PRIVATE Aeriform)
    if(MINGW)
        target_compile_options(AeriformTests PRIVATE -Wa,-mbig-obj)
        if(AERIFORM_STATIC_RUNTIME)
            target_link_options(AeriformTests PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    endif()
    add_test(NAME AeriformTests COMMAND AeriformTests)
    add_test(NAME AeriformSmoke COMMAND AeriformTests --smoke)
endif()

# ---------------------------------------------------------------------------
# VST3 host-load check: a tiny JUCE host that loads the built .vst3, prepares it,
# sends notes and renders audio. Stand-in for pluginval when it is unavailable.
# ---------------------------------------------------------------------------
if(AERIFORM_BUILD_HOST_CHECK)
    juce_add_console_app(AeriformHostCheck PRODUCT_NAME "AeriformHostCheck")
    target_sources(AeriformHostCheck PRIVATE Tests/HostCheck/HostCheckMain.cpp)
    target_compile_definitions(AeriformHostCheck PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(AeriformHostCheck PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_events
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    if(MINGW)
        target_compile_options(AeriformHostCheck PRIVATE -Wa,-mbig-obj)
        if(AERIFORM_STATIC_RUNTIME)
            target_link_options(AeriformHostCheck PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    endif()
endif()
