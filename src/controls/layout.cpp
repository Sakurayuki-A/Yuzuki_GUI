#include <yuzuki/controls/layout.hpp>

namespace yzk {

Size Layout::measure_impl(Size available, const PaintContext* ctx) {
    Size inner = available;
    if (inner.width > padding_ * 2.0f) {
        inner.width -= padding_ * 2.0f;
    } else {
        inner.width = 0.0f;
    }
    if (inner.height > padding_ * 2.0f) {
        inner.height -= padding_ * 2.0f;
    } else {
        inner.height = 0.0f;
    }
    Size result = measure_content(inner, ctx);
    result.width += padding_ * 2.0f;
    result.height += padding_ * 2.0f;
    if (result.width > available.width) result.width = available.width;
    if (result.height > available.height) result.height = available.height;
    return result;
}

void Layout::perform_layout(const PaintContext* ctx) {
    if (last_measure_pass_ != g_layout_pass) measure(bounds_.size(), ctx);
    const RectF b = bounds_;
    const f32 w = b.width() > padding_ * 2.0f ? b.width() - padding_ * 2.0f : 0.0f;
    const f32 h = b.height() > padding_ * 2.0f ? b.height() - padding_ * 2.0f : 0.0f;
    arrange_content(RectF::make(padding_, padding_, w, h), ctx);
}

Size Layout::measure_content(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, 0.0f};
}

void Layout::arrange_content(const RectF& area, const PaintContext* ctx) {
    (void)area;
    (void)ctx;
}

}  // namespace yzk