# Silktone Ambient Suite — two macOS plugins (AU / VST3 / AAX / Standalone)
#   1. Silktone Ambient        — ambient multi-effects (centerpiece)
#   2. Silktone Vocal Channel  — vocal-first channel strip (EQ / comp / sat)
# AAX is built UNSIGNED (JUCE 8 vendors the AAX SDK 2.9.0, no separate
# download needed). Unsigned .aaxplugin loads ONLY in Pro Tools Developer,
# never in retail Pro Tools — that needs PACE signing (separate step).
# Build on a Mac (see README.md):
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
# On Linux this builds the VST3 + Standalone formats (AU is Apple-only)
# and the standalone DSP test:
#   ./build/test_dsp
cmake_minimum_required(VERSION 3.22)
project(SilktoneAmbientSuite VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Minimum macOS for Joel's machine (Monterey 12.7.6). NOTE: this line alone
# does NOT take effect with the Xcode generator — the workflow MUST also pass
# -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 on the cmake command line (set before
# project(), so the Xcode generator uses it as the project-wide default).
# This line keeps local builds honest.
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")

# JUCE is fetched at configure time (not committed to this repo).
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- Shared DSP core (no JUCE dependency — unit-testable standalone) ---
add_library(SilktoneAmbientDSP STATIC
    Shared/DSPBlocks.cpp
    Shared/FDNReverb.cpp
    Shared/TapeEchoDSP.cpp
    Shared/AmbientDSP.cpp
    Shared/VocalChannelDSP.cpp
)
# The VST3 is a shared object, so the static DSP library it links must
# be position-independent code (without this the Ambient VST3 link fails
# with a relocation error on the thread-local buffers in AmbientDSP).
set_target_properties(SilktoneAmbientDSP PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(SilktoneAmbientDSP PUBLIC Shared)
target_compile_features(SilktoneAmbientDSP PUBLIC cxx_std_17)

# --- Shared plugin setup ---
function(silktone_add_plugin TARGET_NAME PRODUCT_NAME BUNDLE_ID PLUGIN_CODE DESCRIPTION AAX_CATEGORY)
    juce_add_plugin(${TARGET_NAME}
        COMPANY_NAME "Silktone"
        COMPANY_WEBSITE "https://soundsgoodent.example"
        COMPANY_EMAIL "joel@soundsgoodent.example"
        BUNDLE_ID ${BUNDLE_ID}
        PLUGIN_MANUFACTURER_CODE SgEn
        PLUGIN_CODE ${PLUGIN_CODE}
        FORMATS AU VST3 AAX Standalone
        AAX_CATEGORY ${AAX_CATEGORY}
        PRODUCT_NAME ${PRODUCT_NAME}
        DESCRIPTION ${DESCRIPTION}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES "Fx"
        AU_MAIN_TYPE kAudioUnitType_Effect  # JUCE 8 wants the kAudioUnitType_* name, not the four-char code ("aufx" left JUCE_AU_MAIN_TYPE_CODE unset and the AU build failed)
        # COPY_PLUGIN_AFTER_BUILD must stay FALSE: for AAX, JUCE copies
        # the bundle to /Library/Application Support/Avid/Audio/Plug-Ins,
        # a system folder the CI runner cannot write to (copyDir.cmake
        # fails with "Maybe need administrative privileges"). The workflow
        # packages the bundles from the artefacts folder itself, and
        # installs the AUs for auval by hand — nothing needs the copy.
        COPY_PLUGIN_AFTER_BUILD FALSE
    )
    juce_generate_juce_header(${TARGET_NAME})
    target_compile_definitions(${TARGET_NAME} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        SilktoneAmbientDSP
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    )
endfunction()

silktone_add_plugin(SilktoneAmbient
    "Silktone Ambient"
    "com.soundsgoodent.silktone-ambient"
    SAmb
    "Lexicon / Valhalla-style ambient multi-effects"
    "AAX_ePlugInCategory_Effect|AAX_ePlugInCategory_Reverb"
)
target_sources(SilktoneAmbient PRIVATE
    Ambient/PluginProcessor.cpp
    Ambient/PluginEditor.cpp
    Shared/TronLookAndFeel.cpp
)

silktone_add_plugin(SilktoneVocalChannel
    "Silktone Vocal Channel"
    "com.soundsgoodent.silktone-vocal-channel"
    SVch
    "Vocal-first channel strip: EQ, compressor, saturation"
    "AAX_ePlugInCategory_EQ|AAX_ePlugInCategory_Dynamics"
)
target_sources(SilktoneVocalChannel PRIVATE
    VocalChannel/PluginProcessor.cpp
    VocalChannel/PluginEditor.cpp
    Shared/TronLookAndFeel.cpp
)

# --- Standalone DSP test (no JUCE needed) ---
#   ./build/test_dsp [ambient-demo.wav] [vocal-demo.wav]
add_executable(test_dsp tests/test_dsp.cpp)
target_link_libraries(test_dsp PRIVATE SilktoneAmbientDSP)
target_compile_features(test_dsp PRIVATE cxx_std_17)
