#pragma once
#include <yuzuki/core/encoding.hpp>
#include <yuzuki/render/backend.hpp>

#include "sweep_effect.hpp"

#include <list>
#include <map>
#include <vector>

#include <string>

#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace yzk {

class D2DBackend final : public RenderBackend {
public:
    D2DBackend() = default;
    ~D2DBackend() override;

    bool create_target(void* native_window, u32 width_px, u32 height_px, u32 dpi) override;
    void destroy_target() override;
    bool resize(u32 width_px, u32 height_px) override;
    void set_dpi(u32 dpi) override;
    u32 dpi() const override { return dpi_; }
    f32 dpi_scale() const override { return dpi_ / 96.0f; }

    bool begin_frame(const Color& clear) override;
    bool end_frame() override;
    f64 composite_ms_avg() const override {
        return composite_count_ ? composite_ms_total_ / static_cast<f64>(composite_count_) : -1.0;
    }
    bool begin_partial_frame(const Color& clear) override;
    bool begin_damage_rect(const RectF& rect) override;
    void end_damage_rect() override;

    void begin_clip(const RectF& rect) override;
    void end_clip() override;
    void push_visual(const Transform2D& transform, f32 opacity) override;
    void pop_visual() override;

    FontId create_font(const FontSpec& spec) override;
    bool add_font_file(const String& path) override;
    BitmapId load_bitmap(const String& path) override;
    void unload_bitmap(BitmapId id) override;
    Size bitmap_size(BitmapId id) const override;
    void draw_bitmap(BitmapId id, const RectF& rect, f32 radius) override;
    Size measure_text(FontId font, const String& text, f32 max_width) override;
    i32 hit_test_text(FontId font, const String& text, f32 width, f32 x, f32 y) override;
    Point caret_position(FontId font, const String& text, f32 width, i32 pos) override;
    std::vector<TextSelectionRect> text_selection_rects(FontId font, const String& text, f32 width,
                                                        f32 height, i32 begin, i32 end) override;
    void draw_text(FontId font, const String& text, const RectF& rect,
                   const Color& color, TextAlignH align_h, TextAlignV align_v) override;

    void fill_rect(const RectF& rect, const Color& color) override;
    void fill_rounded(const RectF& rect, const Color& color, f32 radius) override;
    void fill_circle(Point center, f32 radius, const Color& color) override;
    void fill_gradient(const RectF& rect, const Color& color_a, const Color& color_b,
                       bool vertical, f32 radius) override;
    void fill_radial_gradient(const Point& center, f32 radius, const Color& color_a,
                              const Color& color_b) override;
    void fill_sweep_gradient(const RectF& rect, const Point& center, f32 start_angle,
                             f32 sweep_angle, const Color& color_a, const Color& color_b,
                             f32 radius) override;
    void fill_sweep_gradient_stops(const RectF& rect, const Point& center, f32 start_angle,
                                   f32 sweep_angle, const std::vector<GradientStop>& stops,
                                   f32 radius) override;
    void draw_shadow(const RectF& rect, f32 radius, f32 blur, const Color& color) override;
    bool draw_backdrop_blur(const RectF& rect, f32 blur, const Color& tint, f32 radius) override;
    void draw_border(const RectF& rect, const Color& color, f32 width, f32 radius) override;
    void draw_line(Point a, Point b, const Color& color, f32 width) override;

private:
    bool create_device();
    bool create_swap_chain(void* native_window, u32 width_px, u32 height_px);
    bool recreate_target();
    // Device lost/reset recovery: destroy and rebuild the target at the original size;
    // always returns false (the caller aborts the current frame).
    bool handle_device_lost();
    bool ensure_brush(const Color& color);
    bool ensure_sweep_effect();
    // Cached gaussian-blur effect for backdrop blur (created once per device).
    bool ensure_blur_effect();
    struct CachedShadow;
    bool render_shadow_bitmap(f32 width, f32 height, f32 radius, f32 blur);
    CachedShadow* find_shadow(f32 width, f32 height, f32 radius, f32 blur);
    bool get_layout(FontId font, const std::wstring& wide, f32 width, f32 height,
                    Microsoft::WRL::ComPtr<IDWriteTextLayout>* out_layout,
                    DWRITE_TEXT_METRICS* out_metrics);
    // Hit-testing / caret / selection layout: wraps at the REAL content height (read
    // back from metrics) instead of a fake 1e7 box, so is-inside and clamping semantics
    // match the drawn text for multi-line (CJK) content.
    bool get_hit_layout(FontId font, const std::wstring& wide, f32 width,
                        Microsoft::WRL::ComPtr<IDWriteTextLayout>* out_layout);

