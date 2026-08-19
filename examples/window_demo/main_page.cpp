#include "main_page.hpp"

#include <cstdio>

namespace yzk {

namespace {

// Caption buttons: minimize / maximize-restore / close
struct CaptionButton : Button {
    Window& win;
    CaptionButton(const String& text, Window& window) : Button(text), win(window) {
        set_accent(false);
        set_min_width(40.0f);
    }
};

struct MinButton : CaptionButton {
    using CaptionButton::CaptionButton;
    void on_click() override { win.minimize(); }
};

struct MaxButton : CaptionButton {
    using CaptionButton::CaptionButton;
    void on_click() override {
        win.maximize_toggle();
        set_text(win.maximized() ? "-" : "+");
        invalidate();
    }
};

struct CloseButton : CaptionButton {
    using CaptionButton::CaptionButton;
    void on_click() override { win.close(); }
};

struct SecondCloseButton : Button {
    Window& win;
    explicit SecondCloseButton(Window& window) : Button("Close"), win(window) {}
    void on_click() override { win.close(); }
};

// Second window (with a system frame)
struct SecondWindow : Window {
    SecondWindow() : Window("Second Window", 480, 320) {}

    void open() {
        if (!is_created()) {
            if (!create()) return;
            wchar_t exe_path[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
            const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
            backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
            set_root(make_page());
        }
        show();
    }

    Widget* make_page() {
        auto root = new DockPanel;
        auto body = new FlexBox(Orientation::Vertical);
        body->set_padding(24.0f);
        body->set_spacing(10.0f);
        body->set_align_cross(FlexCrossAlign::Stretch);
        root->dock(body, Dock::Fill);

        auto title = new Label("Second Window");
        title->set_bold(true);
        title->set_align(TextAlignH::Left, TextAlignV::Center);
        body->append_child(title);

        auto hint = new Label("A normal framed window, coexisting with the borderless one.");
        hint->set_text_role(TextRole::Secondary);
        hint->set_small(true);
        hint->set_align(TextAlignH::Left, TextAlignV::Center);
        body->append_child(hint);

        auto close = new SecondCloseButton(*this);
        body->append_child(close);
        return root;
    }
};

struct OpenSecondButton : Button {
    SecondWindow* second = nullptr;
    OpenSecondButton() : Button("Open second window") { second = new SecondWindow; }
    void on_click() override { second->open(); }
};

}  // namespace

Widget* make_window_demo_page(Window& window) {
    auto root = new DockPanel;

    // Background
    auto bg = new Box;
    bg->set_bg(Color{0x1a, 0x1a, 0x1c});
    root->dock(bg, Dock::Fill);

    // Custom title bar (drag the empty area to move the window)
    auto caption = new Box;
    caption->set_bg(Color{0x22, 0x22, 0x26});
    root->dock(caption, Dock::Top);
    window.set_caption(caption);

    auto cap_row = new FlexBox;
    cap_row->set_padding(8.0f);
    cap_row->set_spacing(6.0f);
    caption->append_child(cap_row);

    auto cap_title = new Label("Borderless Window - drag this bar");
    cap_title->set_text_role(TextRole::Secondary);
    cap_title->set_small(true);
    cap_title->set_align(TextAlignH::Left, TextAlignV::Center);
    cap_title->set_flex_grow(1.0f);
    cap_row->append_child(cap_title);

    auto min = new MinButton("-", window);
    cap_row->append_child(min);
    auto max = new MaxButton("+", window);
    cap_row->append_child(max);
    auto close = new CloseButton("x", window);
    cap_row->append_child(close);

    // Content
    auto body = new FlexBox(Orientation::Vertical);
    body->set_padding(24.0f);
    body->set_spacing(10.0f);
    body->set_align_cross(FlexCrossAlign::Stretch);
    root->dock(body, Dock::Fill);

    auto title = new Label("Window Foundation");
    title->set_bold(true);
    title->set_align(TextAlignH::Left, TextAlignV::Center);
    body->append_child(title);

    auto hint = new Label(
        "Borderless window: drag the caption bar to move, pull the 8px edges to resize, "
        "maximize keeps the taskbar visible.");
    hint->set_text_role(TextRole::Secondary);
    hint->set_small(true);
    hint->set_align(TextAlignH::Left, TextAlignV::Center);
    body->append_child(hint);

    auto open = new OpenSecondButton;
    body->append_child(open);

    return root;
}

}  // namespace yzk