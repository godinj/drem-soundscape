cmake_minimum_required(VERSION 3.22)
project(DremSoundscape VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_gui_app(DremSoundscape
    PRODUCT_NAME "Drem Soundscape"
    COMPANY_NAME "Drem"
    BUNDLE_ID "com.drem.soundscape"
)

juce_generate_juce_header(DremSoundscape)

target_sources(DremSoundscape PRIVATE
    src/Main.cpp
    src/LoopingAudioSource.cpp
    src/WaveformDisplay.cpp
    src/SoundLayer.cpp
    src/CrossfadeCurveEditor.cpp
    src/FilteredAudioSource.cpp
    src/MainComponent.cpp
)

target_include_directories(DremSoundscape PRIVATE src)

target_compile_definitions(DremSoundscape PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DremSoundscape,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DremSoundscape,JUCE_VERSION>"
)

target_link_libraries(DremSoundscape PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
