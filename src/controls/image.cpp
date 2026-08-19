#include <yuzuki/controls/image.hpp>
#include <yuzuki/ui/window.hpp>

#include <algorithm>

namespace yzk {

bool Image::load_from_file(Window& win, const String& path) {
    const BitmapId id = win.backend().load_bitmap(path);
    if (id == kInvalidBitmap) return false;
    bitmap_ = id;
    invalidate();
    return true;
}

void Image::set_bitmap(BitmapId id) {
    bitmap_ = id;
    invalidate();
}

Size Image::measure_impl(Size available, const PaintContext* ctx) {
    const Size raw = ctx->bitmap_size(bitmap_);
    if (raw.empty()) return Size{};
    if (mode_ == ImageScaleMode::None) return raw;
    if (mode_ == ImageScaleMode::Stretch) return available;
    f32 cap_w = available.width;
    f32 cap_h = available.height;
    if (max_size_.width > 0.0f) cap_w = std::min(cap_w, max_size_.width);
    if (max_size_.height > 0.0f) cap_h = std::min(cap_h, max_size_.height);
    const f32 scale = std::min(cap_w / raw.width, cap_h / raw.height);
    const f32 fit = std::min(scale, 1.0f);
    return Size{raw.width * fit, raw.height * fit};
}

void Image::paint_impl(PaintContext& ctx) {
    if (bitmap_ == kInvalidBitmap) return;
    const Size raw = ctx.bitmap_size(bitmap_);
    if (raw.empty()) return;

    RectF dest = bounds_;
    switch (mode_) {
        case ImageScaleMode::Stretch:
            break;

        case ImageScaleMode::None: {
            const f32 x = bounds_.left + (bounds_.width() - raw.width) * 0.5f;
            const f32 y = bounds_.top + (bounds_.height() - raw.height) * 0.5f;
            dest = RectF::make(x, y, raw.width, raw.height);
            break;
        }

        case ImageScaleMode::Contain: {
            const f32 scale =
                std::min(bounds_.width() / raw.width, bounds_.height() / raw.height);
            const f32 w = raw.width * scale;
            const f32 h = raw.height * scale;
            dest = RectF::make(bounds_.left + (bounds_.width() - w) * 0.5f,
                               bounds_.top + (bounds_.height() - h) * 0.5f, w, h);
            break;
        }

        case ImageScaleMode::Cover: {
            const f32 scale =
                std::max(bounds_.width() / raw.width, bounds_.height() / raw.height);
            const f32 w = raw.width * scale;
            const f32 h = raw.height * scale;
            dest = RectF::make(bounds_.left + (bounds_.width() - w) * 0.5f,
                               bounds_.top + (bounds_.height() - h) * 0.5f, w, h);
            break;
        }
    }

    ctx.draw_bitmap(bitmap_, dest, radius_);
}

}  // namespace yzk