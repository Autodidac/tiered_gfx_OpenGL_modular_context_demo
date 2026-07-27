set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(
    epoch_integrated_opengl_scene
    VERSION 4.6.7
    DESCRIPTION "C++23 cross-platform OpenGL scene with EpochGui, Tier-0 mobile budgets and Tier-1 desktop paths"
    LANGUAGES CXX
)

# Desktop builds are module-first on both MSVC and Linux. Catch the exact
# regression that caused MSVC C7577 before project generation reaches compile.
set(EPOCH_MODULE_IMPLEMENTATION_UNITS
    src/epoch/core/log.cpp
    src/epoch/core/time.cpp
    src/epoch/context/context_spine_module.cpp
    src/epoch/render/render_spine_module.cpp
    src/epoch/render/scene/camera.cpp
    src/epoch/render/scene/scene_spine.cpp
    src/epoch/render/techniques/technique_catalog.cpp
    src/app/application.cpp
)
foreach(module_source IN LISTS EPOCH_MODULE_IMPLEMENTATION_UNITS)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${module_source}" module_source_text)
    if(NOT module_source_text MATCHES "^[ \t\r\n]*module[ \t]*;")
        message(FATAL_ERROR
            "${module_source} must begin with an unconditional global module fragment: module;")
    endif()
endforeach()

if(UNIX AND NOT APPLE)
    if(NOT CMAKE_GENERATOR MATCHES "Ninja")
        message(FATAL_ERROR "Linux named-module builds require the Ninja generator.")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
        message(FATAL_ERROR "Linux named modules require GCC 14 or newer.")
    endif()
endif()

if(ANDROID)
    message(FATAL_ERROR
        "Use integration/android/app/src/main/cpp/CMakeLists.txt for the Android EGL/OpenGL ES pathway. "
        "The root project is the validated Windows/Linux desktop build.")
elseif(WIN32)
    enable_language(RC)
elseif(NOT UNIX OR APPLE)
    message(FATAL_ERROR "This package currently provides working Windows and Linux desktop paths.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(EPOCH_GENERATED_ASSET_ROOT "${CMAKE_BINARY_DIR}/generated_assets")
execute_process(
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_model_assets.py"
            --source-root "${CMAKE_CURRENT_SOURCE_DIR}"
            --destination "${EPOCH_GENERATED_ASSET_ROOT}"
    RESULT_VARIABLE EPOCH_MODEL_PREPARE_RESULT
    OUTPUT_VARIABLE EPOCH_MODEL_PREPARE_OUTPUT
    ERROR_VARIABLE EPOCH_MODEL_PREPARE_ERROR
)
if(NOT EPOCH_MODEL_PREPARE_RESULT EQUAL 0)
    message(FATAL_ERROR
        "Failed to reconstruct the exact runtime model assets.\n"
        "${EPOCH_MODEL_PREPARE_OUTPUT}${EPOCH_MODEL_PREPARE_ERROR}")
endif()
string(STRIP "${EPOCH_MODEL_PREPARE_OUTPUT}" EPOCH_MODEL_PREPARE_OUTPUT)
message(STATUS "${EPOCH_MODEL_PREPARE_OUTPUT}")

add_subdirectory(vendor/EpochGui)

set(EPOCH_PLATFORM_SOURCES)
set(EPOCH_PLATFORM_HEADERS)
if(WIN32)
    list(APPEND EPOCH_PLATFORM_SOURCES
        src/epoch/platform/win32/win32_window.cpp
        src/epoch/platform/win32/wgl_context.cpp
    )
    list(APPEND EPOCH_PLATFORM_HEADERS
        src/epoch/platform/win32/win32_window.hpp
        src/epoch/platform/win32/wgl_context.hpp
    )
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(PNG REQUIRED)
    list(APPEND EPOCH_PLATFORM_SOURCES
        src/epoch/platform/linux/x11_glx_window.cpp
    )
    list(APPEND EPOCH_PLATFORM_HEADERS
        src/epoch/platform/linux/x11_glx_window.hpp
    )
endif()