    // ===== Gradient brush cache (P1) =====
    // Brushes are created in origin-relative space and positioned per draw via
    // SetTransform(translation), so one cached brush serves every rect of the same
    // extent + color pair. LRU-bounded; cleared on device loss.
    struct GradientCacheKey {
        u8 kind = 0;  // 0 = linear vertical, 1 = linear horizontal, 2 = radial
        f32 extent = 0.0f;  // gradient length: rect height / width / radius
        Color color_a{};
        Color color_b{};

        bool operator<(const GradientCacheKey& o) const {
            if (kind != o.kind) return kind < o.kind;
            if (extent != o.extent) return extent < o.extent;
            if (color_a.r != o.color_a.r || color_a.g != o.color_a.g ||
                color_a.b != o.color_a.b || color_a.a != o.color_a.a) {
                return (color_a.r != o.color_a.r)
                           ? color_a.r < o.color_a.r
                           : (color_a.g != o.color_a.g)
                                 ? color_a.g < o.color_a.g
                                 : (color_a.b != o.color_a.b) ? color_a.b < o.color_a.b
                                                              : color_a.a < o.color_a.a;
            }
            return (color_b.r != o.color_b.r)
                       ? color_b.r < o.color_b.r
                       : (color_b.g != o.color_b.g)
                             ? color_b.g < o.color_b.g
                             : (color_b.b != o.color_b.b) ? color_b.b < o.color_b.b
                                                          : color_b.a < o.color_b.a;
        }
    };
    struct GradientCacheEntry {
        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stops;
        Microsoft::WRL::ComPtr<ID2D1Brush> brush;
        std::list<GradientCacheKey>::iterator lru;
    };
    // Returns nullptr on failure; keeps the entry MRU-fresh.
    ID2D1Brush* ensure_gradient(const GradientCacheKey& key);

    struct TextLayoutKey {
        FontId font = 0;
        std::wstring text;
        f32 width = 0.0f;
        f32 height = 0.0f;

        bool operator<(const TextLayoutKey& o) const {
            if (font != o.font) return font < o.font;
            if (width != o.width) return width < o.width;
            if (height != o.height) return height < o.height;
            return text < o.text;
        }
    };

    struct TextLayoutEntry {
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        DWRITE_TEXT_METRICS metrics{};
        std::list<TextLayoutKey>::iterator lru_it;
    };

    // Shadow bitmap cache, keyed by (rect size, radius, blur). Bitmaps are drawn 1:1
    // (no stretch) so corner proportions stay undistorted. LRU: last_use carries a
    // monotonic tick; eviction drops the least recently used entry instead of
    // clearing the whole cache.
    struct CachedShadow {
        Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
        f32 width = 0.0f;
        f32 height = 0.0f;
        f32 radius = 0.0f;
        f32 blur = 0.0f;
        u64 last_use = 0;
    };

    // Deferred shadow generation request: registered during the frame, rendered before the next
    // BeginDraw (nested SetTarget + BeginDraw/EndDraw inside a frame would produce blank bitmaps).
    struct ShadowRequest {
        f32 width = 0.0f;
        f32 height = 0.0f;
        f32 radius = 0.0f;
        f32 blur = 0.0f;
    };

    // True if end_frame generated shadow bitmaps that need an immediate repaint to appear.
    bool shadows_pending_after_frame() const { return shadow_work_done_; }
    f64 composite_ms_total_ = 0.0;  // layer->swapchain composite pass, cumulative wall ms
    u64 composite_count_ = 0;
    // Backdrop blur snapshot regions (window DIPs); a dirty rect intersecting one must be repainted in full.
    const std::vector<RectF>& backdrop_regions() const { return backdrop_regions_; }

