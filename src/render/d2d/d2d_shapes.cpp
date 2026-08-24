// D2D backend: shapes and gradients (rect / rounded / circle / linear / radial / sweep / border / line).
// The sweep gradient uses a custom D2D effect (see sweep_effect.cpp) with a snapshot input rebuilt per draw.
#include "d2d_backend.hpp"
#include "d2d_internal.hpp"

#include <cmath>

namespace yzk {

ID2D1Brush* D2DBackend::ensure_gradient(const GradientCacheKey& key) {
    const auto it = gradient_cache_.find(key);
    if (it != gradient_cache_.end()) {
        gradient_lru_.splice(gradient_lru_.begin(), gradient_lru_, it->second.lru);
        return it->second.brush.Get();
    }

    D2D1_GRADIENT_STOP stops[2] = {
        {0.0f, to_d2d(key.color_a)},
        {1.0f, to_d2d(key.color_b)},
    };
    GradientCacheEntry entry;
    HRESULT hr = context_->CreateGradientStopCollection(
        stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &entry.stops);
    if (FAILED(hr)) return nullptr;

    // Origin-relative axis: the per-draw brush transform positions the gradient.
    if (key.kind == 0 || key.kind == 1) {  // linear vertical / horizontal
        const D2D1_POINT_2F end =
            key.kind == 0 ? D2D1::Point2F(0.0f, key.extent) : D2D1::Point2F(key.extent, 0.0f);
        Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> linear;
        hr = context_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.0f, 0.0f), end),
            entry.stops.Get(), &linear);
        entry.brush = std::move(linear);
    } else {  // radial
        Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> radial;
        hr = context_->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(D2D1::Point2F(0.0f, 0.0f),
                                                D2D1::Point2F(0.0f, 0.0f), key.extent,
                                                key.extent),
            entry.stops.Get(), &radial);
        entry.brush = std::move(radial);
    }
    if (FAILED(hr) || !entry.brush) return nullptr;

    gradient_lru_.push_front(key);
    entry.lru = gradient_lru_.begin();
    const auto inserted = gradient_cache_.emplace(key, std::move(entry));
    ID2D1Brush* brush = inserted.first->second.brush.Get();

    // LRU eviction: drop the least-recently-used brush when over capacity.
    constexpr size_t kMaxGradientBrushes = 128;
    while (gradient_cache_.size() > kMaxGradientBrushes) {
        gradient_cache_.erase(gradient_lru_.back());
        gradient_lru_.pop_back();
    }
    return brush;
}

void D2DBackend::fill_rect(const RectF& rect, const Color& color) {
    if (!ensure_brush(color)) return;
    context_->FillRectangle(to_d2d(rect), brush_.Get());
}

void D2DBackend::fill_rounded(const RectF& rect, const Color& color, f32 radius) {
    if (!ensure_brush(color)) return;
    context_->FillRoundedRectangle(to_d2d(rect, radius), brush_.Get());
}

void D2DBackend::fill_circle(Point center, f32 radius, const Color& color) {
    if (!ensure_brush(color)) return;
    context_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius),
                          brush_.Get());
}

void D2DBackend::fill_gradient(const RectF& rect, const Color& color_a, const Color& color_b,
                               bool vertical, f32 radius) {
    if (!context_ || !drawing_) return;

    GradientCacheKey key{static_cast<u8>(vertical ? 0 : 1),
                         vertical ? rect.height() : rect.width(), color_a, color_b};
    if (key.extent <= 0.0f) return;
    ID2D1Brush* gradient = ensure_gradient(key);
    if (!gradient) return;
    gradient->SetTransform(D2D1::Matrix3x2F::Translation(rect.left, rect.top));

    if (radius > 0.0f) {
        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> geometry;
        HRESULT hr = factory_->CreateRoundedRectangleGeometry(to_d2d(rect, radius), &geometry);
        if (FAILED(hr)) return;
        context_->FillGeometry(geometry.Get(), gradient);
    } else {
        context_->FillRectangle(to_d2d(rect), gradient);
    }
}

void D2DBackend::fill_radial_gradient(const Point& center, f32 radius, const Color& color_a,
                                      const Color& color_b) {
    if (!context_ || !drawing_ || radius <= 0.0f) return;
    if (color_a.is_transparent() && color_b.is_transparent()) return;

    GradientCacheKey key{2, radius, color_a, color_b};
    ID2D1Brush* brush = ensure_gradient(key);
    if (!brush) return;
    brush->SetTransform(D2D1::Matrix3x2F::Translation(center.x, center.y));

    context_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius),
                          brush);
}

bool D2DBackend::ensure_sweep_effect() {
    if (sweep_effect_) return true;
    if (!factory_) return false;
    HRESULT hr = SweepGradientEffect::register_effect(factory_.Get());
    if (FAILED(hr)) return false;
    hr = SweepGradientEffect::create(context_.Get(), &sweep_effect_, &sweep_effect_impl_);
    return SUCCEEDED(hr);
}

void D2DBackend::fill_sweep_gradient(const RectF& rect, const Point& center, f32 start_angle,
                                     f32 sweep_angle, const Color& color_a, const Color& color_b,
                                     f32 radius) {
    const std::vector<GradientStop> stops = {{0.0f, color_a}, {1.0f, color_b}};
    fill_sweep_gradient_stops(rect, center, start_angle, sweep_angle, stops, radius);
}

