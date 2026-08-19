#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/animation.hpp>

#include <vector>

namespace yzk {

class Window;

enum class NotificationType : u8 { Info, Success, Warning, Error };

// A single notification: type-colored icon + title + body + close button.
// Slides in from the right with fade, auto-dismisses after duration; width from measure
class Notification : public Widget {
public:
    Notification(String title, String message, NotificationType type);
    ~Notification() override;

    void set_duration_ms(f32 ms) { duration_ms_ = ms; }
    NotificationType type() const { return type_; }
    bool dismissed() const { return dismissed_; }
    f32 last_height() const { return last_height_; }

    void dismiss();  // Starts exit animation, removes from window on completion

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;

private:
    friend class NotificationManager;
    void wrap(const PaintContext* ctx, f32 max_width);
    void begin_enter();
    void finish();

    String title_;
    String message_;
    NotificationType type_;
    f32 duration_ms_ = 3200.0f;
    std::vector<String> lines_;
    f32 line_h_ = 16.0f;
    f32 last_height_ = 64.0f;
    f32 progress_ = 0.0f;  // 0 = hidden, 1 = fully visible
    bool dismissing_ = false;
    bool dismissed_ = false;
    bool hovered_ = false;
    AnimationSystem::Token active_tween_ = 0;
};

// Notification manager: stacks up from bottom-right, auto-queues
class NotificationManager {
public:
    static NotificationManager& instance();

    Notification& show(Window& win, const String& title, const String& message = String(),
                       NotificationType type = NotificationType::Info, f32 duration_ms = 3200.0f);
    void dismiss_all(Window& win);
    void layout_all(Window& win);
    void remove(Notification* notification);

private:
    NotificationManager() = default;
    std::vector<Notification*> notifications_;
};

}  // namespace yzk