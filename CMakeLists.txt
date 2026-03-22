cmake_minimum_required(VERSION 3.22)
project(C700 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/JUCE)

juce_add_plugin(C700
    COMPANY_NAME "C700Fork"
    PLUGIN_MANUFACTURER_CODE C7Fk
    PLUGIN_CODE C700
    FORMATS VST3
    PRODUCT_NAME "C700"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(C700
    PRIVATE
        # --- JUCE plugin wrapper ---
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginEditor.cpp
        src/plugin/C700Adapter.cpp

        # --- DSP core engine ---
        C700Kernel.cpp
        C700DSP.cpp
        C700Driver.cpp
        MidiDriverBase.cpp
        DynamicVoiceAllocator.cpp
        brrcodec.cpp
        EchoKernel.cpp
        DspController.cpp
        DspRegFIFO.cpp
        RegisterLogger.cpp

        # --- Data / utility ---
        DataBuffer.cpp
        MemManager.cpp
        ChunkReader.cpp
        FileAccess.cpp
        PlayerCodeReader.cpp
        C700Parameters.cpp
        C700Properties.cpp

        # --- File generation (needed by C700DSP register logger) ---
        PlayingFileGenerateBase.cpp
        SmcFileGenerate.cpp
        SpcFileGenerate.cpp

        # --- USB hardware stubs (compiled, stubs on Linux) ---
        commusb/SpcControlDevice.cpp

        # --- Blargg SPC emulation ---
        snes_spc/SNES_SPC.cpp
        snes_spc/SNES_SPC_misc.cpp
        snes_spc/SNES_SPC_state.cpp
        snes_spc/SPC_DSP.cpp
        snes_spc/SPC_Filter.cpp
        snes_spc/dsp.cpp
        snes_spc/spc.cpp

        # --- Linux platform utilities ---
        src/platform/PathUtils.cpp
)

target_include_directories(C700
    PRIVATE
        src/plugin
        ${CMAKE_CURRENT_SOURCE_DIR}        # repo root (C700Kernel.h etc.)
        ${CMAKE_CURRENT_SOURCE_DIR}/commusb
        ${CMAKE_CURRENT_SOURCE_DIR}/snes_spc
)

target_compile_definitions(C700
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
    PRIVATE
        PATH_LEN_MAX=1024
)

target_link_libraries(C700
    PRIVATE
        juce::juce_audio_utils
        pthread
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
