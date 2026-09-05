# Yuzuki app-shell CMake helper.
#
# A "clone and start" app template: attach the framework-managed application
# icon + DPI/ComCtl-v6 manifest to an executable and copy the required fonts
# next to it. Usage:
#
#   add_executable(my_app main.cpp)
#   target_link_libraries(my_app PRIVATE yuzuki)
#   yuzuki_add_app_shell(my_app)
#
# Optional args:
#   FONTS  <path.ttf>...   extra fonts copied next to the exe (relative to
#                          ${CMAKE_SOURCE_DIR} when not absolute)
#
# The manifest enables Per-Monitor V2 DPI awareness and the v6 Common Controls,
# matching what Application::instance() sets up at runtime; apps that use
# yuzuki_add_app_shell get the shell for both explorer and the title bar.

function(yuzuki_add_app_shell target)
    cmake_parse_arguments(SHELL "" "" "FONTS" ${ARGN})

    set(app_shell_res_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../resources/app-shell")
    if(NOT EXISTS "${app_shell_res_dir}/app.rc")
        message(FATAL_ERROR "yuzuki_add_app_shell: resources/app-shell not found")
    endif()

    target_sources(${target} PRIVATE "${app_shell_res_dir}/app.rc")

    # Route the include path so the .rc resolves "yuzuki.ico".
    set_property(SOURCE "${app_shell_res_dir}/app.rc" PROPERTY
        VS_RC_INCLUDE "${app_shell_res_dir}")

    # Merge the DPI/ComCtl manifest into the app embed (coexists with the MSVC
    # runtime manifest under the same RT_MANIFEST id). MSVC-linking generators:
    # /MANIFESTINPUT merges the file with the linker-generated manifest.
    if(MSVC)
        target_link_options(${target} PRIVATE
            "/MANIFESTINPUT:${app_shell_res_dir}/app.manifest")
    endif()

    target_compile_definitions(${target} PRIVATE
        _UNICODE UNICODE)

    # Copy the framework fonts the shell expects next to the exe (post-build).
    set(shell_fonts
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf>")
    if(DEFINED SHELL_FONTS)
        foreach(font IN LISTS SHELL_FONTS)
            if(NOT IS_ABSOLUTE "${font}")
                set(font "${CMAKE_SOURCE_DIR}/${font}")
            endif()
            list(APPEND shell_fonts "$<BUILD_INTERFACE:${font}>")
        endforeach()
    endif()
    set(copy_cmds "")
    foreach(font IN LISTS shell_fonts)
        list(APPEND copy_cmds
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${font}"
            "$<TARGET_FILE_DIR:${target}>"
        )
    endforeach()
    add_custom_command(TARGET ${target} POST_BUILD ${copy_cmds})
endfunction()