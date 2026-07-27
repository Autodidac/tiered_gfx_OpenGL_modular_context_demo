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
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EPOCH_GENERATED_ASSET_ROOT}/assets"
            "$<TARGET_FILE_DIR:epoch_integrated_opengl_scene>/assets"
    COMMENT "Copying scene assets and exact reconstructed OBJ models")

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
