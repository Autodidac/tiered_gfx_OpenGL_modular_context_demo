# Android EGL / OpenGL ES pathway

This directory is an integration pathway, not a validated Android release. The desktop source tree is validated on Windows and Linux; no Android SDK or NDK was used for this package.

## Included

- NativeActivity manifest
- NDK CMake target
- EGL window/context lifecycle
- OpenGL ES 3.x proof frame
- explicit Tier-0 budget: one spotlight, four point lights, 3x3 directional-shadow PCF, vertex billboards, instancing, CPU cloth/particles and vertex water

## Connect the full renderer

1. Create an Android Studio Native C++ application and copy `app/src/main` into it.
2. Point `externalNativeBuild.cmake.path` at `app/src/main/cpp/CMakeLists.txt`.
3. Add the project `assets` directory to the APK assets and resolve it through `AAssetManager` instead of filesystem paths.
4. Add an EGL/OpenGL ES backend implementing the same context/resource contracts as the WGL and GLX paths.
5. Build GLES shader variants using `#version 320 es`, fragment precision declarations and Tier-0 compile definitions. Do not include tessellation, desktop timer-query, indirect-draw or Tier-1 PCSS shaders in the mobile package.
6. Feed touch/gamepad state into `context::InputState` and drive the existing scene/update/render ownership from the NativeActivity loop.

The `native_main.cpp` file owns only Android lifecycle and EGL. It intentionally does not fork world or editor data.
