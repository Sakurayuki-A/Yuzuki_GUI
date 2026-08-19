#include <yuzuki/controls/list_view.hpp>

#include <algorithm>
#include <cmath>

namespace yzk {

namespace {
constexpr f32 kTextPadding = 10.0f;
constexpr f32 kScrollbarWidth = 6.0f;
constexpr f32 kScrollbarMargin = 4.0f;
constexpr f32 kThumbMinHeight = 24.0f;
constexpr f32 kWheelStep = 48.0f;
}

void ListView::set_items(const std::vector<String>& items) {
    source_ = nullptr;
    items_ = items;
    selected_ = -1;
    hovered_ = -1;
    scroll_y_ = 0.0f;
    max_scroll_ = 0.0f;
    invalidate();
}

void ListView::set_data_source(DataSource* source) {
    if (source_ == source) return;
    source_ = source;
    selected_ = -1;
    hovered_ = -1;
    scroll_y_ = 0.0f;
    max_scroll_ = 0.0f;
    invalidate();
}

void ListView::set_row_delegate(RowDelegate* delegate) {
    if (delegate_ == delegate) return;
    delegate_ = delegate;
    invalidate();
}

void ListView::add_item(const String& item) {
    if (source_) return;  // Data source mode: content managed by the source
    items_.push_back(item);
    update_max_scroll();
    invalidate();
}

void ListView::clear_items() {
    source_ = nullptr;
    items_.clear();
    selected_ = -1;
    hovered_ = -1;
    scroll_y_ = 0.0f;
    max_scroll_ = 0.0f;
    invalidate();
}

void ListView::set_selected(i32 index) {
    if (index < -1 || index >= count()) return;
    if (selected_ == index) return;
    selected_ = index;
    on_selected(selected_);
    invalidate();
}

void ListView::set_hovered(i32 index) {
    if (index < -1 || index >= count()) return;
    if (hovered_ == index) return;
    hovered_ = index;
    invalidate();
}

void ListView::set_row_height(f32 height) {
    if (row_height_ == height) return;
    row_height_ = height;
    update_max_scroll();
    invalidate();
}

void ListView::set_scroll_y(f32 y) {
    update_max_scroll();
    if (scroll_y_ == y) return;
    scroll_y_ = y;
    clamp_scroll();
    invalidate();
}

void ListView::scroll_by(f32 dy) {
    set_scroll_y(scroll_y_ + dy);
}

void ListView::update_max_scroll() {
    const f32 content = content_height();
    const f32 view = bounds_.height();
    max_scroll_ = content > view ? content - view : 0.0f;
}

void ListView::clamp_scroll() {
    if (scroll_y_ < 0.0f) scroll_y_ = 0.0f;
    if (scroll_y_ > max_scroll_) scroll_y_ = max_scroll_;
}

Size ListView::measure_impl(Size available, const PaintContext* ctx) {
    (void)ctx;
    const f32 w = available.width > 0.0f ? available.width : 0.0f;
    const f32 content = content_height();
    const f32 h = available.height > 0.0f ? std::min(available.height, content) : content;
    return Size{w, h};
}

void ListView::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const RectF& b = bounds_;

    update_max_scroll();
    clamp_scroll();

    const f32 view_h = b.height();
    const i32 n = count();
    if (n == 0) return;

    const f32 bar_w = show_scrollbar_ && max_scroll_ > 0.0f ? kScrollbarWidth + kScrollbarMargin : 0.0f;
    const f32 row_w = b.width() - bar_w;

    ctx.push_clip(b);

    const i32 first = std::max(0, static_cast<i32>(std::floor(scroll_y_ / row_height_)));
    const i32 last =
        std::min(n - 1, static_cast<i32>(std::ceil((scroll_y_ + view_h) / row_height_)));

    for (i32 i = first; i <= last; ++i) {
        const f32 row_top = b.top + static_cast<f32>(i) * row_height_ - scroll_y_;
        const RectF row = RectF::make(b.left, row_top, row_w, row_height_);

        Color bg{0, 0, 0, 0};
        Color fg = theme.text;
        const bool is_selected = (i == selected_);
        const bool is_hovered = (i == hovered_);
        if (is_selected) {
            bg = theme.accent;
            fg = theme.accent_text;
        } else if (is_hovered) {
            bg = theme.accent.with_alpha(40);
            fg = theme.text;
        }
        if (!bg.is_transparent()) {
            const RectF hl = RectF::make(row.left + 4.0f, row.top + 2.0f, row.width() - 8.0f,
                                         row_height_ - 4.0f);
            ctx.fill_rounded(hl, bg, 3.0f);
        }

        if (delegate_) {
            delegate_->draw(*this, ctx, i, row);
        } else {
            const RectF text_rect =
                RectF::make(row.left + kTextPadding, row.top, row.width() - kTextPadding * 2.0f,
                            row_height_);
            ctx.draw_text(text_at(i), text_rect, fg, TextAlignH::Left, TextAlignV::Center);
        }
    }

