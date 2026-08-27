#include <yuzuki/controls/backdrop_blur.hpp>

namespace yzk {

BackdropBlur& BackdropBlur::set_blur(f32 blur) {
    if (blur_ == blur) return *this;
    blur_ = blur;
    invalidate();
    return *this;
}

BackdropBlur& BackdropBlur::set_tint(const Color& tint) {
    if (tint_ == tint) return *this;
    tint_ = tint;
    invalidate();
    return *this;
}

BackdropBlur& BackdropBlur::set_corner_radius(f32 radius) {
    if (corner_radius_ == radius) return *this;
    corner_radius_ = radius;
    invalidate();
    return *this;
}

Size BackdropBlur::measure_impl(Size available, const PaintContext* ctx) {
    (void)ctx;
    return Size{available.width > 0.0f ? available.width : 0.0f, 0.0f};
}

void BackdropBlur::paint_impl(PaintContext& ctx) {
    const RectF& b = bounds_;
    if (b.empty()) return;

    const f32 ox = ctx.offset_x();
    const f32 oy = ctx.offset_y();
    ctx.set_offset(ox + b.left, oy + b.top);

    ctx.draw_backdrop_blur(b, blur_, tint_, corner_radius_);

    for (Widget* child = first_child(); child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        child->paint(ctx);
    }

    ctx.set_offset(ox, oy);
}

}  // namespace yzk