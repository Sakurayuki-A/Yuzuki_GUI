#include <yuzuki/controls/dock_panel.hpp>

namespace yzk {

namespace {

RectF shrink(RectF r, f32 w, f32 h) {
    if (w < 0.0f) w = 0.0f;
    if (h < 0.0f) h = 0.0f;
    return RectF::make(r.left, r.top, w, h);
}

}  // namespace

void DockPanel::dock(Widget* child, Dock dock) {
    if (!child) return;
    append_child(child);
    docks_.push_back(dock);
}

void DockPanel::set_gap(f32 gap) {
    if (gap_ == gap) return;
    gap_ = gap;
    invalidate();
}

Size DockPanel::measure_content(Size available, const PaintContext* ctx) {
    f32 w = 0.0f;
    f32 h = 0.0f;
    f32 rem_w = available.width;
    f32 rem_h = available.height;

    Widget* child = first_child_;
    size_t i = 0;
    while (child) {
        const Dock dock = i < docks_.size() ? docks_[i] : Dock::Fill;
        if (child->visible()) {
            const Margins m = child->margin();
            const f32 cw = rem_w > m.horizontal() ? rem_w - m.horizontal() : 0.0f;
            const f32 ch = rem_h > m.vertical() ? rem_h - m.vertical() : 0.0f;
            const Size s = child->measure(Size{cw, ch}, ctx);
            switch (dock) {
                case Dock::Left:
                case Dock::Right:
                    w += s.width + m.horizontal() + gap_;
                    rem_w -= s.width + m.horizontal() + gap_;
                    break;
                case Dock::Top:
                case Dock::Bottom:
                    h += s.height + m.vertical() + gap_;
                    rem_h -= s.height + m.vertical() + gap_;
                    break;
                case Dock::Fill:
                default:
                    h = h > s.height + m.vertical() ? h : s.height + m.vertical();
                    break;
            }
        }
        child = child->next_sibling();
        ++i;
    }
    return Size{w, h};
}

void DockPanel::arrange_content(const RectF& area, const PaintContext* ctx) {
    RectF remaining = area;

    Widget* child = first_child_;
    size_t i = 0;
    while (child) {
        const Dock dock = i < docks_.size() ? docks_[i] : Dock::Fill;
        if (child->visible()) {
            const Margins m = child->margin();
            const Size s = child->desired_size();
            const f32 slot_w = s.width + m.horizontal();
            const f32 slot_h = s.height + m.vertical();
            RectF r;
            switch (dock) {
                case Dock::Left:
                    r = shrink(RectF::make(remaining.left + m.left, remaining.top + m.top,
                                           s.width, remaining.height() - m.vertical()),
                               s.width, remaining.height() - m.vertical());
                    remaining.left += slot_w + gap_;
                    break;
                case Dock::Right:
                    r = shrink(RectF::make(remaining.right - slot_w + m.left,
                                           remaining.top + m.top, s.width,
                                           remaining.height() - m.vertical()),
                               s.width, remaining.height() - m.vertical());
                    remaining.right -= slot_w + gap_;
                    break;
                case Dock::Top:
                    r = shrink(RectF::make(remaining.left + m.left, remaining.top + m.top,
                                           remaining.width() - m.horizontal(), s.height),
                               remaining.width() - m.horizontal(), s.height);
                    remaining.top += slot_h + gap_;
                    break;
                case Dock::Bottom:
                    r = shrink(RectF::make(remaining.left + m.left,
                                           remaining.bottom - slot_h + m.top,
                                           remaining.width() - m.horizontal(), s.height),
                               remaining.width() - m.horizontal(), s.height);
                    remaining.bottom -= slot_h + gap_;
                    break;
                case Dock::Fill:
                default:
                    r = shrink(RectF::make(remaining.left + m.left, remaining.top + m.top,
                                           remaining.width() - m.horizontal(),
                                           remaining.height() - m.vertical()),
                               remaining.width() - m.horizontal(),
                               remaining.height() - m.vertical());
                    break;
            }
            child->set_bounds(r);
            child->perform_layout(ctx);
        }
        child = child->next_sibling();
        ++i;
    }
}

}  // namespace yzk