    ctx.pop_clip();

    if (show_border_) {
        ctx.draw_border(b, theme.border, 1.0f, 2.0f);
    }

    if (show_scrollbar_ && max_scroll_ > 0.0f) {
        const f32 track_x = b.right - kScrollbarMargin - kScrollbarWidth;
        const f32 track_top = b.top + kScrollbarMargin;
        const f32 track_h = b.height() - kScrollbarMargin * 2.0f;
        const f32 ratio = track_h / content_height();
        const f32 thumb_h = std::max(ratio * track_h, kThumbMinHeight);
        const f32 thumb_y = track_top + (track_h - thumb_h) * (max_scroll_ > 0.0f ? scroll_y_ / max_scroll_ : 0.0f);

        ctx.fill_rounded(RectF::make(track_x, track_top, kScrollbarWidth, track_h),
                         theme.surface_container_high, kScrollbarWidth / 2.0f);
        ctx.fill_rounded(RectF::make(track_x, thumb_y, kScrollbarWidth, thumb_h),
                         theme.border_hover, kScrollbarWidth / 2.0f);
    }
}

void ListView::on_event(Event& e) {
    switch (e.type) {
        case EventType::Wheel:
            if (max_scroll_ > 0.0f) {
                scroll_by(-static_cast<f32>(e.data.mouse.wheel_delta) / 120.0f * kWheelStep);
                e.consumed = true;
            }
            break;

        case EventType::MouseDown:
            if (enabled() && (e.data.mouse.buttons & MouseButton_Left)) {
                const RectF g = global_bounds();
                const f32 x = e.data.mouse.x - g.left;
                const f32 y = e.data.mouse.y - g.top;
                if (scrollbar_hit(x)) {
                    const f32 track_top = kScrollbarMargin;
                    const f32 track_h = bounds_.height() - kScrollbarMargin * 2.0f;
                    const f32 ratio = track_h / content_height();
                    const f32 thumb_h = std::max(ratio * track_h, kThumbMinHeight);
                    const f32 thumb_y = track_top + (track_h - thumb_h) * (max_scroll_ > 0.0f ? scroll_y_ / max_scroll_ : 0.0f);
                    if (y >= thumb_y && y <= thumb_y + thumb_h) {
                        dragging_thumb_ = true;
                        drag_grab_ = y - thumb_y;
                    } else {
                        const f32 dy = track_h > thumb_h ? (y - thumb_y - thumb_h) : 0.0f;
                        scroll_by(dy / (track_h - thumb_h) * max_scroll_);
                    }
                    e.consumed = true;
                } else {
                    const i32 row = row_at_y(y);
                    set_selected(row);
                    e.consumed = true;
                }
            }
            break;

        case EventType::MouseMove:
            if (dragging_thumb_ && max_scroll_ > 0.0f) {
                const RectF g = global_bounds();
                const f32 y = e.data.mouse.y - g.top;
                const f32 track_top = kScrollbarMargin;
                const f32 track_h = bounds_.height() - kScrollbarMargin * 2.0f;
                const f32 ratio = track_h / content_height();
                const f32 thumb_h = std::max(ratio * track_h, kThumbMinHeight);
                const f32 range = track_h - thumb_h;
                if (range > 0.0f) {
                    set_scroll_y((y - drag_grab_ - track_top) / range * max_scroll_);
                }
                e.consumed = true;
                break;
            }
            {
                const RectF g = global_bounds();
                const i32 row = row_at_y(e.data.mouse.y - g.top);
                if (row != hovered_) {
                    hovered_ = row;
                    invalidate();
                }
            }
            break;

        case EventType::MouseUp:
            if (dragging_thumb_) {
                dragging_thumb_ = false;
                e.consumed = true;
            }
            break;

        case EventType::MouseLeave:
            if (hovered_ != -1) {
                hovered_ = -1;
                invalidate();
            }
            break;

        default:
            break;
    }
}

Widget* ListView::hit_test(f32 x, f32 y) {
    if (!visible()) return nullptr;
    if (x < 0.0f || y < 0.0f || x > bounds_.width() || y > bounds_.height()) return nullptr;
    if (scrollbar_hit(x)) return this;
    return Widget::hit_test(x, y);
}

i32 ListView::row_at_y(f32 y) const {
    const f32 cy = y + scroll_y_;
    if (cy < 0.0f) return -1;
    const i32 row = static_cast<i32>(cy / row_height_);
    return row >= count() ? -1 : row;
}

bool ListView::scrollbar_hit(f32 x) const {
    if (!show_scrollbar_ || max_scroll_ <= 0.0f) return false;
    const f32 bar_w = kScrollbarWidth + kScrollbarMargin;
    return x >= bounds_.width() - bar_w;
}

}  // namespace yzk
