#include <yuzuki/controls/scroll_view.hpp>

namespace yzk {

namespace {
constexpr f32 kScrollbarWidth = 6.0f;
constexpr f32 kScrollbarMargin = 2.0f;
constexpr f32 kThumbMinHeight = 24.0f;
constexpr f32 kWheelStep = 40.0f;
}  // namespace

void ScrollView::set_content(Widget* content) {
    if (content_ == content) return;
    if (content_ && content_) content_->remove_from_parent();
    content_ = content;
    if (content_) append_child(content_);
    invalidate();
}

void ScrollView::set_scroll_y(f32 y) {
    scroll_y_ = y;
    clamp_scroll();
    invalidate();
}

void ScrollView::scroll_by(f32 dy) {
    set_scroll_y(scroll_y_ + dy);
}

void ScrollView::set_suggested_height(f32 height) {
    if (suggested_height_ == height) return;
    suggested_height_ = height;
    invalidate();
}

Size ScrollView::measure_impl(Size available, const PaintContext* ctx) {
    (void)ctx;
    const f32 w = available.width > 0.0f ? available.width : 0.0f;
    return Size{w, suggested_height_};
}

void ScrollView::perform_layout(const PaintContext* ctx) {
    Widget::perform_layout(ctx);
    if (!content_) return;

    const f32 view_w = bounds_.width();
    const f32 view_h = bounds_.height();
    const Size content_size = content_->measure(Size{view_w, 1e7f}, ctx);
    content_height_ = content_size.height;

    const bool need_bar = content_height_ > view_h;
    max_scroll_ = need_bar ? (content_height_ - view_h) : 0.0f;
    clamp_scroll();

    const f32 bar_w = need_bar ? (kScrollbarWidth + kScrollbarMargin * 2.0f) : 0.0f;
    content_->set_bounds(RectF::make(0.0f, -scroll_y_, view_w - bar_w, content_height_));
    content_->perform_layout(ctx);
}

void ScrollView::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const RectF& b = bounds_;

    ctx.fill_rounded(b, theme.surface, theme.corner_radius);
    ctx.draw_border(b, theme.border, 1.0f, theme.corner_radius);

    if (content_ && content_->visible()) {
        const f32 bar_w = has_scrollbar() ? (kScrollbarWidth + kScrollbarMargin * 2.0f) : 0.0f;
        const RectF viewport = RectF::make(b.left, b.top, b.width() - bar_w, b.height());
        if (!viewport.empty()) {
            ctx.push_clip(viewport);
            const f32 ox = ctx.offset_x();
            const f32 oy = ctx.offset_y();
            ctx.set_offset(ox + b.left, oy + b.top);
            content_->paint(ctx);
            ctx.set_offset(ox, oy);
            ctx.pop_clip();
        }
    }

    if (has_scrollbar()) {
        const f32 track_x = b.right - kScrollbarMargin - kScrollbarWidth;
        const f32 track_top = b.top + kScrollbarMargin;
        const f32 track_h = b.height() - kScrollbarMargin * 2.0f;
        const f32 ratio = track_h / content_height_;
        const f32 thumb_h = ratio * track_h;
        const f32 thumb_h_clamped = thumb_h < kThumbMinHeight ? kThumbMinHeight : thumb_h;
        const f32 thumb_y = track_top + (track_h - thumb_h_clamped) * (max_scroll_ > 0.0f ? scroll_y_ / max_scroll_ : 0.0f);

        ctx.fill_rounded(RectF::make(track_x, track_top, kScrollbarWidth, track_h),
                         Color{0xE0, 0xE0, 0xE0}, kScrollbarWidth / 2.0f);
        ctx.fill_rounded(RectF::make(track_x, thumb_y, kScrollbarWidth, thumb_h_clamped),
                         theme.border_hover, kScrollbarWidth / 2.0f);
    }
}

void ScrollView::on_event(Event& e) {
    switch (e.type) {
        case EventType::Wheel:
            if (content_ && has_scrollbar()) {
                scroll_by(-static_cast<f32>(e.data.mouse.wheel_delta) / 120.0f * kWheelStep);
                e.consumed = true;
            }
            break;

        case EventType::MouseDown:
            if ((e.data.mouse.buttons & MouseButton_Left) && has_scrollbar()) {
                const RectF g = global_bounds();
                const f32 x = e.data.mouse.x - g.left;
                const f32 y = e.data.mouse.y - g.top;
                if (scrollbar_hit(x)) {
                    const f32 track_top = kScrollbarMargin;
                    const f32 track_h = bounds_.height() - kScrollbarMargin * 2.0f;
                    const f32 ratio = track_h / content_height_;
                    const f32 thumb_h = ratio * track_h;
                    const f32 thumb_h_clamped = thumb_h < kThumbMinHeight ? kThumbMinHeight : thumb_h;
                    const f32 thumb_y = track_top + (track_h - thumb_h_clamped) * (max_scroll_ > 0.0f ? scroll_y_ / max_scroll_ : 0.0f);
                    if (y >= thumb_y && y <= thumb_y + thumb_h_clamped) {
                        dragging_thumb_ = true;
                        drag_grab_ = y - thumb_y;
                    } else {
                        const f32 dy = track_h > thumb_h_clamped ? (y - thumb_y - thumb_h_clamped) : 0.0f;
                        scroll_by(dy / (track_h - thumb_h_clamped) * max_scroll_);
                    }
                    e.consumed = true;
                }
            }
            break;

        case EventType::MouseMove:
            if (dragging_thumb_ && has_scrollbar()) {
                const RectF g = global_bounds();
                const f32 y = e.data.mouse.y - g.top;
                const f32 track_top = kScrollbarMargin;
                const f32 track_h = bounds_.height() - kScrollbarMargin * 2.0f;
                const f32 ratio = track_h / content_height_;
                const f32 thumb_h = ratio * track_h;
                const f32 thumb_h_clamped = thumb_h < kThumbMinHeight ? kThumbMinHeight : thumb_h;
                const f32 range = track_h - thumb_h_clamped;
                if (range > 0.0f) {
                    set_scroll_y((y - drag_grab_ - track_top) / range * max_scroll_);
                }
                e.consumed = true;
            }
            break;

        case EventType::MouseUp:
            if (dragging_thumb_) {
                dragging_thumb_ = false;
                e.consumed = true;
            }
            break;

        default:
            break;
    }
}

Widget* ScrollView::hit_test(f32 x, f32 y) {
    if (!visible()) return nullptr;
    if (x < 0.0f || y < 0.0f || x > bounds_.width() || y > bounds_.height()) return nullptr;
    if (scrollbar_hit(x)) return this;
    return Widget::hit_test(x, y);
}

void ScrollView::clamp_scroll() {
    if (scroll_y_ < 0.0f) scroll_y_ = 0.0f;
    if (scroll_y_ > max_scroll_) scroll_y_ = max_scroll_;
}

bool ScrollView::scrollbar_hit(f32 x) const {
    if (!has_scrollbar()) return false;
    const f32 bar_w = kScrollbarWidth + kScrollbarMargin * 2.0f;
    return x >= bounds_.width() - bar_w;
}

}  // namespace yzk