void D2DBackend::fill_sweep_gradient_stops(const RectF& rect, const Point& center,
                                           f32 start_angle, f32 sweep_angle,
                                           const std::vector<GradientStop>& stops_in,
                                           f32 radius) {
    if (!context_ || !drawing_ || !layer_ || rect.empty() || stops_in.empty()) return;
    bool all_transparent = true;
    for (const GradientStop& s : stops_in) {
        if (!s.color.is_transparent()) {
            all_transparent = false;
            break;
        }
    }
    if (all_transparent) return;
    if (!ensure_sweep_effect()) return;

    // Normalize: sort by position, clamp into [0,1], keep at most kMaxSweepStops.
    std::vector<GradientStop> stops = stops_in;
    std::sort(stops.begin(), stops.end(),
              [](const GradientStop& a, const GradientStop& b) { return a.position < b.position; });
    if (stops.size() > kMaxSweepStops) {
        // Even sampling of the overflow so the shape stays representative.
        const size_t n = stops.size();
        std::vector<GradientStop> reduced;
        reduced.reserve(kMaxSweepStops);
        for (u32 i = 0; i < kMaxSweepStops; ++i) {
            const size_t idx = i * (n - 1) / (kMaxSweepStops - 1);
            reduced.push_back(stops[idx]);
        }
        stops = std::move(reduced);
    }

    const f32 scale = static_cast<f32>(dpi_) / 96.0f;

    SweepParams params{};
    params.center = D2D1::Point2F(center.x - rect.left, center.y - rect.top);
    params.size = D2D1::SizeF(rect.width(), rect.height());
    params.start_angle = start_angle;
    params.sweep_angle = sweep_angle > 0.0f ? sweep_angle : 6.2831853f;
    params.stop_count = static_cast<f32>(stops.size());
    for (u32 i = 0; i < kMaxSweepStops; ++i) {
        const u32 src = static_cast<u32>(i < stops.size() ? i : stops.size() - 1);
        params.colors[i] = to_d2d(stops[src].color);
        const f32 pos = i < stops.size() ? stops[i].position : 1.0f;
        float* slot = &params.positions_packed[i / 4].r;
        slot[i % 4] = pos;
    }
    sweep_effect_impl_->set_params(params);

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> snapshot;
    const UINT32 w = static_cast<UINT32>(std::ceil(rect.width() * scale));
    const UINT32 h = static_cast<UINT32>(std::ceil(rect.height() * scale));
    if (w == 0 || h == 0) return;

    // Snapshot cached by (size, dpi): resizing a sweep-filled control recreates it,
    // every other draw reuses the blank input bitmap.
    if (sweep_snapshot_ && sweep_snapshot_w_ == w && sweep_snapshot_h_ == h &&
        sweep_snapshot_dpi_ == dpi_) {
        snapshot = sweep_snapshot_;
    } else {
        D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            static_cast<f32>(dpi_), static_cast<f32>(dpi_));
        HRESULT hr0 = context_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &snapshot);
        if (FAILED(hr0)) return;
        sweep_snapshot_ = snapshot;
        sweep_snapshot_w_ = w;
        sweep_snapshot_h_ = h;
        sweep_snapshot_dpi_ = dpi_;
    }

    sweep_effect_->SetInput(0, snapshot.Get());

    const D2D1_POINT_2F origin = D2D1::Point2F(rect.left, rect.top);

    // Rounded mask geometry cached per exact rect+radius (same pattern as draw_bitmap).
    if (radius > 0.0f) {
        const bool same_mask = rounded_geometry_ &&
                               rounded_geometry_rect_.left == rect.left &&
                               rounded_geometry_rect_.top == rect.top &&
                               rounded_geometry_rect_.right == rect.right &&
                               rounded_geometry_rect_.bottom == rect.bottom &&
                               rounded_geometry_radius_ == radius;
        if (!same_mask) {
            if (FAILED(factory_->CreateRoundedRectangleGeometry(
                    to_d2d(rect, radius), &rounded_geometry_))) {
                return;
            }
            rounded_geometry_rect_ = to_d2d(rect);
            rounded_geometry_radius_ = radius;
        }
    }

    D2D1_LAYER_PARAMETERS1 layer_params = D2D1::LayerParameters1(
        D2D1::InfiniteRect(),
        radius > 0.0f ? rounded_geometry_.Get() : nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(), 1.0f, nullptr, D2D1_LAYER_OPTIONS1_NONE);
    context_->PushLayer(layer_params, nullptr);
    context_->DrawImage(sweep_effect_.Get(), origin, D2D1_INTERPOLATION_MODE_LINEAR,
                        D2D1_COMPOSITE_MODE_SOURCE_OVER);
    context_->PopLayer();
}

void D2DBackend::draw_border(const RectF& rect, const Color& color, f32 width, f32 radius) {
    if (!ensure_brush(color)) return;
    D2D1_RECT_F r = to_d2d(rect);
    if (radius > 0.0f) {
        context_->DrawRoundedRectangle(to_d2d(rect, radius), brush_.Get(), width);
    } else {
        context_->DrawRectangle(&r, brush_.Get(), width);
    }
}

void D2DBackend::draw_line(Point a, Point b, const Color& color, f32 width) {
    if (!ensure_brush(color)) return;
    context_->DrawLine(D2D1::Point2F(a.x, a.y), D2D1::Point2F(b.x, b.y), brush_.Get(), width);
}

}  // namespace yzk