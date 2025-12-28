cmake_minimum_required(VERSION 3.15)

project(crit VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE - using 8.0.4 for macOS 15+ SDK compatibility
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

# Plugin formats to build
set(FORMATS VST3 AU)

juce_add_plugin(crit
    COMPANY_NAME "rosh"
    BUNDLE_ID com.roshanpathak.crit
    PLUGIN_MANUFACTURER_CODE RsPk
    PLUGIN_CODE Crit
    FORMATS ${FORMATS}
    PRODUCT_NAME "crit"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(crit)

target_sources(crit
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Waveshaper.cpp
        Source/DSP/DynamicEQ.cpp
        Source/DSP/PitchDrift.cpp
        Source/DSP/FaultDecimator.cpp
        Source/DSP/MidSideDivergence.cpp
        Source/DSP/SoftLimiter.cpp
)

# Console test target for block-level DSP testing
juce_add_console_app(crit_test)
juce_generate_juce_header(crit_test)
target_sources(crit_test
    PRIVATE
        test/test_harness.cpp
        Source/DSP/FaultDecimator.cpp
        Source/DSP/MidSideDivergence.cpp
        Source/DSP/SoftLimiter.cpp
        Source/DSP/DynamicEQ.cpp
        Source/DSP/Waveshaper.cpp
)
target_link_libraries(crit_test PRIVATE juce::juce_dsp juce::juce_core juce::juce_audio_basics)

# Put test binary and outputs under the top-level test/ directory
set_target_properties(crit_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/test/bin")

target_compile_definitions(crit
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(crit
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Install any shipped factory presets placed under Source/Presets into the package installer.
# If you add .crit files to Source/Presets, the installer will include them so they can be
# extracted onto user machines during installation or by the plugin on first run.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Source/Presets DESTINATION share/crit FILES_MATCHING PATTERN "*.crit")

# Install common top-level docs and assets into the package share area so installers
# can include them alongside plugin bundles.
install(FILES ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION share/crit OPTIONAL)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION share/crit/assets OPTIONAL)

# On macOS, add install rules for the built plugin bundles so packaging tools (cpack)
# can include them in installer images. These install destinations are relative to
# the package root during `cpack` packaging and map to system locations on install.
if(APPLE)
    # Note: build layout places bundles under ${CMAKE_BINARY_DIR}/crit_artefacts/<Config>/{AU,VST3}
    set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/crit_artefacts/${CMAKE_BUILD_TYPE}")
    if(EXISTS "${BUNDLE_DIR}/AU/crit.component")
        install(DIRECTORY "${BUNDLE_DIR}/AU/crit.component" DESTINATION "Library/Audio/Plug-Ins/Components")
    endif()
    if(EXISTS "${BUNDLE_DIR}/VST3/crit.vst3")
        install(DIRECTORY "${BUNDLE_DIR}/VST3/crit.vst3" DESTINATION "Library/Audio/Plug-Ins/VST3")
    endif()
endif()

# Custom helper target to run the platform install script which copies built plugin bundles
# and presets into system locations. Run with:
#   cmake --build build --target install_bundle
if(APPLE)
    add_custom_target(install_bundle
        COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/scripts/install/install_mac.sh ${CMAKE_SOURCE_DIR}
        COMMENT "Installing plugin bundles and presets to system directories (macOS)"
    )
elseif(WIN32)
    add_custom_target(install_bundle
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/install/install_windows.ps1 ${CMAKE_SOURCE_DIR}
        COMMENT "Installing plugin bundles and presets to system directories (Windows)"
    )
else()
    add_custom_target(install_bundle
        COMMAND ${CMAKE_COMMAND} -E echo "install_bundle target not supported on this platform"
    )
endif()

# CPack packaging configuration (create a macOS Drag-and-Drop DMG via `cpack -G DragNDrop`)
set(CPACK_PACKAGE_NAME "crit")
set(CPACK_PACKAGE_VENDOR "rosh")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_DMG_VOLUME_NAME "crit")
set(CPACK_DMG_FORMAT "UDZO")

include(CPack)

# Convenience target to run cpack from the build directory (generates DMG)
add_custom_target(package_all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
    COMMENT "Run CPack to generate packages (DMG for macOS)"
)

# Staging target for PKG creation: installs files into a staging prefix so external
# packaging tools (pkgbuild/productbuild) can create OS installers from the layout.
add_custom_target(stage_for_pkg
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --prefix ${CMAKE_BINARY_DIR}/package_stage
    COMMENT "Create staging install tree at ${CMAKE_BINARY_DIR}/package_stage"
)

# Convenience targets to create macOS PKG using the helper script. These targets will
# first create a staging tree then invoke the packaging script to produce a .pkg.
if(APPLE)
    add_custom_target(package_pkg_arm64
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --prefix ${CMAKE_BINARY_DIR}/package_stage
        COMMAND ${CMAKE_COMMAND} -E env CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}/scripts/package_mac_pkgs.sh ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/dist arm64
        COMMENT "Stage and build macOS PKG for Apple Silicon (arm64)"
    )

    add_custom_target(package_pkg_x86_64
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --prefix ${CMAKE_BINARY_DIR}/package_stage
        COMMAND ${CMAKE_COMMAND} -E env CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}/scripts/package_mac_pkgs.sh ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/dist x86_64
        COMMENT "Stage and build macOS PKG for Intel (x86_64)"
    )
endif()
