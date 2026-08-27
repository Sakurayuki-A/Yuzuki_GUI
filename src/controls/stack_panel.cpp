#include <yuzuki/controls/stack_panel.hpp>

#include <vector>

namespace yzk {

StackPanel::StackPanel(Orientation orientation) : orientation_(orientation) {}

StackPanel& StackPanel::set_orientation(Orientation orientation) {
    if (orientation_ == orientation) return *this;
    orientation_ = orientation;
    invalidate();
    return *this;
}

StackPanel& StackPanel::set_spacing(f32 spacing) {
    if (spacing_ == spacing) return *this;
    spacing_ = spacing;
    invalidate();
    return *this;
}

StackPanel& StackPanel::set_stretch_children(bool stretch) {
    if (stretch_ == stretch) return *this;
    stretch_ = stretch;
    invalidate();
    return *this;
}
Size StackPanel::measure_content(Size available, const PaintContext* ctx) {
    f32 width = 0.0f;
    f32 height = 0.0f;
    i32 count = 0;

    Size inner = available;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible() || !child->participates_in_layout()) continue;
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
    struct Item {
        Widget* w = nullptr;
        Margins m;
        f32 main = 0.0f;
    };
    std::vector<Item> items;
    f32 grow_sum = 0.0f;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        // Floating children (menus/popups) size and anchor themselves: recurse for
        // their own layout pass but keep them out of the flow math.
        if (!child->participates_in_layout()) {
            child->perform_layout(ctx);
            continue;
        }

        const Margins m = child->margin();
        const Size s = child->desired_size();
        if (s.width <= 0.0f && s.height <= 0.0f) continue;

        Item it;
        it.w = child;
        it.m = m;
        it.main = (orientation_ == Orientation::Vertical) ? s.height : s.width;
        grow_sum += child->flex_grow();
        items.push_back(it);
    }

    const bool horiz = orientation_ == Orientation::Horizontal;
    const f32 area_main = horiz ? area.width() : area.height();

    // Main-axis fill: distribute free space to flex-grow children (flex-grow
    // semantics). This lets a child stretch along the flow axis, e.g. a page that
    // should fill the remaining height instead of collapsing to content height.
    if (fill_ && grow_sum > 0.0f && !items.empty()) {
        f32 total = 0.0f;
        for (const Item& it : items) {
            total += it.main + (horiz ? it.m.horizontal() : it.m.vertical());
        }
        total += spacing_ * static_cast<f32>(items.size() - 1);
        const f32 free = area_main - total;
        if (free > 0.0f) {
            for (Item& it : items) {
                if (it.w->flex_grow() > 0.0f) it.main += free * it.w->flex_grow() / grow_sum;
            }
        }
    }

    f32 cursor = horiz ? area.left : area.top;
    for (Item& it : items) {
        const Size d = it.w->desired_size();
        RectF r;
        if (horiz) {
            r.left = cursor + it.m.left;
            r.right = r.left + it.main;
            r.top = area.top + it.m.top;
            r.bottom = stretch_ ? area.bottom - it.m.bottom : r.top + d.height;
        } else {
            r.left = area.left + it.m.left;
            r.right = stretch_ ? area.right - it.m.right : r.left + d.width;
            r.top = cursor + it.m.top;
            r.bottom = r.top + it.main;
        }

        it.w->set_bounds(r);
        it.w->perform_layout(ctx);
        cursor = (horiz ? r.right : r.bottom) + (horiz ? it.m.right : it.m.bottom) + spacing_;
    }
}

}  // namespace yzk
