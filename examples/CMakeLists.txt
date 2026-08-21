add_executable(hello hello/main.cpp)
target_link_libraries(hello PRIVATE yuzuki)
add_custom_command(TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:hello>/Satoshi-Regular.otf)

add_executable(render_demo render_demo/main.cpp)
target_link_libraries(render_demo PRIVATE yuzuki)
add_custom_command(TARGET render_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:render_demo>/Satoshi-Regular.otf)

add_executable(layout_test layout_test/main.cpp)
target_link_libraries(layout_test PRIVATE yuzuki)

add_executable(codex_ui
    codex_ui/main.cpp
    codex_ui/components.cpp
    codex_ui/main_page.cpp)
target_link_libraries(codex_ui PRIVATE yuzuki)
add_custom_command(TARGET codex_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:codex_ui>/Satoshi-Regular.otf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:codex_ui>/Phosphor.ttf)

add_executable(animation_demo
    animation_demo/main.cpp
    animation_demo/main_page.cpp)
target_link_libraries(animation_demo PRIVATE yuzuki)
add_custom_command(TARGET animation_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:animation_demo>/Satoshi-Regular.otf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:animation_demo>/Phosphor.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/testimg.jpg
        $<TARGET_FILE_DIR:animation_demo>/testimg.jpg)

add_executable(transform_demo
    transform_demo/main.cpp
    transform_demo/main_page.cpp)
target_link_libraries(transform_demo PRIVATE yuzuki)
add_custom_command(TARGET transform_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:transform_demo>/Satoshi-Regular.otf)

add_executable(controls_demo
    controls_demo/main.cpp
    controls_demo/main_page.cpp)
target_link_libraries(controls_demo PRIVATE yuzuki)
add_custom_command(TARGET controls_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:controls_demo>/Satoshi-Regular.otf)

add_executable(window_demo
    window_demo/main.cpp
    window_demo/main_page.cpp)
target_link_libraries(window_demo PRIVATE yuzuki)
add_custom_command(TARGET window_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:window_demo>/Satoshi-Regular.otf)

add_executable(perf_demo
    perf_demo/main.cpp
    perf_demo/main_page.cpp)
target_link_libraries(perf_demo PRIVATE yuzuki)
add_custom_command(TARGET perf_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:perf_demo>/Satoshi-Regular.otf)

add_executable(icofont_demo
    icofont_demo/main.cpp
    icofont_demo/main_page.cpp)
target_link_libraries(icofont_demo PRIVATE yuzuki)
add_custom_command(TARGET icofont_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:icofont_demo>/Satoshi-Regular.otf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:icofont_demo>/Phosphor.ttf)

add_executable(notification_demo
    notification_demo/main.cpp
    notification_demo/main_page.cpp)
target_link_libraries(notification_demo PRIVATE yuzuki)
add_custom_command(TARGET notification_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Satoshi-Regular.otf
        $<TARGET_FILE_DIR:notification_demo>/Satoshi-Regular.otf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/icofonts/regular/Phosphor.ttf
        $<TARGET_FILE_DIR:notification_demo>/Phosphor.ttf)
