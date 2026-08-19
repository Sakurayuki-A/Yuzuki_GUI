#include <yuzuki/controls/wrap_panel.hpp>

namespace yzk {

void WrapPanel::set_spacing(f32 spacing) {
    if (spacing_ == spacing) return;
    spacing_ = spacing;
    invalidate();
}

void WrapPanel::set_line_spacing(f32 line_spacing) {
    if (line_spacing_ == line_spacing) return;
    line_spacing_ = line_spacing;
    invalidate();
}

Size WrapPanel::measure_content(Size available, const PaintContext* ctx) {
    child_x_.clear();
    child_y_.clear();
    child_size_.clear();

    f32 line_w = 0.0f;
    f32 line_h = 0.0f;
    f32 row_y = 0.0f;
    f32 total_w = 0.0f;
    const bool bounded = available.width > 0.0f;

    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        const Margins m = child->margin();
        const f32 cw = available.width > m.horizontal() ? available.width - m.horizontal() : 0.0f;
        const f32 ch = available.height > m.vertical() ? available.height - m.vertical() : 0.0f;
        const Size s = child->measure(Size{cw, ch}, ctx);
        if (s.width <= 0.0f && s.height <= 0.0f) continue;
        const f32 slot_w = s.width + m.horizontal();
        const f32 slot_h = s.height + m.vertical();
        const f32 add = (line_w > 0.0f ? spacing_ : 0.0f) + slot_w;
        if (bounded && line_w > 0.0f && line_w + add > available.width) {
            row_y += line_h + line_spacing_;
            total_w = total_w > line_w ? total_w : line_w;
            line_w = 0.0f;
            line_h = 0.0f;
            child_x_.push_back(line_w);
            child_y_.push_back(row_y);
            child_size_.push_back(s);
            line_w = slot_w;
            line_h = slot_h;
            continue;
        }
        child_x_.push_back(line_w + (line_w > 0.0f ? spacing_ : 0.0f));
        child_y_.push_back(row_y);
        child_size_.push_back(s);
        line_w += add;
        line_h = line_h > slot_h ? line_h : slot_h;
    }
    total_w = total_w > line_w ? total_w : line_w;
    return Size{total_w, row_y + line_h};
}

void WrapPanel::perform_layout(const PaintContext* ctx) {
    child_x_.clear();
    child_y_.clear();
    child_size_.clear();
    Layout::perform_layout(ctx);
}

void WrapPanel::arrange_content(const RectF& area, const PaintContext* ctx) {
    if (child_size_.empty()) measure_content(Size{area.width(), area.height()}, ctx);
    size_t i = 0;
    for (Widget* child = first_child_; child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        if (i >= child_size_.size()) break;
        const Margins m = child->margin();
        const Size s = child_size_[i];
        child->set_bounds(RectF::make(area.left + child_x_[i] + m.left,
                                      area.top + child_y_[i] + m.top, s.width, s.height));
        child->perform_layout(ctx);
        ++i;
    }
}

}  // namespace yzk