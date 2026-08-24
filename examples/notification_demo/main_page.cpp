#include "main_page.hpp"

#include <yuzuki/controls/button.hpp>
#include <yuzuki/controls/label.hpp>
#include <yuzuki/controls/notification.hpp>
#include <yuzuki/controls/stack_panel.hpp>

namespace {

using namespace yzk;

// Clicking this button shows an animated notification sliding in from the bottom-right.
class ShowOverlayButton : public Button {
public:
    explicit ShowOverlayButton(Window& win) : Button("Show Overlay"), win_(win) {
        set_min_width(180.0f);
        set_padding(14.0f);
    }
    void on_click() override {
        NotificationManager::instance().show(
            win_, "Notification title",
            "This is an animated notification.\nIt slides in from the bottom-right.",
            NotificationType::Success);
    }

private:
    Window& win_;
};

}  // namespace

namespace yzk {

Widget* make_notification_page(Window& win) {
    auto* root = new StackPanel(Orientation::Vertical);
    root->set_spacing(14.0f);
    root->set_padding(24.0f);

    auto* title = new Label("Notification Demo");
    title->set_bold(true);

    auto* hint = new Label("Click the button, then keep the mouse still.");
    hint->set_text_role(TextRole::Secondary);
    hint->set_small(true);

    auto* button = new ShowOverlayButton(win);

    root->append_child(title);
    root->append_child(hint);
    root->append_child(button);
    return root;
}

}  // namespace yzk