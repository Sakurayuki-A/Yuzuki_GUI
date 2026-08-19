#include <yuzuki/yuzuki.hpp>

#include <functional>
#include <memory>
#include <string>

using namespace yzk;

namespace {

class ActionButton : public Button {
public:
    ActionButton(String text, std::function<void()> action)
        : Button(std::move(text)), action_(std::move(action)) {}

    void on_click() override {
        if (action_) action_();
    }

private:
    std::function<void()> action_;
};

class ProgressSlider : public Slider {
public:
    ProgressSlider(ProgressBar* bar, Label* label) : bar_(bar), label_(label) {}

    void set_spin(SpinBox* spin) { spin_ = spin; }

    void on_changed(f32 value) override {
        const f32 frac = value / 100.0f;
        bar_->set_value(frac);
        if (spin_) spin_->set_value(value);
        label_->set_text(std::to_string(static_cast<int>(value)) + "%");
    }

private:
    ProgressBar* bar_;
    SpinBox* spin_ = nullptr;
    Label* label_;
};

class ItemList : public ListView {
public:
    ItemList(Label* label) : label_(label) {}

    void on_selected(i32 index) override {
        if (index < 0) return;
        label_->set_text("Selected: " + items()[static_cast<size_t>(index)]);
    }

private:
    Label* label_;
};

class DialogOverlay : public Overlay {
public:
    DialogOverlay() {
        const RectF panel = RectF::make(110.0f, 330.0f, 300.0f, 200.0f);
        set_panel_rect(panel);
        title_ = new Label("Overlay Dialog");
        title_->set_bold(true);
        title_->set_bounds(RectF::make(panel.left + 20.0f, panel.top + 18.0f,
                                       panel.width() - 40.0f, 26.0f));
        body_ = new Label("A modal overlay on top of the window.\n"
                          "Click outside, press Esc, or hit a button to close.");
        body_->set_bounds(RectF::make(panel.left + 20.0f, panel.top + 54.0f,
                                      panel.width() - 40.0f, 60.0f));
        ok_ = new ActionButton("OK", [this] { close(); });
        ok_->set_bounds(RectF::make(panel.left + 20.0f, panel.bottom - 50.0f, 90.0f, 34.0f));
        cancel_ = new ActionButton("Cancel", [this] { close(); });
        cancel_->set_accent(false);
        cancel_->set_bounds(RectF::make(panel.right - 110.0f, panel.bottom - 50.0f, 90.0f, 34.0f));
        append_child(title_);
        append_child(body_);
        append_child(ok_);
        append_child(cancel_);
    }

    ~DialogOverlay() override {
        delete title_;
        delete body_;
        delete ok_;
        delete cancel_;
    }

private:
    Label* title_ = nullptr;
    Label* body_ = nullptr;
    Button* ok_ = nullptr;
    Button* cancel_ = nullptr;
};

}  // namespace

