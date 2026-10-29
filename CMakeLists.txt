cmake_minimum_required(VERSION 3.22)

project(GuitarDelay VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.4
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

set(GUITAR_DELAY_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND GUITAR_DELAY_FORMATS AU)
endif()

# A Linux-hosted MinGW build cannot run the Windows VST3 manifest helper.
# moduleinfo.json is written afterwards by installer/windows/write_moduleinfo.cmake.
set(GUITAR_DELAY_VST3_AUTO_MANIFEST TRUE)
if(WIN32 AND CMAKE_CROSSCOMPILING)
    set(GUITAR_DELAY_VST3_AUTO_MANIFEST FALSE)
endif()

add_library(GuitarDelayDsp STATIC
    Source/DSP/TapeDelay.cpp
    Source/DSP/AnalogDelay.cpp
    Source/DSP/DigitalDelay.cpp
)
set_target_properties(GuitarDelayDsp PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(GuitarDelayDsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_compile_features(GuitarDelayDsp PUBLIC cxx_std_20)
target_link_libraries(GuitarDelayDsp
    PUBLIC
        juce::juce_audio_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

juce_add_plugin(GuitarDelay
    COMPANY_NAME "White Tea"
    BUNDLE_ID com.whitetea.guitardelay
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Wtea
    PLUGIN_CODE Gdly
    FORMATS ${GUITAR_DELAY_FORMATS}
    PRODUCT_NAME "Sunset Delay"
    # Insert effect. JUCE emits Fx|Delay. An Instrument subcategory is
    # Fx|Instrument, and hosts then load the plugin with the audio input off.
    VST3_CATEGORIES Delay Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
    VST3_AUTO_MANIFEST ${GUITAR_DELAY_VST3_AUTO_MANIFEST}
)

target_sources(GuitarDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

juce_add_binary_data(GuitarDelayAssets
    SOURCES
        Resources/landscape.jpg
)

target_compile_definitions(GuitarDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_JACK=0
        JUCE_ALSA=1
)

target_link_libraries(GuitarDelay
    PRIVATE
        GuitarDelayDsp
        GuitarDelayAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

if(TARGET GuitarDelay_Standalone)
    target_sources(GuitarDelay_Standalone PRIVATE Source/StandaloneApp.cpp)
    target_compile_definitions(GuitarDelay_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
endif()

juce_add_console_app(GuitarDelayRenderTest
    PRODUCT_NAME "guitardelay-render-test"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_sources(GuitarDelayRenderTest
    PRIVATE
        tests/DspRenderTest.cpp
)

target_compile_definitions(GuitarDelayRenderTest
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(GuitarDelayRenderTest
    PRIVATE
        GuitarDelayDsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

juce_add_console_app(GuitarDelayHostCheck
    PRODUCT_NAME "sunset-delay-host-check"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_sources(GuitarDelayHostCheck
    PRIVATE
        tests/HostCompatibility.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(GuitarDelayHostCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_JACK=0
        JUCE_ALSA=1
)

target_link_libraries(GuitarDelayHostCheck
    PRIVATE
        GuitarDelayDsp
        GuitarDelayAssets
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

juce_add_console_app(GuitarDelayRenderDemos
    PRODUCT_NAME "guitardelay-render-demos"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_sources(GuitarDelayRenderDemos
    PRIVATE
        tools/RenderDemos.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(GuitarDelayRenderDemos
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_JACK=0
        JUCE_ALSA=1
)

target_link_libraries(GuitarDelayRenderDemos
    PRIVATE
        GuitarDelayDsp
        GuitarDelayAssets
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# clang-cl defines JUCE_CLANG, but libwebp's AVX2/SSE4.1 wrappers use a clang
# target pragma that does not expose the intrinsic types on the MSVC frontend.
# Those translation units are CPU-dispatched, so raising the arch here does not
# change the rest of the plugin.
if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(TARGET juce_graphics)
        get_target_property(guitar_delay_gfx_dir juce_graphics INTERFACE_JUCE_MODULE_PATH)
        if(guitar_delay_gfx_dir)
            set_source_files_properties(
                "${guitar_delay_gfx_dir}/juce_graphics/juce_graphics_libwebp_dsp_dec_avx2.c"
                "${guitar_delay_gfx_dir}/juce_graphics/juce_graphics_libwebp_dsp_enc_avx2.c"
                PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
            set_source_files_properties(
                "${guitar_delay_gfx_dir}/juce_graphics/juce_graphics_libwebp_dsp_dec_sse41.c"
                "${guitar_delay_gfx_dir}/juce_graphics/juce_graphics_libwebp_dsp_enc_sse41.c"
                PROPERTIES COMPILE_OPTIONS "/clang:-msse4.1")
        endif()
    endif()
endif()

if(WIN32 AND CMAKE_CROSSCOMPILING AND TARGET GuitarDelay_VST3)
    add_custom_command(TARGET GuitarDelay_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DBUNDLE_DIR=$<TARGET_FILE_DIR:GuitarDelay_VST3>/../..
            -DVERSION=${PROJECT_VERSION}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/write_moduleinfo.cmake
        VERBATIM)
endif()

# Stages a Windows build, compiles installer/windows/GuitarDelay.iss
# (Inno Setup) or the NSIS fallback, and zips the VST3 bundle.
# Not part of the default Linux build.
add_custom_target(package_windows
    COMMAND ${CMAKE_COMMAND} -E env
        GUITAR_DELAY_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}
        GUITAR_DELAY_BUILD=${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/stage-and-pack.sh
    COMMENT "Build the Windows installer from the staged VST3 and standalone"
    VERBATIM)
if(TARGET GuitarDelay_VST3)
    add_dependencies(package_windows GuitarDelay_VST3)
endif()
if(TARGET GuitarDelay_Standalone)
    add_dependencies(package_windows GuitarDelay_Standalone)
endif()
