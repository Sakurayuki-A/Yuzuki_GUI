#include <yuzuki/controls/stack_panel.hpp>

namespace yzk {

StackPanel::StackPanel(Orientation orientation) : orientation_(orientation) {}

void StackPanel::set_orientation(Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    invalidate();
}

void StackPanel::set_spacing(f32 spacing) {
    if (spacing_ == spacing) return;
    spacing_ = spacing;
    invalidate();
}

void StackPanel::set_stretch_children(bool stretch) {
    if (stretch_ == stretch) return;
    stretch_ = stretch;
    invalidate();
}
Size StackPanel::measure_content(Size available, const PaintContext* ctx) {
    f32 width = 0.0f;
    f32 height = 0.0f;
    i32 count = 0;

    Size inner = available;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        const Margins m = child->margin();
        const f32 cw = inner.width > m.horizontal() ? inner.width - m.horizontal() : 0.0f;
        const f32 ch = inner.height > m.vertical() ? inner.height - m.vertical() : 0.0f;
        const Size s = child->measure(Size{cw, ch}, ctx);
        if (s.width <= 0.0f && s.height <= 0.0f) continue;
        if (orientation_ == Orientation::Vertical) {
            width = width > s.width + m.horizontal() ? width : s.width + m.horizontal();
            height += s.height + m.vertical();
        } else {
            height = height > s.height + m.vertical() ? height : s.height + m.vertical();
            width += s.width + m.horizontal();
        }
        ++count;
    }

    if (count == 0) return Size{0.0f, 0.0f};

    if (count > 1) {
        const f32 total_spacing = spacing_ * static_cast<f32>(count - 1);
        if (orientation_ == Orientation::Vertical) height += total_spacing;
        else width += total_spacing;
    }

    return Size{width, height};
}

void StackPanel::arrange_content(const RectF& area, const PaintContext* ctx) {
    f32 cursor = (orientation_ == Orientation::Vertical) ? area.top : area.left;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;

        const Margins m = child->margin();
        Size s = child->desired_size();
        if (s.width <= 0.0f && s.height <= 0.0f) continue;
        RectF r = child->bounds();

        if (orientation_ == Orientation::Vertical) {
            r.left = area.left + m.left;
            r.right = stretch_ ? area.right - m.right : r.left + s.width;
            r.top = cursor + m.top;
            r.bottom = r.top + s.height;
            cursor = r.bottom + m.bottom + spacing_;
        } else {
            r.left = cursor + m.left;
            r.right = r.left + s.width;
            r.top = area.top + m.top;
            r.bottom = stretch_ ? area.bottom - m.bottom : r.top + s.height;
            cursor = r.right + m.right + spacing_;
        }

        child->set_bounds(r);
        child->perform_layout(ctx);
    }
}

}  // namespace yzk