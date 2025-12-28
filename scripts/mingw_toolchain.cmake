# Minimal CMake toolchain for mingw-w64 cross-compilation
set(CMAKE_SYSTEM_NAME Windows)

if(NOT DEFINED MINGW_ARCH)
  set(MINGW_ARCH "x86_64")
endif()

if(MINGW_ARCH STREQUAL "x86_64")
  set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
  set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
  set(WINDRES_EXECUTABLE x86_64-w64-mingw32-windres)
else()
  set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
  set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
  set(WINDRES_EXECUTABLE i686-w64-mingw32-windres)
endif()

set(CMAKE_RC_COMPILER ${WINDRES_EXECUTABLE})

# Tell CMake to prefer toolchain paths when cross-compiling
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# (Reverted) Do not force-include C++ headers globally — breaks C builds.
