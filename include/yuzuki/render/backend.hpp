#pragma once
#include <yuzuki/core/types.hpp>

#include <vector>

namespace yzk {

// Shadow pad factor: the bitmap is generated at target rect + 2.5*blur so the
// blur isn't clipped at the edge; painted bounds account for this expansion.
constexpr f32 kShadowPadFactor = 2.5f;

// Shadow cache key quantization grid (DIP): sizes snap to the grid so animations
// reuse cached bitmaps (slight stretch is invisible on soft shadows) instead of
// regenerating a shadow map every frame, which delays shadows and spikes frame time.
constexpr f32 kShadowGrid = 4.0f;

struct FontSpec {
    String family = "Satoshi";
    f32 size = 14.0f;
    u16 weight = 400;
    bool italic = false;

    bool operator==(const FontSpec& o) const {
        return family == o.family && size == o.size && weight == o.weight && italic == o.italic;
    }

    bool operator<(const FontSpec& o) const {
        if (family != o.family) return family < o.family;
        if (size != o.size) return size < o.size;
        if (weight != o.weight) return weight < o.weight;
        return italic < o.italic;
    }
};

using FontId = u32;
constexpr FontId kInvalidFont = 0;

using BitmapId = u32;
constexpr BitmapId kInvalidBitmap = 0;

struct Paint {
    Color fill{0, 0, 0, 0};
    Color stroke{0, 0, 0, 0};
    f32 stroke_width = 0.0f;
    CornerRadius radius{0.0f};

    bool filled() const { return !fill.is_transparent(); }
    bool stroked() const { return !stroke.is_transparent() && stroke_width > 0.0f; }
};

struct BackendInfo {
    String name;
    u32 version_major = 0;
    u32 version_minor = 0;
};

struct TextSelectionRect {
    RectF rect;
    i32 begin = 0;
    i32 end = 0;
};

class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    virtual BackendInfo info() const = 0;

    virtual bool create_target(void* native_window, u32 width_px, u32 height_px, u32 dpi) = 0;
    virtual void destroy_target() = 0;
    virtual bool resize(u32 width_px, u32 height_px) = 0;
    virtual void set_dpi(u32 dpi) = 0;
    virtual u32 dpi() const = 0;
    virtual f32 dpi_scale() const { return static_cast<f32>(dpi()) / 96.0f; }

    virtual bool begin_frame(const Color& clear, const RectF* clip_dip) = 0;
    virtual bool end_frame() = 0;
    // True if deferred resources (e.g. shadow maps) became ready this frame and need one more repaint to show.
    virtual bool shadows_pending_after_frame() const { return false; }

    // Partial redraw: begin_partial_frame starts a session (no full-screen clear), then
    // begin_damage_rect/end_damage_rect per dirty rect (fill background, then draw with GPU
    // clipping), and end_frame presents only the dirty rectangles. Default implementation is full-frame.
    virtual bool begin_partial_frame(const Color& clear) {
        (void)clear;
        return false;
    }
    virtual bool begin_damage_rect(const RectF& rect) {
        (void)rect;
        return false;
    }
    virtual void end_damage_rect() {}
    // Backdrop blur snapshot regions (window DIPs): snapshots read accumulated frame content, so a dirty
    // rect intersecting one must be repainted in full, or the snapshot captures stale pixels (drag ghosting).
    virtual const std::vector<RectF>& backdrop_regions() const {
        static const std::vector<RectF> kEmpty;
        return kEmpty;
    }

    virtual void begin_clip(const RectF& rect) = 0;
    virtual void end_clip() = 0;

    // Visual transform (opacity + affine): after push, this control (and its subtree) draws transformed;
    // opacity < 1 composites via a layer. Command coordinates are in input space and mapped to window
    // space by the D2D transform.
    virtual void push_visual(const Transform2D& transform, f32 opacity) = 0;
    virtual void pop_visual() = 0;

    virtual FontId create_font(const FontSpec& spec) = 0;
    virtual bool add_font_file(const String& path) = 0;
    virtual BitmapId load_bitmap(const String& path) = 0;
    virtual Size bitmap_size(BitmapId id) const = 0;
    virtual void draw_bitmap(BitmapId id, const RectF& rect, f32 radius = 0.0f) = 0;
    virtual Size measure_text(FontId font, const String& text, f32 max_width) = 0;
    virtual i32 hit_test_text(FontId font, const String& text, f32 width, f32 x, f32 y) {
        (void)font;
        (void)text;
        (void)width;
        (void)x;
        (void)y;
        return 0;
    }
    virtual Point caret_position(FontId font, const String& text, f32 width, i32 pos) {
        (void)font;
        (void)text;
        (void)width;
        (void)pos;
        return Point{};
    }
    virtual std::vector<TextSelectionRect> text_selection_rects(FontId font, const String& text,
                                                                f32 width, f32 height, i32 begin,
                                                                i32 end) {
        (void)font;
        (void)text;
        (void)width;
        (void)height;
        (void)begin;
        (void)end;
        return {};
    }
    virtual void draw_text(FontId font, const String& text, const RectF& rect,
                           const Color& color, TextAlignH align_h, TextAlignV align_v) = 0;

    virtual void fill_rect(const RectF& rect, const Color& color) = 0;
    virtual void fill_rounded(const RectF& rect, const Color& color, f32 radius) = 0;
    virtual void fill_circle(Point center, f32 radius, const Color& color) = 0;
    virtual void fill_gradient(const RectF& rect, const Color& color_a, const Color& color_b,
                               bool vertical = true, f32 radius = 0.0f) = 0;
    virtual void fill_radial_gradient(const Point& center, f32 radius, const Color& color_a,
                                      const Color& color_b) = 0;
    virtual void fill_sweep_gradient(const RectF& rect, const Point& center, f32 start_angle,
                                     f32 sweep_angle, const Color& color_a, const Color& color_b,
                                     f32 radius = 0.0f) = 0;
    virtual void draw_shadow(const RectF& rect, f32 radius, f32 blur, const Color& color) = 0;
    virtual bool draw_backdrop_blur(const RectF& rect, f32 blur, const Color& tint,
                                    f32 radius) = 0;
    virtual void draw_border(const RectF& rect, const Color& color, f32 width, f32 radius) = 0;
    virtual void draw_line(Point a, Point b, const Color& color, f32 width) = 0;
};

}  // namespace yzk