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
