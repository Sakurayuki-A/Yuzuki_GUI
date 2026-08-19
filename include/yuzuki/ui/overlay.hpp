#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/animation.hpp>

namespace yzk {

class Window;

// Full-screen modal overlay: dim mask + centered panel (shadow/rounded corners).
// Animated by default: mask fades in, panel slides up; closing reverses the
// progress (driven by AnimationSystem). For drop-down panels call
// set_slide_direction(-1): it expands downward from the anchor edge.
class Overlay : public Widget {
public:
    Overlay();
    ~Overlay() override;

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    void show(Window& win);
    void close();
    bool is_open() const { return open_; }

    void set_animated(bool animated) { animated_ = animated; }
    bool animated() const { return animated_; }

    void set_dim(const Color& color) { dim_ = color; }
    void set_dim_blurred(bool blurred) { blurred_ = blurred; }
    void set_shadow(bool shadow) { shadow_ = shadow; }
    void set_panel_rect(const RectF& rect) { panel_rect_ = rect; }
    const RectF& panel_rect() const { return panel_rect_; }

    // Panel slide direction: +1 = slide up from below (default, centered panel),
    // -1 = slide down from above (drop-down panel)
    void set_slide_direction(f32 dir) { slide_dir_ = dir < 0.0f ? -1.0f : 1.0f; }
    f32 slide_direction() const { return slide_dir_; }

    virtual void on_close() {}

protected:
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;

private:
    void finish_close();

    RectF panel_rect_;
    Color dim_ = Color{0x00, 0x00, 0x00, 90};
    bool blurred_ = false;
    bool shadow_ = true;
    bool open_ = false;
    bool animated_ = true;
    bool closing_ = false;
    f32 progress_ = 1.0f;
    f32 slide_dir_ = 1.0f;
    AnimationSystem::Token active_tween_ = 0;
};

}  // namespace yzk