    HWND hwnd_ = nullptr;
    u32 width_px_ = 0;
    u32 height_px_ = 0;
    u32 dpi_ = 96;
    bool drawing_ = false;
    bool clip_pushed_ = false;
    bool shadow_work_done_ = false;
    // Partial redraw: this frame's dirty rects (DIPs, inflated for AA edges), used as Present1 dirty rectangles.
    Color damage_clear_{0, 0, 0, 0};

    // Backdrop blur snapshot regions (window DIPs, with a 3*blur margin). The snapshot reads
    // accumulated layer content, so Window must merge any intersecting dirty rect into a full
    // repaint or the snapshot captures stale pixels (drag ghosting).
    std::vector<RectF> backdrop_regions_;

    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device_;
    Microsoft::WRL::ComPtr<ID2D1Factory1> factory_;
    Microsoft::WRL::ComPtr<ID2D1Device> device_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain_;
    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> target_bitmap_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> layer_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> blur_snapshot_;
    u32 blur_snapshot_w_ = 0;
    u32 blur_snapshot_h_ = 0;
    Microsoft::WRL::ComPtr<ID2D1Effect> blur_effect_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> sweep_snapshot_;
    u32 sweep_snapshot_w_ = 0;
    u32 sweep_snapshot_h_ = 0;
    u32 sweep_snapshot_dpi_ = 0;

    Microsoft::WRL::ComPtr<IDWriteFactory> dwrite_;
    Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection_;
    // All font files registered so far; the collection is rebuilt from this list on
    // every add_font_file so files accumulate instead of replacing each other.
    struct FontFileEntry {
        WString path;
        Microsoft::WRL::ComPtr<IDWriteFontFile> file;
    };
    std::vector<FontFileEntry> font_files_;
    std::vector<Microsoft::WRL::ComPtr<IDWriteTextFormat>> fonts_;
    std::map<FontSpec, FontId> font_cache_;
    std::map<TextLayoutKey, TextLayoutEntry> layout_cache_;
    // LRU order: list head is most-recently-used, tail is least-recently-used.
    std::list<TextLayoutKey> lru_order_;
    std::vector<CachedShadow> shadow_cache_;
    std::vector<ShadowRequest> pending_shadows_;
    u64 shadow_lru_tick_ = 0;  // monotonic stamp for shadow cache LRU

    std::map<GradientCacheKey, GradientCacheEntry> gradient_cache_;
    std::list<GradientCacheKey> gradient_lru_;

    Microsoft::WRL::ComPtr<IWICImagingFactory> wic_factory_;
    // Bitmaps survive device loss: the WIC source is kept so the device-dependent
    // ID2D1Bitmap can be rebuilt lazily on the next draw; BitmapIds stay valid.
    struct BitmapEntry {
        Microsoft::WRL::ComPtr<IWICBitmapSource> source;
        Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
        Size dip_size{};  // cached at load; queryable without a live device

        bool empty() const { return !source && !bitmap; }
    };
    std::vector<BitmapEntry> bitmaps_;
    // Recreates the device bitmap from its WIC source if needed; nullptr if unavailable.
    ID2D1Bitmap* ensure_bitmap(u32 index);
    Microsoft::WRL::ComPtr<ID2D1Layer> clip_layer_;
    // Visual transform stack (saved by push_visual, restored by pop_visual) and opacity layer stack.
    std::vector<D2D1_MATRIX_3X2_F> visual_transform_stack_;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Layer>> visual_layer_stack_;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Layer>> visual_layer_pool_;
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> rounded_geometry_;
    D2D1_RECT_F rounded_geometry_rect_{};
    f32 rounded_geometry_radius_ = 0.0f;
    Microsoft::WRL::ComPtr<ID2D1Effect> sweep_effect_;
    SweepGradientEffect* sweep_effect_impl_ = nullptr;
};

}  // namespace yzk
