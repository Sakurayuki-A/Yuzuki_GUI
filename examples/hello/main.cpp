#include <yuzuki/yuzuki.hpp>

using namespace yzk;

int main() {
    auto& app = Application::instance();

    Window win("Hello", 480, 320);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    auto* root = new DockPanel;
    auto* column = new StackPanel(Orientation::Vertical);
    column->set_spacing(12.0f).set_padding(24.0f);
    column->set_stretch_children(true);
    root->dock(column, Dock::Fill);

    auto* title = new Label("Hello, Yuzuki");
    title->set_bold(true);
    column->append_child(title);

    auto* hint = new Label("Type something and press Go.");
    hint->set_text_role(TextRole::Secondary);
    column->append_child(hint);

    auto* input = new TextBox(String(), TextBoxConfig{});
    input->set_placeholder("Your name...");
    column->append_child(input);

    auto* echo = new Label("");
    echo->set_text_role(TextRole::Secondary);
    column->append_child(echo);

    auto* go = new Button("Go");
    go->set_on_click([input, echo, &win]() {
        const String text = input->text();
        echo->set_text(text.empty() ? "Please type something." : "Hello, " + text + "!");
        win.invalidate_all();
    });
    column->append_child(go);

    auto* theme_toggle = new Button("Theme: Dark");
    theme_toggle->set_accent(false);
    bool dark = true;
    theme_toggle->set_on_click([&win, &dark, theme_toggle]() {
        dark = !dark;
        Theme::set(dark ? Theme::make_dark() : Theme::make_light());
        win.invalidate_all();
        theme_toggle->set_text(dark ? "Theme: Dark" : "Theme: Light");
    });
    column->append_child(theme_toggle);

    win.set_root(root);
    win.show();

    return app.run();
}