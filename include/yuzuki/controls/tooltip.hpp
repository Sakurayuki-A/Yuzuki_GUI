#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/animation.hpp>

#include <map>
#include <vector>

namespace yzk {

class Window;

// Tooltip bubble: dark rounded card with small text, fades in/out, follows the mouse, does not intercept clicks
class Tooltip : public Widget {
public:
    Tooltip() = default;

    void set_text(const String& text);
    void set_anchor(f32 x, f32 y);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    Widget* hit_test(f32 x, f32 y) override { return nullptr; }

    friend class TooltipManager;

private:
    String text_;
    f32 anchor_x_ = 0.0f;
    f32 anchor_y_ = 0.0f;
    f32 size_w_ = 80.0f;
    f32 size_h_ = 28.0f;
    f32 progress_ = 0.0f;
};

// Tooltip manager: register widgets, delayed show on hover, follow the mouse, auto-hide on leave
class TooltipManager {
public:
    static TooltipManager& instance();

    void set_tooltip(Widget* owner, const String& text);
    void remove_tooltip(Widget* owner);
    void on_hover_changed(Window& win, Widget* hovered, f32 mx, f32 my);
    void on_mouse_move(Window& win, f32 mx, f32 my);
    void on_timer(Tooltip& tip);

private:
    TooltipManager() = default;
    Tooltip* get_tip(Window& win);
    void hide(Window& win);

    struct Entry {
        Widget* owner = nullptr;
        String text;
    };

    std::vector<Entry> entries_;
    std::map<Window*, Tooltip*> tips_;
    Widget* owner_ = nullptr;
    f32 mx_ = 0.0f;
    f32 my_ = 0.0f;
};

}  // namespace yzk