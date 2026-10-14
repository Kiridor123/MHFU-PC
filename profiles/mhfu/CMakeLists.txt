cmake_minimum_required(VERSION 3.20)

set(MHFU_PROFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Configuring Monster Hunter Freedom Unite profile (ULUS-10391)")

# The AOT corpus is derived from the user's executable and is not committed.
file(GLOB MHFU_GENERATED CONFIGURE_DEPENDS "${MHFU_PROFILE_DIR}/generated/*.cpp")

if(MHFU_GENERATED)
    list(LENGTH MHFU_GENERATED MHFU_GENERATED_COUNT)
    message(STATUS "mhfu: ${MHFU_GENERATED_COUNT} generated AOT units found")
    if(MSVC)
        set(MHFU_GENERATED_OPTIONS "/O2;/bigobj")
    else()
        set(MHFU_GENERATED_OPTIONS "-O2;-g0")
    endif()
    set_source_files_properties(${MHFU_GENERATED} PROPERTIES COMPILE_OPTIONS "${MHFU_GENERATED_OPTIONS}")
    add_library(mhfu_generated OBJECT ${MHFU_GENERATED})
    target_include_directories(mhfu_generated PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${MHFU_PROFILE_DIR}/generated")
    set_target_properties(mhfu_generated PROPERTIES JOB_POOL_COMPILE psprecomp_generated)
else()
    message(STATUS "mhfu: no generated AOT units found yet in profiles/mhfu/generated; using stub")
    add_library(mhfu_generated OBJECT host/generated_stub.cpp)
    target_include_directories(mhfu_generated PRIVATE
        "${PROJECT_SOURCE_DIR}/include")
endif()

# Embedded NID table
set(MHFU_NID_TABLE "${CMAKE_CURRENT_BINARY_DIR}/generated_nids/nid_table.inc")
add_custom_command(
    OUTPUT "${MHFU_NID_TABLE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated_nids"
    COMMAND ${CMAKE_COMMAND} -DINPUT=${PROJECT_SOURCE_DIR}/configs/nids.csv
                             -DOUTPUT=${MHFU_NID_TABLE}
                             -P "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/tools/embed_nids.cmake"
    DEPENDS "${PROJECT_SOURCE_DIR}/configs/nids.csv"
            "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/tools/embed_nids.cmake"
    COMMENT "Embedding PSP NID database")
add_custom_target(mhfu_nids DEPENDS "${MHFU_NID_TABLE}")

# Version header
set(MHFU_VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated_version/yakumo_version.hpp")
add_custom_target(mhfu_version
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DOUTPUT=${MHFU_VERSION_HEADER}
            -P "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/tools/write_version.cmake"
    BYPRODUCTS "${MHFU_VERSION_HEADER}"
    COMMENT "Checking build version")

# Generated hooks stubs
set(MHFU_HUD_INC "${CMAKE_CURRENT_BINARY_DIR}/generated_hooks/hud_functions.inc")
set(MHFU_LAYERED_INC "${CMAKE_CURRENT_BINARY_DIR}/generated_hooks/layered_armor_functions.inc")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated_hooks")
if(NOT EXISTS "${MHFU_HUD_INC}")
    file(WRITE "${MHFU_HUD_INC}" "// MHFU: HUD function hooks stub\n")
endif()
if(NOT EXISTS "${MHFU_LAYERED_INC}")
    file(WRITE "${MHFU_LAYERED_INC}" "// MHFU: Layered armor function hooks stub\n")
endif()

# Shaders
find_program(MHFU_GLSLANG NAMES glslangValidator glslang)
if(MHFU_GLSLANG)
    set(MHFU_SHADER_INC "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders/ge_shaders.inc")
    set(MHFU_SHADERS
        "kGeVertexShader=${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/shaders/ge.vert"
        "kGeRawVertexShader=GE_RAW_VERTICES@${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/shaders/ge.vert"
        "kGeCheckVertexShader=GE_RAW_VERTICES,GE_CHECK_DECODE@${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/shaders/ge.vert"
        "kGeFragmentShader=${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/shaders/ge.frag"
        "kGeFragmentShaderPlain=GE_NO_SPECIALIZATION@${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/shaders/ge.frag")
    set(MHFU_SHADER_SOURCES ${MHFU_SHADERS})
    list(TRANSFORM MHFU_SHADER_SOURCES REPLACE "^[A-Za-z]+=([A-Z_,]+@)?" "")
    list(REMOVE_DUPLICATES MHFU_SHADER_SOURCES)
    add_custom_command(
        OUTPUT "${MHFU_SHADER_INC}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders"
        COMMAND python "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/tools/embed_shaders.py" "${MHFU_GLSLANG}" "${MHFU_SHADER_INC}"
                ${MHFU_SHADERS}
        DEPENDS ${MHFU_SHADER_SOURCES}
                "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/tools/embed_shaders.py"
        COMMENT "Compiling PSP GE shaders to SPIR-V")
    add_custom_target(mhfu_shaders DEPENDS "${MHFU_SHADER_INC}")
endif()

# FFmpeg for ATRAC3 and video decoding
include("${PROJECT_SOURCE_DIR}/profiles/mhp3rd/cmake/FFmpeg.cmake")

# Renderer: SDL3 and Vulkan
find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

set(MHFU_HOST_SOURCES
    host/main.cpp
    host/mhfu_profile.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/app_paths.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/platform/utf8_path.cpp
    ${MHFU_PROFILE_DIR}/host/overlays.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/camera_probe.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/camera/camera_input.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/camera/game_aspect.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/camera/game_camera.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/camera/lock_on.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/camera/free_camera.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/game_hud.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/kernel.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/load_trace.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/fast_loading.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/fast_forward.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/fast_forward_switch.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/load_detector.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/kernel/iso_image.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_common.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_threadman.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_sysmem.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_io.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_system.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_media.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_atrac.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_mpeg.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_font.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_utility.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_savedata.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/hle/hle_adhoc.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/audio/atrac_decoder.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/audio/audio_sink.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/audio/sas_core.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/audio/guest_pcm.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/movie/avc_decoder.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/movie/psmf_demuxer.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/fonts/game_font.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/ge_state.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/texture_decode.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/descriptor_pools.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/vulkan_renderer.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/mmpx.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/ui_textures.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/frame_pacing.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/frame_interpolation.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/replacement_textures.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/screenshot.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/texture_pack.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/texture_pack_import.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/gpu/device_report.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/bindings.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/chords.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/presets.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/gamepad_mapping.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/gamepad_devices.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/touch_action.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/input/touch_controls.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/perf/frame_stats.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/perf/perf_overlay.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/settings/settings.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/text/language.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/text/translation.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/file_overlay.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mod_import.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mod_ini.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mod_library.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mod_session.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mhp3rd_data_bin.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mhp3rd_mod_format.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/mods/mhp3rd_mods.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/game/guest_ram.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/game/game_data.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/game/equipment_models.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/game/layered_armor.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/game/layered_armor_hook.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/save_data/aes128.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/save_data/param_sfo.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/save_data/savedata_crypto.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/save_data/savedata_store.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/save_data/save_transfer.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/adhoc/client.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/adhoc/discovery.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/adhoc/host.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/adhoc/server.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/install/user_data.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/install/executable_preparation.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/install/front_end.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/install/installer.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/install/installer_dialogs.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/bindings_editor.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/controllers_screen.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/file_browser.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/font_menu.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/input_script.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/layer.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/menu.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/layered_armor_screen.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/mods_screen.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/save_screen.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/setup_screens.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/text_input.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/touch_editor.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/touch_overlay.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/texture_pack_screen.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/translation_screen.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/ui/widgets.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/tiny_aes/aes.c
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui/backends/imgui_impl_sdl3.cpp
    ${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui/backends/imgui_impl_vulkan.cpp
)
set_source_files_properties(${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/tiny_aes/aes.c PROPERTIES LANGUAGE CXX)

add_executable(MHFU
    ${MHFU_HOST_SOURCES}
    $<TARGET_OBJECTS:mhfu_generated>
)

add_dependencies(MHFU mhfu_nids mhfu_version)
if(MHFU_GLSLANG)
    add_dependencies(MHFU mhfu_shaders)
endif()

mhp3rd_use_ffmpeg(MHFU)

target_include_directories(MHFU PRIVATE
    "${MHFU_PROFILE_DIR}/host"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host"
    "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party"
    "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/third_party/imgui"
    "${MHFU_PROFILE_DIR}/generated"
    "${CMAKE_CURRENT_BINARY_DIR}/generated_nids"
    "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders"
    "${CMAKE_CURRENT_BINARY_DIR}/generated_version"
    "${CMAKE_CURRENT_BINARY_DIR}/generated_hooks"
)

target_compile_definitions(MHFU PRIVATE
    MHP3RD_HAS_RENDERER=1
    MHP3RD_HAS_SDL_AUDIO=1
    MHP3RD_HAS_SDL=1
)

target_link_libraries(MHFU PRIVATE
    psprecomp_core
    SDL3::SDL3
    Vulkan::Vulkan
)

if(WIN32)
    target_link_libraries(MHFU PRIVATE ws2_32 iphlpapi)
    if(MINGW)
        target_link_options(MHFU PRIVATE "-Wl,--out-implib,${CMAKE_CURRENT_BINARY_DIR}/libMHFU.dll.a" "-Wl,--export-all-symbols")
    endif()
endif()

set_target_properties(MHFU PROPERTIES
    ENABLE_EXPORTS TRUE
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME MHFU
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set(MHFU_OVERLAY_DIR "${CMAKE_BINARY_DIR}/bin/overlays")

function(mhfu_add_overlay meta_path)
    get_filename_component(source_dir "${meta_path}" DIRECTORY)
    get_filename_component(MHP3RD_OVERLAY_PREFIX "${source_dir}" NAME)
    file(STRINGS "${meta_path}" meta_lines REGEX "^(base|size|code_size|hash|name)=")
    foreach(line IN LISTS meta_lines)
        string(REGEX MATCH "^([a-z_]+)=(.+)$" matched "${line}")
        if(NOT matched)
            continue()
        endif()
        string(TOUPPER "${CMAKE_MATCH_1}" key)
        set(MHP3RD_OVERLAY_${key} "${CMAKE_MATCH_2}")
    endforeach()
    if(NOT MHP3RD_OVERLAY_BASE OR NOT MHP3RD_OVERLAY_SIZE OR NOT MHP3RD_OVERLAY_CODE_SIZE
       OR NOT MHP3RD_OVERLAY_HASH OR NOT MHP3RD_OVERLAY_NAME)
        message(WARNING "mhfu: incomplete overlay metadata in ${meta_path}")
        return()
    endif()

    file(GLOB sources CONFIGURE_DEPENDS "${source_dir}/*.cpp")
    if(NOT sources)
        message(WARNING "mhfu: no sources for overlay ${MHP3RD_OVERLAY_PREFIX}")
        return()
    endif()
    set(entry_point "${CMAKE_CURRENT_BINARY_DIR}/overlays/${MHP3RD_OVERLAY_PREFIX}_module.cpp")
    configure_file("${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host/overlay_module.cpp.in" "${entry_point}" @ONLY)

    set(target "overlay_${MHP3RD_OVERLAY_PREFIX}")
    add_library(${target} MODULE ${sources} "${entry_point}")
    target_include_directories(${target} PRIVATE
        "${source_dir}"
        "${MHFU_PROFILE_DIR}/host"
        "${PROJECT_SOURCE_DIR}/profiles/mhp3rd/host"
        "${PROJECT_SOURCE_DIR}/include")
    if(WIN32)
        target_link_libraries(${target} PRIVATE MHFU)
    else()
        add_dependencies(${target} MHFU)
    endif()
    target_compile_options(${target} PRIVATE -O${PSPRECOMP_GENERATED_OPT_LEVEL} -g0)
    set_target_properties(${target} PROPERTIES
        JOB_POOL_COMPILE psprecomp_generated
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        PREFIX ""
        SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
        OUTPUT_NAME "${MHP3RD_OVERLAY_PREFIX}"
        LIBRARY_OUTPUT_DIRECTORY "${MHFU_OVERLAY_DIR}")
endfunction()

file(GLOB MHFU_OVERLAY_METADATA CONFIGURE_DEPENDS "${MHFU_PROFILE_DIR}/overlays/*/meta.txt")
foreach(meta IN LISTS MHFU_OVERLAY_METADATA)
    mhfu_add_overlay("${meta}")
endforeach()
list(LENGTH MHFU_OVERLAY_METADATA MHFU_OVERLAY_COUNT)
message(STATUS "mhfu: ${MHFU_OVERLAY_COUNT} overlay librar(ies) in ${MHFU_OVERLAY_DIR}")

