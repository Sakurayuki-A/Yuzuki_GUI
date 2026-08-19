#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class Window;

// Image scale modes
enum class ImageScaleMode : u8 {
    Stretch,  // Stretch to fill (may distort)
    Contain,  // Scale to fit, fully visible
    Cover,    // Scale to fill, cropped
    None,     // Original size, centered
};

// Bitmap widget: loads files (PNG/JPEG etc., decoded via WIC), optional rounded corners
class Image : public Widget {
public:
    Image() = default;

    bool load_from_file(Window& win, const String& path);
    void set_bitmap(BitmapId id);
    BitmapId bitmap() const { return bitmap_; }

    void set_scale_mode(ImageScaleMode mode) {
        if (mode_ == mode) return;
        mode_ = mode;
        invalidate();
    }
    ImageScaleMode scale_mode() const { return mode_; }

    void set_corner_radius(f32 radius) {
        if (radius_ == radius) return;
        radius_ = radius;
        invalidate();
    }
    f32 corner_radius() const { return radius_; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    BitmapId bitmap_ = kInvalidBitmap;
    ImageScaleMode mode_ = ImageScaleMode::Contain;
    f32 radius_ = 0.0f;
};

}  // namespace yzk