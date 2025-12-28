# Install script for directory: /Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/opt/homebrew/bin/x86_64-w64-mingw32-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-build/modules/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-build/extras/Build/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/JUCE-8.0.4" TYPE FILE FILES
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-build/JUCEConfigVersion.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-build/JUCEConfig.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/JUCECheckAtomic.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/JUCEHelperTargets.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/JUCEModuleSupport.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/JUCEUtils.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/JuceLV2Defines.h.in"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/LaunchScreen.storyboard"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/PIPAudioProcessor.cpp.in"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/PIPAudioProcessorWithARA.cpp.in"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/PIPComponent.cpp.in"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/PIPConsole.cpp.in"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/RecentFilesMenuTemplate.nib"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/UnityPluginGUIScript.cs.in"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/checkBundleSigning.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/copyDir.cmake"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/juce_runtime_arch_detection.cpp"
    "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-src/extras/Build/CMake/juce_LinuxSubprocessHelper.cpp"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/roshanpathak/Desktop/Manual Library/scripts/crit_vst/crit/build-windows-x86_64/_deps/juce-build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
