#include <yuzuki/controls/progress_bar.hpp>

namespace yzk {

namespace {
constexpr f32 kHeight = 4.0f;
constexpr f32 kRadius = 2.0f;
constexpr f32 kMinWidth = 120.0f;
}

void ProgressBar::set_value(f32 value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    if (value_ == value) return;
    value_ = value;
    invalidate();
}

void ProgressBar::set_indeterminate(bool indeterminate) {
    if (indeterminate_ == indeterminate) return;
    indeterminate_ = indeterminate;
    invalidate();
}

Size ProgressBar::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{kMinWidth, kHeight};
}

void ProgressBar::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 y = bounds_.top + (bounds_.height() - kHeight) / 2.0f;
    const RectF track = RectF::make(bounds_.left, y, bounds_.width(), kHeight);

    ctx.fill_rounded(track, theme.track, kRadius);

    const f32 fill_w = indeterminate_ ? bounds_.width() * 0.35f : bounds_.width() * value_;
    if (fill_w > 0.0f) {
        const RectF fill = RectF::make(bounds_.left, y, fill_w, kHeight);
        ctx.fill_rounded(fill, theme.accent, kRadius);
    }
}

}  // namespace yzk
