// App-shell template: the normative Yuzuki launch sequence (Phase 5.2.5).
//
// Run this with resource app.rc + app.manifest attached (see yuzuki_add_app_shell
// in examples/CMakeLists.txt): the exe then has an icon in Explorer, is
// PerMonitorV2 DPI aware (manifest + Application), and links the v6 common controls.
//
// The launch sequence is the canonical entry point for every Yuzuki app:
//   1. Application::instance()        -> DPI awareness + module handle
//   2. Window constructor            -> logical size in DIPs
//   3. window.create()               -> HWND + render target (per-monitor DPI)
//   4. backend().add_font_file(...)  -> register the app font
//   5. window.set_root(...)          -> build the widget tree
//   6. window.show(); app.run()      -> pump messages + render on demand
#include <yuzuki/yuzuki.hpp>

using namespace yzk;

int main() {
    auto& app = Application::instance();

    Window window("App Shell", 520, 360);

    if (!window.create()) return 1;

    // The shell manifest is DPI-aware; the font is co-located by the CMake post-build.
    window.backend().add_font_file("LexendDeca-Regular.ttf");

    auto root = new FlexBox(Orientation::Vertical);
    root->set_padding(28.0f);
    root->set_spacing(12.0f);
    root->set_align_cross(FlexCrossAlign::Stretch);
    window.set_root(root);

    auto title = new Label("App Shell Template");
    title->set_bold(true);
    root->append_child(title);

    auto hint = new Label(
        "This exe carries the framework app.rc + app.manifest resources: an icon in "
        "Explorer, PerMonitorV2 DPI awareness, and the v6 common controls. See "
        "cmake/yuzuki_add_app_shell.cmake for how to attach the shell to any target.");
    hint->set_text_role(TextRole::Secondary);
    hint->set_small(true);
    hint->set_align(TextAlignH::Left, TextAlignV::Top);
    hint->set_flex_grow(1.0f);
    root->append_child(hint);

    auto run_btn = new Button("Run");
    run_btn->set_accent(true);
    root->append_child(run_btn);

    auto quit = new Button("Close");
    root->append_child(quit);
    quit->on_click([&]() { window.close(); });

    window.show();
    return app.run();
}