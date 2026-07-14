cmake_minimum_required(VERSION 3.22)

project(GuitarSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GuitarSynth
    COMPANY_NAME "GuitarSynth"
    BUNDLE_ID com.guitarsynth.plugin
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE GtSy
    PLUGIN_CODE GtS1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Guitar Synth"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Guitar Synth needs microphone access to track your guitar input.")

target_sources(GuitarSynth
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PitchTracker.cpp
        Source/PyinDecoder.cpp
        Source/EnvelopeFollower.cpp
        Source/SynthEngine.cpp
        Source/LookAndFeel.cpp)

target_compile_definitions(GuitarSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GuitarSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

juce_generate_juce_header(GuitarSynth)

if(TARGET GuitarSynth_Standalone)
    target_sources(GuitarSynth_Standalone PRIVATE Source/StandaloneMain.cpp)
    target_compile_definitions(GuitarSynth_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
endif()

enable_testing()

add_executable(GuitarSynthTests Tests/AudioInputTests.cpp)

add_executable(GuitarSynthProcessorTests Tests/ProcessorTests.cpp)
target_include_directories(GuitarSynthProcessorTests PRIVATE
    Source
    "${CMAKE_CURRENT_BINARY_DIR}/GuitarSynth_artefacts/JuceLibraryCode"
    "${CMAKE_BINARY_DIR}/_deps/juce-src/modules")
target_compile_definitions(GuitarSynthProcessorTests PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_MODULE_AVAILABLE_juce_audio_basics=1
    JUCE_MODULE_AVAILABLE_juce_audio_devices=1
    JUCE_MODULE_AVAILABLE_juce_audio_formats=1
    JUCE_MODULE_AVAILABLE_juce_audio_plugin_client=1
    JUCE_MODULE_AVAILABLE_juce_audio_processors=1
    JUCE_MODULE_AVAILABLE_juce_audio_utils=1
    JUCE_MODULE_AVAILABLE_juce_core=1
    JUCE_MODULE_AVAILABLE_juce_data_structures=1
    JUCE_MODULE_AVAILABLE_juce_dsp=1
    JUCE_MODULE_AVAILABLE_juce_events=1
    JUCE_MODULE_AVAILABLE_juce_graphics=1
    JUCE_MODULE_AVAILABLE_juce_gui_basics=1
    JUCE_MODULE_AVAILABLE_juce_gui_extra=1
    JUCE_SHARED_CODE=1
    JUCE_STANDALONE_APPLICATION=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(GuitarSynthProcessorTests PRIVATE
    GuitarSynth
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
add_dependencies(GuitarSynthProcessorTests GuitarSynth)
add_test(NAME GuitarSynthProcessorTests COMMAND GuitarSynthProcessorTests)

add_executable(GuitarSynthPitchTrackerTests Tests/PitchTrackerTests.cpp)
target_include_directories(GuitarSynthPitchTrackerTests PRIVATE
    Source
    "${CMAKE_CURRENT_BINARY_DIR}/GuitarSynth_artefacts/JuceLibraryCode"
    "${CMAKE_BINARY_DIR}/_deps/juce-src/modules")
target_compile_definitions(GuitarSynthPitchTrackerTests PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_MODULE_AVAILABLE_juce_core=1
    JUCE_MODULE_AVAILABLE_juce_dsp=1
    JUCE_SHARED_CODE=1
    JUCE_STANDALONE_APPLICATION=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(GuitarSynthPitchTrackerTests PRIVATE
    GuitarSynth
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
add_dependencies(GuitarSynthPitchTrackerTests GuitarSynth)
add_test(NAME GuitarSynthPitchTrackerTests COMMAND GuitarSynthPitchTrackerTests)

target_link_libraries(GuitarSynthTests PRIVATE)

add_test(NAME GuitarSynthTests COMMAND GuitarSynthTests)
