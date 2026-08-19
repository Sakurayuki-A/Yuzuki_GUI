#include <yuzuki/controls/box.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

void Box::set_bg(const Color& color) {
    if (bg_ == color) return;
    bg_ = color;
    invalidate();
}

void Box::set_radius(f32 radius) {
    if (radius_ == radius) return;
    radius_ = radius;
    invalidate();
}

void Box::set_border_width(f32 width) {
    if (border_width_ == width) return;
    border_width_ = width;
    invalidate();
}

void Box::set_border_color(const Color& color) {
    if (border_color_ == color) return;
    border_color_ = color;
    invalidate();
}

void Box::set_border(f32 width, const Color& color) {
    set_border_width(width);
    set_border_color(color);
}

void Box::set_padding(f32 padding) {
    if (padding_ == padding) return;
    padding_ = padding;
    invalidate();
}

void Box::set_shadow(f32 blur, f32 offset_y) {
    if (shadow_blur_ == blur && shadow_offset_y_ == offset_y) return;
    shadow_blur_ = blur;
    shadow_offset_y_ = offset_y;
    invalidate();
}

void Box::set_shadow_color(const Color& color) {
    if (shadow_color_ == color) return;
    shadow_color_ = color;
    invalidate();
}

Size Box::measure_impl(Size available, const PaintContext* ctx) {
    f32 w = padding_ * 2.0f, h = padding_ * 2.0f;
    if (Widget* child = first_child()) {
        if (child->visible()) {
            const f32 p2 = padding_ * 2.0f;
            const f32 inner_w = available.width > p2 ? available.width - p2 : 0.0f;
            const f32 inner_h = available.height > p2 ? available.height - p2 : 0.0f;
            const Size s = child->measure(Size{inner_w, inner_h}, ctx);
            w += s.width;
            h += s.height;
        }
    }
    return Size{w, h};
}

void Box::perform_layout(const PaintContext* ctx) {
    Widget* child = first_child();
    if (!child || !child->visible()) return;
    // Child fills the content area; its desired size only affects the container in measure_impl
    child->set_bounds(content_area());
    child->perform_layout(ctx);
}

RectF Box::content_area() const {
    const f32 p = padding_;
    return RectF::make(bounds_.left + p, bounds_.top + p,
                       bounds_.width() - p * 2.0f, bounds_.height() - p * 2.0f);
}

void Box::paint_impl(PaintContext& ctx) {
    const RectF b = bounds_;
    if (shadow_blur_ > 0.0f) {
        ctx.draw_shadow(b.translated(0.0f, shadow_offset_y_), radius_, shadow_blur_,
                        shadow_color_);
    }
    if (!bg_.is_transparent()) {
        if (radius_ > 0.0f) {
            ctx.fill_rounded(b, bg_, radius_);
        } else {
            ctx.fill_rect(b, bg_);
        }
    }
    if (border_width_ > 0.0f && !border_color_.is_transparent()) {
        ctx.draw_border(b, border_color_, border_width_, radius_);
    }
    Widget::paint_impl(ctx);
}

}  // namespace yzk