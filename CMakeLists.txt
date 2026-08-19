cmake_minimum_required(VERSION 3.20)

project(YuzukiUI
    VERSION 0.2.0
    DESCRIPTION "A low-footprint retained-mode C++ UI framework"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(yuzuki STATIC
    src/application.cpp
    src/animation.cpp
    src/theme.cpp
    src/paint.cpp
    src/widget.cpp
    src/window.cpp
    src/ui/window_frame.cpp
    src/ui/window_input.cpp
    src/ui/window_borderless.cpp
    src/ui/window_wndproc.cpp
    src/overlay.cpp
    src/controls/label.cpp
    src/controls/button.cpp
    src/controls/stack_panel.cpp
    src/controls/flex_box.cpp
    src/controls/box.cpp
    src/controls/layout.cpp
    src/controls/grid_panel.cpp
    src/controls/dock_panel.cpp
    src/controls/wrap_panel.cpp
    src/controls/text_box.cpp
    src/controls/combo_box.cpp
    src/controls/check_box.cpp
    src/controls/toggle_switch.cpp
    src/controls/slider.cpp
    src/controls/progress_bar.cpp
    src/controls/spin_box.cpp
    src/controls/radio_button.cpp
    src/controls/scroll_view.cpp
    src/controls/list_view.cpp
    src/controls/icon.cpp
    src/controls/notification.cpp
    src/controls/image.cpp
    src/controls/tooltip.cpp
    src/controls/context_menu.cpp
    src/controls/backdrop_blur.cpp
    src/render/d2d/d2d_backend.cpp
    src/render/d2d/d2d_text.cpp
    src/render/d2d/d2d_bitmap.cpp
    src/render/d2d/d2d_shapes.cpp
    src/render/d2d/d2d_effects.cpp
    src/render/d2d/sweep_effect.cpp)

target_include_directories(yuzuki PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_features(yuzuki PUBLIC cxx_std_17)

target_link_libraries(yuzuki PUBLIC d2d1 dwrite dxgi d3d11 d3dcompiler winmm)

if(MSVC)
    target_compile_options(yuzuki PRIVATE /W4 /permissive- /utf-8 /EHsc)
    target_compile_options(yuzuki PUBLIC /utf-8)
else()
    target_compile_options(yuzuki PRIVATE -Wall -Wextra)
endif()

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE _WIN32_WINNT=0x0A00)
endif()

enable_testing()
add_subdirectory(examples)
add_subdirectory(tests)