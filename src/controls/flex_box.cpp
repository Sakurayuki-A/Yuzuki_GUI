#include <yuzuki/controls/flex_box.hpp>

#include <algorithm>
#include <vector>

namespace yzk {

FlexBox::FlexBox(Orientation direction) : direction_(direction) {}

void FlexBox::set_direction(Orientation direction) {
    if (direction_ == direction) return;
    direction_ = direction;
    invalidate();
}

void FlexBox::set_spacing(f32 spacing) {
    if (spacing_ == spacing) return;
    spacing_ = spacing;
    invalidate();
}

void FlexBox::set_align_main(FlexAlign align) {
    if (main_align_ == align) return;
    main_align_ = align;
    invalidate();
}

void FlexBox::set_align_cross(FlexCrossAlign align) {
    if (cross_align_ == align) return;
    cross_align_ = align;
    invalidate();
}

namespace {

struct FlexItem {
    Widget* w = nullptr;
    Margins m;
    f32 main = 0.0f;   // Net main-axis size (excl. margin)
    f32 cross = 0.0f;  // Net cross-axis size
    f32 grow = 0.0f;
    f32 shrink = 0.0f;
};

}  // namespace

Size FlexBox::measure_content(Size available, const PaintContext* ctx) {
    f32 main = 0.0f;
    f32 cross = 0.0f;
    i32 count = 0;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        const Margins m = child->margin();
        const f32 cw = available.width > m.horizontal() ? available.width - m.horizontal() : 0.0f;
        const f32 ch = available.height > m.vertical() ? available.height - m.vertical() : 0.0f;
        const Size s = child->measure(Size{cw, ch}, ctx);
        if (s.width <= 0.0f && s.height <= 0.0f) continue;
        if (direction_ == Orientation::Horizontal) {
            main += s.width + m.horizontal();
            cross = cross > s.height + m.vertical() ? cross : s.height + m.vertical();
        } else {
            main += s.height + m.vertical();
            cross = cross > s.width + m.horizontal() ? cross : s.width + m.horizontal();
        }
        ++count;
    }

    if (count > 1) main += spacing_ * static_cast<f32>(count - 1);
    return direction_ == Orientation::Horizontal ? Size{main, cross} : Size{cross, main};
}

void FlexBox::arrange_content(const RectF& area, const PaintContext* ctx) {
    std::vector<FlexItem> items;
    f32 grow_sum = 0.0f;
    f32 shrink_sum = 0.0f;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        const Margins m = child->margin();
        const Size s = child->desired_size();
        if (s.width <= 0.0f && s.height <= 0.0f) continue;
        FlexItem it;
        it.w = child;
        it.m = m;
        it.grow = child->flex_grow();
        it.shrink = child->flex_shrink();
        if (direction_ == Orientation::Horizontal) {
            it.main = s.width;
            it.cross = s.height;
        } else {
            it.main = s.height;
            it.cross = s.width;
        }
        grow_sum += it.grow;
        shrink_sum += it.shrink;
        items.push_back(it);
    }
    if (items.empty()) return;

    const bool horiz = direction_ == Orientation::Horizontal;
    const f32 area_main = horiz ? area.width() : area.height();
    const f32 area_cross = horiz ? area.height() : area.width();

    f32 total = 0.0f;
    for (const FlexItem& it : items) {
        total += it.main + (horiz ? it.m.horizontal() : it.m.vertical());
    }
    total += spacing_ * static_cast<f32>(items.size() - 1);

    // Main-axis sizing: free space split by grow, overflow cut by shrink, floored at min size
    const f32 free = area_main - total;
    if (free > 0.0f && grow_sum > 0.0f) {
        for (FlexItem& it : items) it.main += free * it.grow / grow_sum;
    } else if (free < 0.0f && shrink_sum > 0.0f) {
        const f32 over = -free;
        for (FlexItem& it : items) {
            f32 cut = over * it.shrink / shrink_sum;
            const f32 min_main = horiz ? it.w->min_size().width : it.w->min_size().height;
            f32 floor = it.main < min_main ? it.main : min_main;
            if (cut > it.main - floor) cut = it.main - floor;
            if (cut < 0.0f) cut = 0.0f;
            it.main -= cut;
        }
    }

    // Main-axis alignment
    f32 new_total = 0.0f;
    for (const FlexItem& it : items) {
        new_total += it.main + (horiz ? it.m.horizontal() : it.m.vertical());
    }
    new_total += spacing_ * static_cast<f32>(items.size() - 1);
    const f32 lead = area_main - new_total;

    f32 spacing = spacing_;
    f32 start_lead = 0.0f;
    switch (main_align_) {
        case FlexAlign::Start:
            break;
        case FlexAlign::Center:
            start_lead = lead > 0.0f ? lead * 0.5f : 0.0f;
            break;
        case FlexAlign::End:
            start_lead = lead > 0.0f ? lead : 0.0f;
            break;
        case FlexAlign::SpaceBetween:
            if (lead > 0.0f && items.size() > 1) spacing += lead / static_cast<f32>(items.size() - 1);
            break;
        case FlexAlign::SpaceAround: {
            const f32 gap = lead > 0.0f ? lead / static_cast<f32>(items.size()) : 0.0f;
            spacing += gap;
            start_lead = gap * 0.5f;
            break;
        }
    }

    // Placement: cross-axis alignment + main-axis cursor advance
    const f32 main_start = horiz ? area.left : area.top;
    const f32 cross_start = horiz ? area.top : area.left;
    f32 cursor = main_start + start_lead;

    for (FlexItem& it : items) {
        const f32 main_pos = cursor + (horiz ? it.m.left : it.m.top);
        f32 cross_pos = 0.0f;
        f32 cross_size = 0.0f;
        if (cross_align_ == FlexCrossAlign::Stretch) {
            cross_size = area_cross - (horiz ? it.m.vertical() : it.m.horizontal());
            if (cross_size < 0.0f) cross_size = 0.0f;
            cross_pos = cross_start + (horiz ? it.m.top : it.m.left);
        } else {
            cross_size = it.cross;
            const f32 slack =
                area_cross - (horiz ? it.m.vertical() : it.m.horizontal()) - it.cross;
            switch (cross_align_) {
                case FlexCrossAlign::Start:
                    cross_pos = cross_start + (horiz ? it.m.top : it.m.left);
                    break;
                case FlexCrossAlign::Center:
                    cross_pos = cross_start + (slack > 0.0f ? slack * 0.5f : 0.0f) +
                                (horiz ? it.m.top : it.m.left);
                    break;
                case FlexCrossAlign::End:
                    cross_pos = cross_start + (slack > 0.0f ? slack : 0.0f) +
                                (horiz ? it.m.top : it.m.left);
                    break;
                default:
                    break;
            }
        }

        RectF r;
        if (horiz) {
            r = RectF::make(main_pos, cross_pos, it.main, cross_size);
        } else {
            r = RectF::make(cross_pos, main_pos, cross_size, it.main);
        }
        it.w->set_bounds(r);
        it.w->perform_layout(ctx);

        cursor = main_pos + it.main + (horiz ? it.m.right : it.m.bottom) + spacing;
    }
}

}  // namespace yzk