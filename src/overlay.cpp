#include <yuzuki/ui/overlay.hpp>
#include <yuzuki/ui/window.hpp>

namespace yzk {

namespace {

constexpr u32 kKeyEscape = 0x1B;

constexpr f32 kOpenDurationMs = 280.0f;
constexpr f32 kCloseDurationMs = 220.0f;
constexpr f32 kSlidePx = 28.0f;

}  // namespace

Overlay::Overlay() {
    set_focusable(true);
}

Overlay::~Overlay() = default;

void Overlay::show(Window& win) {
    if (open_) return;
    Widget* root = win.root();
    if (!root) return;

    AnimationSystem::instance().finish_tween(active_tween_);
    closing_ = false;

    set_bounds(win.bounds());
    set_visible(true);
    root->append_child(this);
    open_ = true;
    win.invalidate_all();
    request_focus();

    if (animated_) {
        progress_ = 0.0f;
        active_tween_ = AnimationSystem::instance().tween(
            0.0f, 1.0f, kOpenDurationMs, Easing::OutCubic,
            [this](f32 v) {
                progress_ = v;
                invalidate();
            });
    } else {
        progress_ = 1.0f;
        active_tween_ = 0;
    }
}

void Overlay::close() {
    if (!open_) return;
    if (!animated_ || progress_ <= 0.02f) {
        finish_close();
        return;
    }
    if (closing_) return;
    closing_ = true;
    AnimationSystem::instance().finish_tween(active_tween_);
    active_tween_ = AnimationSystem::instance().tween(
        progress_, 0.0f, kCloseDurationMs, Easing::InCubic,
        [this](f32 v) {
            progress_ = v;
            invalidate();
            if (v <= 0.001f) finish_close();
        });
}

void Overlay::finish_close() {
    if (!open_) return;
    open_ = false;
    closing_ = false;
    set_visible(false);
    if (window()) window()->set_focus(nullptr);
    remove_from_parent();
    if (window()) window()->invalidate_all();
    on_close();
}

void Overlay::paint_impl(PaintContext& ctx) {
    const f32 ox = ctx.offset_x();
    const f32 oy = ctx.offset_y();
    ctx.set_offset(ox + bounds_.left, oy + bounds_.top);

    const RectF full = RectF::make(0.0f, 0.0f, bounds_.width(), bounds_.height());
    const f32 p = animated_ ? progress_ : 1.0f;
    const Color dim = dim_.with_alpha(static_cast<u8>(dim_.a * p));
    if (blurred_) {
        ctx.draw_backdrop_blur(full, 8.0f, dim, 0.0f);
    } else {
        ctx.fill_rect(full, dim);
    }

    // Panel slide: slide_dir_=+1 slides up from below (centered panel), -1 drops down
    // from above (drop-down panels start kSlidePx above their final position).
    const f32 slide = (1.0f - ease(p, Easing::OutCubic)) * kSlidePx * slide_dir_;
    ctx.set_offset(ox + bounds_.left, oy + bounds_.top + slide);

    const Theme& theme = ctx.theme();
    if (shadow_) {
        ctx.draw_shadow(panel_rect_, theme.corner_radius, 18.0f, Color{0x00, 0x00, 0x00, 40});
    }
    ctx.fill_rounded(panel_rect_, theme.surface, theme.corner_radius);
    ctx.draw_border(panel_rect_, theme.border, 1.0f, theme.corner_radius);

    Widget::paint_impl(ctx);

    ctx.set_offset(ox, oy);
}

void Overlay::perform_layout(const PaintContext* ctx) {
    if (window()) set_bounds(window()->bounds());
    Widget::perform_layout(ctx);
}

void Overlay::on_event(Event& e) {
    if (e.type == EventType::KeyDown && e.data.key.code == kKeyEscape) {
        e.consumed = true;
        close();
        return;
    }
    if (e.type == EventType::MouseDown && (e.data.mouse.buttons & MouseButton_Left)) {
        const RectF g = global_bounds();
        const f32 lx = e.data.mouse.x - g.left;
        const f32 ly = e.data.mouse.y - g.top;
        if (!panel_rect_.contains(lx, ly)) {
            e.consumed = true;
            close();
            return;
        }
    }
    Widget::on_event(e);
}

}  // namespace yzk