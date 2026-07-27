cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(
    epoch_integrated_opengl_scene
    VERSION 4.6.6
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

set(EPOCH_SPINE_SOURCES
    src/epoch/core/log.cpp
    src/epoch/core/time.cpp
    src/epoch/context/detail/context_spine_impl.cpp
    src/epoch/context/detail/context_spine_bridge.cpp
    src/epoch/context/context_spine_module.cpp
    ${EPOCH_PLATFORM_SOURCES}
    src/epoch/render/gl/gl_api.cpp
    src/epoch/render/gl/image.cpp
    src/epoch/render/gl/mesh.cpp
    src/epoch/render/gl/obj_loader.cpp
    src/epoch/render/gl/shader_program.cpp
    src/epoch/render/gl/texture.cpp
    src/epoch/render/gl/frame_targets.cpp
    src/epoch/render/resource_spine.cpp
    src/epoch/render/scene/camera.cpp
    src/epoch/render/scene/scene_spine.cpp
    src/epoch/render/techniques/shadow_mapping.cpp
    src/epoch/render/techniques/point_shadow_mapping.cpp
    src/epoch/render/techniques/sky_environment.cpp
    src/epoch/render/techniques/pbr_forward.cpp
    src/epoch/render/techniques/billboard_foliage.cpp
    src/epoch/render/techniques/particle_system.cpp
    src/epoch/render/techniques/instanced_props.cpp
    src/epoch/render/techniques/render_to_texture.cpp
    src/epoch/render/techniques/bloom.cpp
    src/epoch/render/techniques/post_process.cpp
    src/epoch/render/techniques/tessellation.cpp
    src/epoch/render/techniques/cloth_simulation.cpp
    src/epoch/render/techniques/water_simulation.cpp
    src/epoch/render/techniques/gpu_profiler.cpp
    src/epoch/render/techniques/technique_catalog.cpp
    src/epoch/render/world/world_scene.cpp
    src/epoch/gui/gui_overlay.cpp
    src/epoch/render/detail/render_spine_impl.cpp
    src/epoch/render/detail/render_spine_bridge.cpp
    src/epoch/render/render_spine_module.cpp
)

set(EPOCH_SPINE_HEADERS
    src/epoch/context/detail/context_spine_impl.hpp
    src/epoch/context/detail/context_spine_bridge.hpp
    ${EPOCH_PLATFORM_HEADERS}
    src/epoch/render/gl/gl_api.hpp
    src/epoch/render/gl/image.hpp
    src/epoch/render/gl/mesh.hpp
    src/epoch/render/gl/shader_program.hpp
    src/epoch/render/gl/texture.hpp
    src/epoch/render/gl/frame_targets.hpp
    src/epoch/render/resource_spine.hpp
    src/epoch/render/detail/render_spine_impl.hpp
    src/epoch/render/detail/render_spine_bridge.hpp
    src/epoch/render/techniques/shadow_mapping.hpp
    src/epoch/render/techniques/point_shadow_mapping.hpp
    src/epoch/render/techniques/sky_environment.hpp
    src/epoch/render/techniques/pbr_forward.hpp
    src/epoch/render/techniques/billboard_foliage.hpp
    src/epoch/render/techniques/particle_system.hpp
    src/epoch/render/techniques/instanced_props.hpp
    src/epoch/render/techniques/render_to_texture.hpp
    src/epoch/render/techniques/bloom.hpp
    src/epoch/render/techniques/post_process.hpp
    src/epoch/render/techniques/tessellation.hpp
    src/epoch/render/techniques/cloth_simulation.hpp
    src/epoch/render/techniques/water_simulation.hpp
    src/epoch/render/techniques/gpu_profiler.hpp
    src/epoch/render/world/world_scene.hpp
    src/epoch/render/world/hardcoded_scene_defaults.inc
    src/epoch/render/world/hardcoded_material_defaults.inc
    src/epoch/gui/gui_overlay.hpp
)

set_source_files_properties(
    src/epoch/render/world/hardcoded_scene_defaults.inc
    src/epoch/render/world/hardcoded_material_defaults.inc
    PROPERTIES HEADER_FILE_ONLY TRUE
)

set(EPOCH_SPINE_MODULES
    src/epoch/modules/epoch.core.math.ixx
    src/epoch/modules/epoch.core.log.ixx
    src/epoch/modules/epoch.core.time.ixx
    src/epoch/modules/epoch.context.input.ixx
    src/epoch/modules/epoch.context.frame.ixx
    src/epoch/modules/epoch.context.spine.ixx
    src/epoch/modules/epoch.render.types.ixx
    src/epoch/modules/epoch.render.tier.ixx
    src/epoch/modules/epoch.render.scene.ixx
    src/epoch/modules/epoch.render.technique.context.ixx
    src/epoch/modules/epoch.render.techniques.catalog.ixx
    src/epoch/modules/epoch.render.spine.ixx
)

file(GLOB EPOCH_COMPAT_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/epoch/compat/*.hpp")
add_library(epoch_render_spine STATIC ${EPOCH_SPINE_SOURCES} ${EPOCH_SPINE_HEADERS} ${EPOCH_COMPAT_HEADERS})
target_sources(epoch_render_spine PUBLIC
    FILE_SET epoch_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/epoch/modules"
        FILES ${EPOCH_SPINE_MODULES}
)
add_library(epoch::render_spine ALIAS epoch_render_spine)
target_include_directories(epoch_render_spine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(epoch_render_spine PUBLIC cxx_std_23)
set_target_properties(epoch_render_spine PROPERTIES
    CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO
    FOLDER "Epoch/Renderer Spine"
)

target_link_libraries(epoch_render_spine PUBLIC EpochGui)
if(WIN32)
    target_compile_definitions(epoch_render_spine PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    target_link_libraries(epoch_render_spine PUBLIC opengl32 gdi32 user32 ole32 windowscodecs comdlg32)
    set_target_properties(epoch_render_spine PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    target_compile_definitions(epoch_render_spine PRIVATE EPOCH_PLATFORM_LINUX=1)
    target_link_libraries(epoch_render_spine PUBLIC X11::X11 PNG::PNG ${CMAKE_DL_LIBS} pthread)
endif()

set(EPOCH_APP_SOURCES src/app/main.cpp src/app/application.cpp)
if(WIN32)
    list(APPEND EPOCH_APP_SOURCES src/app/application.rc src/app/resource.h src/app/application.ico)
    add_executable(epoch_integrated_opengl_scene WIN32 ${EPOCH_APP_SOURCES})
else()
    add_executable(epoch_integrated_opengl_scene ${EPOCH_APP_SOURCES})
endif()
target_sources(epoch_integrated_opengl_scene PRIVATE
    FILE_SET app_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/epoch/modules"
        FILES src/epoch/modules/epoch.app.ixx
)
target_link_libraries(epoch_integrated_opengl_scene PRIVATE epoch::render_spine)
target_compile_features(epoch_integrated_opengl_scene PRIVATE cxx_std_23)
set_target_properties(epoch_integrated_opengl_scene PROPERTIES
    CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO
    OUTPUT_NAME "epoch_integrated_opengl_scene"
    FOLDER "Epoch/Applications"
)
if(WIN32)
    target_compile_definitions(epoch_integrated_opengl_scene PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    set_target_properties(epoch_integrated_opengl_scene PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:epoch_integrated_opengl_scene>"
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(MSVC)
    foreach(target EpochGui epoch_render_spine epoch_integrated_opengl_scene)
        target_compile_options(${target} PRIVATE
            /W4 /permissive- /EHsc /utf-8
            /Zc:preprocessor /Zc:__cplusplus /Zc:inline /Zc:externConstexpr
            $<$<CONFIG:Release>:/O2>)
    endforeach()
else()
    foreach(target EpochGui epoch_render_spine epoch_integrated_opengl_scene)
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            $<$<CONFIG:Release>:-O2>)
    endforeach()
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES
    ${EPOCH_SPINE_SOURCES} ${EPOCH_SPINE_HEADERS} ${EPOCH_COMPAT_HEADERS} ${EPOCH_SPINE_MODULES}
    src/epoch/modules/epoch.app.ixx ${EPOCH_APP_SOURCES})

file(GLOB_RECURSE EPOCH_ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/assets" PREFIX "Assets" FILES ${EPOCH_ASSET_FILES})
target_sources(epoch_integrated_opengl_scene PRIVATE ${EPOCH_ASSET_FILES})
set_source_files_properties(${EPOCH_ASSET_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_custom_command(TARGET epoch_integrated_opengl_scene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:epoch_integrated_opengl_scene>/assets"
    COMMENT "Copying scene models, textures, shaders and GUI assets")

if(WIN32)
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT epoch_integrated_opengl_scene)
endif()

foreach(configuration Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${configuration}" configuration_upper)
    foreach(target epoch_render_spine epoch_integrated_opengl_scene)
        set_target_properties(${target} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${configuration_upper}" "${CMAKE_BINARY_DIR}/bin/${configuration}"
            "ARCHIVE_OUTPUT_DIRECTORY_${configuration_upper}" "${CMAKE_BINARY_DIR}/lib/${configuration}"
            "PDB_OUTPUT_DIRECTORY_${configuration_upper}" "${CMAKE_BINARY_DIR}/bin/${configuration}")
    endforeach()
endforeach()
