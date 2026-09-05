#include <yuzuki/yuzuki.hpp>

using namespace yzk;

int main() {
    auto& app = Application::instance();

    Window win("Debug Overlay", 480, 320);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    auto* root = new DockPanel;
    auto* column = new StackPanel(Orientation::Vertical);
    column->set_spacing(12.0f).set_padding(24.0f);
    column->set_stretch_children(true);
    root->dock(column, Dock::Fill);

    auto* title = new Label("Debug Tools Demo");
    title->bold(true);
    column->append_child(title);

    auto* hint = new Label("F1: stats overlay   F2: widget tree inspector");
    hint->text_role(TextRole::Secondary);
    column->append_child(hint);

    auto* counter = new Label("Count: 0");
    counter->text_role(TextRole::Secondary);
    column->append_child(counter);

    auto* bump = new Button("Bump the counter");
    int count = 0;
    bump->on_click([counter, &count]() {
        counter->set_text("Count: " + std::to_string(++count));
    });
    column->append_child(bump);

    auto* separator = new Box;
    separator->height(1.0f);
    separator->set_bg(Theme::get().border);
    column->append_child(separator);

    auto* clip_title = new Label("Clipboard (5.2.1)");
    clip_title->bold(true);
    column->append_child(clip_title);

    auto* clip_hint = new Label("Type below, then Ctrl+C to copy / Ctrl+V to paste / "
                                "Ctrl+X to cut. List: select a row, press Ctrl+C.");
    clip_hint->text_role(TextRole::Secondary);
    column->append_child(clip_hint);

    TextBoxConfig cfg;
    cfg.height = 32.0f;
    auto* clip_box = new TextBox("swap this text", cfg);
    column->append_child(clip_box);

    auto* cb_row = new StackPanel(Orientation::Horizontal);
    cb_row->set_spacing(8.0f);
    auto* copy_btn = new Button("Copy");
    auto* cut_btn = new Button("Cut");
    auto* paste_btn = new Button("Paste");
    auto* show_btn = new Button("Show clipboard");
    auto* clip_status = new Label("clipboard: (empty)");
    clip_status->text_role(TextRole::Secondary);
    auto refresh_status = [clip_status]() {
        const String s = clipboard::get_text();
        clip_status->set_text("clipboard: \"" + (s.empty() ? String("(empty)") : s) + "\"");
    };
    copy_btn->on_click([clip_box, clip_status, refresh_status]() {
        if (!clipboard::set_text(clip_box->text())) {
            clip_status->set_text("clipboard: (copy failed)");
        } else {
            refresh_status();
        }
    });
    cut_btn->on_click([clip_box, clip_status, refresh_status]() {
        clipboard::set_text(clip_box->text());
        clip_box->set_text("");
        clip_status->set_text("clipboard: (cut)");
        refresh_status();
    });
    paste_btn->on_click([clip_box, refresh_status]() {
        const String s = clipboard::get_text();
        if (!s.empty()) clip_box->set_text(s);
        refresh_status();
    });
    show_btn->on_click(refresh_status);
    cb_row->append_child(copy_btn);
    cb_row->append_child(cut_btn);
    cb_row->append_child(paste_btn);
    cb_row->append_child(show_btn);
    column->append_child(clip_status);
    column->append_child(cb_row);

    // A plain list whose rows are copied to the clipboard by Ctrl+C.
    auto* list = new ListView;
    list->set_items({"Apple", "Banana", "Cherry", "中文行", "Date"});
    list->set_on_selected([refresh_status](i32) { refresh_status(); });
    column->append_child(list);

    win.set_root(root);

    // General capabilities: attach the overlay + inspector to any window,
    // F1 / F2 toggle them.
    auto* overlay = new DebugOverlay;
    win.set_debug_overlay(overlay);
    auto* inspector = new WidgetInspector;
    win.set_widget_inspector(inspector);

    win.show();
    return app.run();
}