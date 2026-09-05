#include <yuzuki/yuzuki.hpp>

#include <fstream>

using namespace yzk;

namespace {

String read_file(const String& path) {
    std::ifstream f(utf::to_wide(path), std::ios::binary);
    if (!f) return {};
    std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return data;
}

bool write_file(const String& path, const String& text) {
    std::ofstream f(utf::to_wide(path), std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << text;
    return f.good();
}

}  // namespace

// A tiny text "editor" built entirely on framework controls and dialogs:
// open a file via the native open dialog, edit with the TextBox, save via the
// native save dialog, and confirm exit with the framework MessageBox. No Win32
// calls appear in app code.
int main() {
    auto& app = Application::instance();

    Window win("Dialogs Cookbook", 640, 480);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    auto* root = new DockPanel;
    auto* column = new DockPanel;
    column->set_padding(24.0f);
    root->dock(column, Dock::Fill);

    auto* top = new StackPanel(Orientation::Vertical);
    top->set_spacing(12.0f);
    column->dock(top, Dock::Top);

    auto* title = new Label("Dialogs Cookbook");
    title->bold(true);
    top->append_child(title);

    auto* hint = new Label("Open a text file, edit it, then save. Exit asks for confirmation.");
    hint->text_role(TextRole::Secondary);
    top->append_child(hint);

    auto* status = new Label("no file loaded");
    status->text_role(TextRole::Secondary);
    top->append_child(status);

    auto* bottom = new StackPanel(Orientation::Horizontal);
    bottom->set_spacing(8.0f);
    column->dock(bottom, Dock::Bottom);

    TextBoxConfig cfg;
    cfg.mode = TextBoxMode::MultiLine;
    cfg.enter_submits = false;
    auto* editor = new TextBox("", cfg);
    column->dock(editor, Dock::Fill);

    String current_path;

    auto* open_btn = new Button("Open...");
    auto* save_btn = new Button("Save...");
    auto* quit_btn = new Button("Quit");

    auto on_open = [&]() {
        FileDialogOptions opts;
        opts.filters = {{"Text files", "*.txt;*.md"}, {"All files", "*.*"}};
        auto paths = open_file_dialog(&win, opts);
        if (paths.empty()) {
            status->set_text("open cancelled");
            return;
        }
        current_path = paths[0];
        editor->set_text(read_file(current_path));
        status->set_text("loaded: " + current_path);
    };

    auto on_save = [&]() {
        FileDialogOptions opts;
        opts.filters = {{"Text files", "*.txt"}, {"All files", "*.*"}};
        opts.default_name = current_path.empty() ? "untitled.txt" : current_path;
        String path = save_file_dialog(&win, opts);
        if (path.empty()) {
            status->set_text("save cancelled");
            return;
        }
        current_path = path;
        if (write_file(current_path, editor->text())) {
            status->set_text("saved: " + current_path);
        } else {
            MessageBox m;
            m.show(win, "Save failed", "Could not write the file.", MessageBoxKind::Info);
        }
    };

    // Window accelerators: Ctrl+S/Ctrl+O work anywhere in the window (5.2.3).
    win.add_accelerator(Accelerator{static_cast<u32>('S'), KeyModifier_Control, on_save})
        .add_accelerator(Accelerator{static_cast<u32>('O'), KeyModifier_Control, on_open});

    open_btn->on_click(on_open);
    save_btn->on_click(on_save);

    quit_btn->on_click([&]() {
        MessageBox m;
        m.show(win, "Quit", "Quit the editor?", MessageBoxKind::YesNo,
               [&](MessageBoxResult r) {
                   if (r == MessageBoxResult::Yes) win.close();
               });
    });

    bottom->append_child(open_btn);
    bottom->append_child(save_btn);
    bottom->append_child(quit_btn);

    win.set_root(root);
    win.show();
    return app.run();
}