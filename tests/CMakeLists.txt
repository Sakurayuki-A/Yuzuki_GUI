add_executable(yuzuki_tests test_core.cpp)
target_link_libraries(yuzuki_tests PRIVATE yuzuki)
add_test(NAME core COMMAND yuzuki_tests)

add_executable(yuzuki_visual_tests test_visual.cpp)
target_link_libraries(yuzuki_visual_tests PRIVATE yuzuki ole32 windowscodecs)
add_test(NAME visual COMMAND yuzuki_visual_tests)
add_custom_command(TARGET yuzuki_visual_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/uiFonts/regular/LexendDeca-Regular.ttf
        $<TARGET_FILE_DIR:yuzuki_visual_tests>/LexendDeca-Regular.ttf)
