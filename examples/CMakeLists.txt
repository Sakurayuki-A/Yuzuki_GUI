add_executable(render_demo render_demo/main.cpp)
target_link_libraries(render_demo PRIVATE yuzuki)
add_custom_command(TARGET render_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:render_demo>/LexendDeca-Regular.ttf)

add_executable(layout_test layout_test/main.cpp)
target_link_libraries(layout_test PRIVATE yuzuki)

add_executable(codex_ui
    codex_ui/main.cpp
    codex_ui/components.cpp
    codex_ui/main_page.cpp)
target_link_libraries(codex_ui PRIVATE yuzuki)
add_custom_command(TARGET codex_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:codex_ui>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:codex_ui>/Phosphor.ttf)

add_executable(animation_demo
    animation_demo/main.cpp
    animation_demo/main_page.cpp)
target_link_libraries(animation_demo PRIVATE yuzuki)
add_custom_command(TARGET animation_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:animation_demo>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:animation_demo>/Phosphor.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/testimg.jpg
        $<TARGET_FILE_DIR:animation_demo>/testimg.jpg)

add_executable(window_demo
    window_demo/main.cpp
    window_demo/main_page.cpp)
target_link_libraries(window_demo PRIVATE yuzuki)
add_custom_command(TARGET window_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:window_demo>/LexendDeca-Regular.ttf)

add_executable(perf_demo
    perf_demo/main.cpp
    perf_demo/main_page.cpp)
target_link_libraries(perf_demo PRIVATE yuzuki)
add_custom_command(TARGET perf_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:perf_demo>/LexendDeca-Regular.ttf)

add_executable(icofont_demo
    icofont_demo/main.cpp
    icofont_demo/main_page.cpp)
target_link_libraries(icofont_demo PRIVATE yuzuki)
add_custom_command(TARGET icofont_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:icofont_demo>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:icofont_demo>/Phosphor.ttf)

add_executable(explorer
    explorer/main.cpp
    explorer/app.cpp)
target_link_libraries(explorer PRIVATE yuzuki)
add_custom_command(TARGET explorer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:explorer>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:explorer>/Phosphor.ttf)

add_executable(hello hello/main.cpp)
target_link_libraries(hello PRIVATE yuzuki)
add_custom_command(TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:hello>/LexendDeca-Regular.ttf)

# 5.2.5 App shell: icon + DPI/ComCtl manifest + canonical launch sequence via the
# reusable yuzuki_add_app_shell() helper (clone-and-start template).
add_executable(app_shell app_shell/main.cpp)
target_link_libraries(app_shell PRIVATE yuzuki)
yuzuki_add_app_shell(app_shell)

add_executable(api_validate api_validate/main.cpp)
target_link_libraries(api_validate PRIVATE yuzuki)
add_custom_command(TARGET api_validate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:api_validate>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/testimg.jpg
        $<TARGET_FILE_DIR:api_validate>/testimg.jpg)

add_executable(debug_demo debug_demo/main.cpp)
target_link_libraries(debug_demo PRIVATE yuzuki)
add_custom_command(TARGET debug_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:debug_demo>/LexendDeca-Regular.ttf)

add_executable(cookbook_settings cookbook_settings/main.cpp)
target_link_libraries(cookbook_settings PRIVATE yuzuki)
add_custom_command(TARGET cookbook_settings POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:cookbook_settings>/LexendDeca-Regular.ttf)

add_executable(cookbook_login cookbook_login/main.cpp)
target_link_libraries(cookbook_login PRIVATE yuzuki)
add_custom_command(TARGET cookbook_login POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:cookbook_login>/LexendDeca-Regular.ttf)

add_executable(cookbook_chat cookbook_chat/main.cpp)
target_link_libraries(cookbook_chat PRIVATE yuzuki)
add_custom_command(TARGET cookbook_chat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:cookbook_chat>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:cookbook_chat>/Phosphor.ttf)

add_executable(cookbook_dialogs cookbook_dialogs/main.cpp)
target_link_libraries(cookbook_dialogs PRIVATE yuzuki)
add_custom_command(TARGET cookbook_dialogs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:cookbook_dialogs>/LexendDeca-Regular.ttf)

add_executable(playground
    playground/main.cpp
    playground/app.cpp)
target_link_libraries(playground PRIVATE yuzuki)
add_custom_command(TARGET playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:playground>/LexendDeca-Regular.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:playground>/Phosphor.ttf)