int main() {
    Application& app = Application::instance();

    Window window("YuzukiUI Hello", 520, 860);
    if (!window.create()) return 1;

    {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
        window.backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
    }

    auto root = std::make_unique<StackPanel>(Orientation::Vertical);
    root->set_padding(16.0f);
    root->set_spacing(10.0f);

    auto title = std::make_unique<Label>("YuzukiUI Controls");
    title->set_bold(true);

    auto counter = std::make_unique<Label>("Button clicks: 0");

    auto click_button = std::make_unique<ActionButton>("Click Me", [&] {
        static int count = 0;
        ++count;
        counter->set_text("Button clicks: " + std::to_string(count));
    });

    bool dark = false;
    auto theme_button = std::make_unique<ActionButton>("Toggle Dark", [&] {
        dark = !dark;
        Theme::set(dark ? Theme::make_dark() : Theme::make_light());
        window.invalidate_all();
    });

    auto section_overlay = std::make_unique<Label>("Overlay (modal)");
    section_overlay->set_bold(true);

    auto overlay = std::make_unique<DialogOverlay>();

    auto overlay_button = std::make_unique<ActionButton>("Show Overlay", [&] {
        if (overlay->is_open()) {
            overlay->close();
        } else {
            overlay->show(window);
        }
    });

    auto exit_button = std::make_unique<ActionButton>("Exit", [&] { window.close(); });
    exit_button->set_accent(false);

    auto section_input = std::make_unique<Label>("Input");
    section_input->set_bold(true);

    auto search_box = std::make_unique<TextBox>();
    search_box->set_placeholder("Search...");

    auto pwd_box = std::make_unique<TextBox>(String(), TextBoxConfig{TextBoxMode::Password});
    pwd_box->set_placeholder("Password");

    TextBoxConfig chat_cfg;
    chat_cfg.mode = TextBoxMode::MultiLine;
    chat_cfg.min_lines = 2;
    chat_cfg.max_lines = 6;
    auto chat_box = std::make_unique<TextBox>(String(), chat_cfg);
    chat_box->set_placeholder("Type a message...");

    auto hint = std::make_unique<Label>("Enter for new line; Shift+arrows select, Ctrl+A/C/V/X");
    hint->set_small(true);

    auto section_select = std::make_unique<Label>("Select");
    section_select->set_bold(true);

    auto combo = std::make_unique<ComboBox>();
    {
        std::vector<String> items;
        for (int i = 0; i < 30; ++i) {
            items.push_back("Item " + std::to_string(i));
        }
        combo->set_items(items);
    }
    combo->set_selected_index(0);
    combo->set_width(160.0f);

    auto section_toggle = std::make_unique<Label>("Toggle");
    section_toggle->set_bold(true);

    auto check_box = std::make_unique<CheckBox>("Enable feature");
    auto toggle = std::make_unique<ToggleSwitch>();

    auto section_choice = std::make_unique<Label>("Choice");
    section_choice->set_bold(true);

    auto radio_a = std::make_unique<RadioButton>("Option A");
    auto radio_b = std::make_unique<RadioButton>("Option B");
    auto radio_c = std::make_unique<RadioButton>("Option C");
    radio_a->set_checked(true);

    auto section_progress = std::make_unique<Label>("Progress");
    section_progress->set_bold(true);

    auto progress = std::make_unique<ProgressBar>();
    progress->set_value(0.4f);

    auto progress_value = std::make_unique<Label>("40%");

    auto slider = std::make_unique<ProgressSlider>(progress.get(), progress_value.get());
    slider->set_range(0.0f, 100.0f);
    slider->set_value(40.0f);

    class ProgressSpin : public SpinBox {
    public:
        ProgressSpin(ProgressBar* bar, Label* label) : bar_(bar), label_(label) {}

        void set_slider(Slider* slider) { slider_ = slider; }

        void on_value_changed(f64 value) override {
            bar_->set_value(static_cast<f32>(value / 100.0));
            if (slider_) slider_->set_value(static_cast<f32>(value));
            label_->set_text(std::to_string(static_cast<int>(value)) + "%");
        }

    private:
        ProgressBar* bar_;
        Slider* slider_ = nullptr;
        Label* label_;
    };

    auto spin = std::make_unique<ProgressSpin>(progress.get(), progress_value.get());
    spin->set_range(0.0, 100.0);
    spin->set_step(5.0);
    spin->set_value(40.0);
    spin->set_slider(slider.get());
    slider->set_spin(spin.get());

    auto section_list = std::make_unique<Label>("List (+ScrollView)");
    section_list->set_bold(true);

    auto list_status = std::make_unique<Label>("Click an item");
    list_status->set_small(true);

    auto list = std::make_unique<ItemList>(list_status.get());
    {
        std::vector<String> items;
        for (int i = 0; i < 30; ++i) {
            items.push_back("List item " + std::to_string(i));
        }
        list->set_items(items);
    }

    auto scroll_view = std::make_unique<ScrollView>();
    scroll_view->set_suggested_height(180.0f);
    scroll_view->set_content(list.get());

    root->append_child(title.get());
    root->append_child(counter.get());
    root->append_child(click_button.get());
    root->append_child(theme_button.get());

    root->append_child(section_overlay.get());
    root->append_child(overlay_button.get());

    root->append_child(section_input.get());
    root->append_child(search_box.get());
    root->append_child(pwd_box.get());
    root->append_child(chat_box.get());
    root->append_child(hint.get());

    root->append_child(section_select.get());
    root->append_child(combo.get());

    root->append_child(section_toggle.get());
    root->append_child(check_box.get());
    root->append_child(toggle.get());

    root->append_child(section_choice.get());
    root->append_child(radio_a.get());
    root->append_child(radio_b.get());
    root->append_child(radio_c.get());

    root->append_child(section_progress.get());
    root->append_child(progress.get());
    root->append_child(slider.get());
    root->append_child(spin.get());
    root->append_child(progress_value.get());

    root->append_child(section_list.get());
    root->append_child(list_status.get());
    root->append_child(scroll_view.get());

    root->append_child(exit_button.get());

    window.set_root(root.get());
    window.show();

    const int exit_code = app.run();

    window.set_root(nullptr);
    return exit_code;
}
