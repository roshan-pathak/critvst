Windows cross-build notes (from macOS)

This document describes a minimal approach to cross-compile or prepare Windows installers from macOS.

Prerequisites (macOS):
- Homebrew
- mingw-w64 (cross-compiler): `brew install mingw-w64`
- NSIS (`brew install nsis`) if you want to build NSIS installers on macOS

Notes:
- Cross-compiling a JUCE audio plugin to produce VST3/AU for Windows is non-trivial. The mingw toolchain can produce Windows binaries, but building a Windows VST3 plugin usually requires building with the VST3 SDK and ensuring correct X86/64 vs arm targets.
- For simple helper binaries or packaging steps you can produce Windows .exe installers using `makensis` on macOS once you have a staged folder of artifacts.

Simple cross-compile example (non-JUCE):

```bash
# Build a simple native executable for Windows x86_64 using mingw
x86_64-w64-mingw32-g++ -static -O2 -o mytool.exe mytool.cpp
```

Using NSIS on macOS:
- `makensis` is available via Homebrew (`brew install nsis`). You can then call `makensis installer.nsi` to generate an installer, but the NSIS script must not rely on Windows-only commands at build time (it can call them at runtime via installer sections).

Recommendation:
- For reliable Windows plugin builds, run the build on Windows (native or CI) with Visual Studio or MinGW as appropriate. Use cross-compilation only for helper tools or to generate installers when you have fully produced Windows artifacts.

