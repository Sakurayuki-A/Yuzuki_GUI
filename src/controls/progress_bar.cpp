#include <yuzuki/controls/progress_bar.hpp>

namespace yzk {

namespace {
constexpr f32 kHeight = 4.0f;
constexpr f32 kRadius = 2.0f;
constexpr f32 kMinWidth = 120.0f;
constexpr f32 kSegmentFraction = 0.35f;  // indeterminate segment width vs track
constexpr f32 kSweepPeriodMs = 1200.0f;  // full left->right sweep duration
}

ProgressBar::~ProgressBar() {
    // Frame callbacks outlive widgets: unregister or the lambda dangles.
    if (anim_token_) AnimationSystem::instance().stop_frame(anim_token_);
}

ProgressBar& ProgressBar::set_value(f32 value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    if (value_ == value) return *this;
    value_ = value;
    invalidate();
    return *this;
}

ProgressBar& ProgressBar::set_indeterminate(bool indeterminate) {
    if (indeterminate_ == indeterminate) return *this;
    indeterminate_ = indeterminate;
    if (indeterminate_) {
        phase_ = 0.0f;
        // Real-time sampling keeps the sweep speed independent of frame rate; the
        // per-frame invalidate drives exactly the "small constant load" of a busy bar.
        anim_token_ = AnimationSystem::instance().on_frame([this](f32 now_ms) {
            phase_ = std::fmod(now_ms, kSweepPeriodMs) / kSweepPeriodMs;
            invalidate();
        });
    } else if (anim_token_) {
        AnimationSystem::instance().stop_frame(anim_token_);
        anim_token_ = 0;
    }
    invalidate();
    return *this;
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

    if (indeterminate_) {
        // Segment enters from the left and exits right; clipped to the track so the
        // sweep never paints outside the widget's invalidated extent.
        const f32 seg_w = bounds_.width() * kSegmentFraction;
        const f32 x = bounds_.left + (bounds_.width() + seg_w) * phase_ - seg_w;
        ctx.push_clip(track);
        ctx.fill_rounded(RectF::make(x, y, seg_w, kHeight), theme.accent, kRadius);
        ctx.pop_clip();
    } else {
        const f32 fill_w = bounds_.width() * value_;
        if (fill_w > 0.0f) {
            const RectF fill = RectF::make(bounds_.left, y, fill_w, kHeight);
            ctx.fill_rounded(fill, theme.accent, kRadius);
        }
    }
}

}  // namespace yzk
