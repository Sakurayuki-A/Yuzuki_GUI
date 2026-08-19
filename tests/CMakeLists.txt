add_executable(yuzuki_tests test_core.cpp)
target_link_libraries(yuzuki_tests PRIVATE yuzuki)
add_test(NAME core COMMAND yuzuki_tests)