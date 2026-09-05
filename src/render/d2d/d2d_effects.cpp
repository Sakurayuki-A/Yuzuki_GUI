// D2D backend: effects — shadow bitmap cache (quantized grid + deferred generation) and
// backdrop blur (snapshot + gaussian blur). Shadow bitmaps are cached by (size, radius, blur);
// blur snapshot regions are reported to Window's dirty-rect policy.
#include "d2d_backend.hpp"
#include "d2d_internal.hpp"

#include <d2d1effects.h>

namespace yzk {

void D2DBackend::draw_shadow(const RectF& rect, f32 radius, f32 blur, const Color& color) {
    if (!context_ || !drawing_ || blur <= 0.0f || rect.empty()) return;
    // Skip when alpha is too low to be visible, avoiding wasted bitmap generation during fade-in animations.
    if (color.a < 8) return;

    // Quantize the cache key to a grid so small per-frame size changes reuse the same
    // bitmap. The grid adapts to blur: soft shadows hide the stretch, crisp ones don't —
    // a fixed 4 DIP grid made shadows step visibly while controls animated their size.
    const f32 grid = std::max(1.0f, std::min(kShadowGrid, blur * 0.5f));
    const f32 qw = std::round(rect.width() / grid) * grid;
    const f32 qh = std::round(rect.height() / grid) * grid;
    if (qw <= 0.0f || qh <= 0.0f) return;

    CachedShadow* cached = find_shadow(qw, qh, radius, blur);
    if (!cached) {
        // First frame: only register the request; generation is deferred to end_frame
        // (avoiding a hitch from generating all shadows within one frame).
        for (const ShadowRequest& req : pending_shadows_) {
            if (req.width == qw && req.height == qh && req.radius == radius && req.blur == blur) {
                return;
            }
        }
        pending_shadows_.push_back(ShadowRequest{qw, qh, radius, blur});
        return;
    }

    // Bitmap is generated at rect size + 2.5*blur margin and drawn 1:1 (no stretch) so corner
    // proportions don't distort with larger blur. FillOpacityMask tints it: color = shadow
    // color, opacity = bitmap alpha * color.a (dark shadow on light backgrounds, optional white glow on dark).
    if (!ensure_brush(color)) return;
    const f32 pad = blur * kShadowPadFactor;
    const D2D1_RECT_F dest =
        D2D1::RectF(rect.left - pad, rect.top - pad, rect.right + pad, rect.bottom + pad);
    // FillOpacityMask requires aliased antialiasing (shape comes from the bitmap alpha; no double-AA edges).
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    context_->FillOpacityMask(cached->bitmap.Get(), brush_.Get(), &dest, nullptr);
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

D2DBackend::CachedShadow* D2DBackend::find_shadow(f32 width, f32 height, f32 radius, f32 blur) {
    for (CachedShadow& cached : shadow_cache_) {
        if (cached.width == width && cached.height == height && cached.radius == radius &&
            cached.blur == blur) {
            cached.last_use = ++shadow_lru_tick_;
            return &cached;
        }
    }
    return nullptr;
}

bool D2DBackend::render_shadow_bitmap(f32 width, f32 height, f32 radius, f32 blur) {
    if (!context_ || !layer_ || blur <= 0.0f) return false;
    if (width <= 0.0f || height <= 0.0f) return false;
    if (find_shadow(width, height, radius, blur)) return true;

    const f32 scale = dpi_ / 96.0f;
    const f32 pad = blur * kShadowPadFactor * scale;
    const UINT32 w = static_cast<UINT32>(std::ceil(width * scale + pad * 2.0f));
    const UINT32 h = static_cast<UINT32>(std::ceil(height * scale + pad * 2.0f));
    if (w == 0 || h == 0) return false;

    // 16-bit float intermediate bitmap: avoids banding from 8-bit UNORM quantization on blur gradients.
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, D2D1_ALPHA_MODE_PREMULTIPLIED),
        static_cast<f32>(dpi_), static_cast<f32>(dpi_));

    // White rounded-rect source at rect size with a blur margin so the blur isn't clipped at the
    // edges; corner radius is clamped to half the rect to avoid degenerating into a circle.
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> source;
    if (FAILED(context_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &source))) return false;
    context_->SetTarget(source.Get());
    context_->BeginDraw();
    context_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    f32 corner = radius * scale;
    const f32 max_corner =
        (width < height ? width : height) * 0.5f * scale;
    if (corner > max_corner) corner = max_corner;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> white;
    if (FAILED(context_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &white))) {
        context_->EndDraw();
        context_->SetTarget(layer_.Get());
        return false;
    }
    context_->FillRoundedRectangle(
        D2D1::RoundedRect(
            D2D1::RectF(pad, pad, pad + width * scale, pad + height * scale), corner, corner),
        white.Get());
    context_->EndDraw();

    Microsoft::WRL::ComPtr<ID2D1Effect> effect;
    if (FAILED(context_->CreateEffect(kGaussianBlurClsid, &effect))) {
        context_->SetTarget(layer_.Get());
        return false;
    }
    effect->SetInput(0, source.Get());
    effect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, blur * scale);
    effect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_GAUSSIANBLUR_OPTIMIZATION_QUALITY);

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> output;
    if (FAILED(context_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &output))) {
        context_->SetTarget(layer_.Get());
        return false;
    }
    context_->SetTarget(output.Get());
    context_->BeginDraw();
    context_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    context_->DrawImage(effect.Get());
    context_->EndDraw();

    // Copy the blur into an ordinary bitmap (TARGET bitmaps silently fail as
    // DrawBitmap/FillOpacityMask sources); the float format keeps gradient precision.
    context_->SetTarget(layer_.Get());
    D2D1_BITMAP_PROPERTIES1 copy_props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, D2D1_ALPHA_MODE_PREMULTIPLIED),
        static_cast<f32>(dpi_), static_cast<f32>(dpi_));
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> final;
    if (FAILED(context_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, copy_props, &final))) {
        return false;
    }
    if (FAILED(final->CopyFromBitmap(nullptr, output.Get(), nullptr))) {
        return false;
    }

    // Cap the cache with LRU eviction: drop the least recently used shadow instead of
    // clearing everything (a full clear thrashed under many distinct shadow specs —
    // every entry regenerated the next frame, spiking frame time).
    constexpr size_t kMaxShadows = 64;
    if (shadow_cache_.size() >= kMaxShadows) {
        size_t victim = 0;
        for (size_t i = 1; i < shadow_cache_.size(); ++i) {
            if (shadow_cache_[i].last_use < shadow_cache_[victim].last_use) victim = i;
        }
        shadow_cache_.erase(shadow_cache_.begin() + static_cast<std::ptrdiff_t>(victim));
    }

    CachedShadow entry;
    entry.bitmap = final;
    entry.width = width;
    entry.height = height;
    entry.radius = radius;
    entry.blur = blur;
    shadow_cache_.push_back(std::move(entry));
    return true;
}

bool D2DBackend::ensure_blur_effect() {
    if (blur_effect_) return true;
    if (!context_) return false;
    return SUCCEEDED(context_->CreateEffect(kGaussianBlurClsid, &blur_effect_));
}

bool D2DBackend::draw_backdrop_blur(const RectF& rect, f32 blur, const Color& tint, f32 radius) {
    if (!context_ || !drawing_ || !layer_ || blur <= 0.0f) return false;

    // Register the snapshot region (rect + 3*blur margin) so Window's dirty-rect policy repaints
    // it in full this frame; otherwise the snapshot captures stale pixels.
    {
        const f32 reg_pad = blur * 3.0f;
        const RectF reg = rect.inflated(reg_pad, reg_pad);
        bool found = false;
        for (RectF& r : backdrop_regions_) {
            if (r == reg) {
                found = true;
                break;
            }
        }
        if (!found) backdrop_regions_.push_back(reg);
    }

    const f32 scale = static_cast<f32>(dpi_) / 96.0f;
    const f32 margin_dip = blur * 3.0f;

    f32 src_l = (rect.left - margin_dip) * scale;
    f32 src_t = (rect.top - margin_dip) * scale;
    f32 src_r = (rect.right + margin_dip) * scale;
    f32 src_b = (rect.bottom + margin_dip) * scale;

    src_l = src_l < 0.0f ? 0.0f : src_l;
    src_t = src_t < 0.0f ? 0.0f : src_t;
    src_r = src_r > static_cast<f32>(width_px_) ? static_cast<f32>(width_px_) : src_r;
    src_b = src_b > static_cast<f32>(height_px_) ? static_cast<f32>(height_px_) : src_b;
    if (src_r <= src_l || src_b <= src_t) return false;

    const UINT32 w = static_cast<UINT32>(std::ceil(src_r - src_l));
    const UINT32 h = static_cast<UINT32>(std::ceil(src_b - src_t));
    if (w == 0 || h == 0) return false;

    // Snapshot in FP16 matching the layer: no intermediate quantization before the blur.
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> snapshot;
    if (blur_snapshot_ && blur_snapshot_w_ == w && blur_snapshot_h_ == h) {
        snapshot = blur_snapshot_;
    } else {
        D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, D2D1_ALPHA_MODE_PREMULTIPLIED),
            static_cast<f32>(dpi_), static_cast<f32>(dpi_));
        HRESULT hr0 = context_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &snapshot);
        if (FAILED(hr0)) return false;
        blur_snapshot_ = snapshot;
        blur_snapshot_w_ = w;
        blur_snapshot_h_ = h;
    }

    // Flush before the read-back copy: layer_ is the CURRENT render target here, and
    // CopyFromBitmap can otherwise capture pixels before this frame's queued draws
    // reach the GPU -> the panel shows last-frame content (visible flicker whenever
    // the page changes). The sync is load-bearing; do not remove.
    context_->Flush();

    const D2D1_RECT_U src_rect = D2D1::RectU(static_cast<UINT32>(src_l), static_cast<UINT32>(src_t),
                                             static_cast<UINT32>(src_r), static_cast<UINT32>(src_b));
    if (FAILED(snapshot->CopyFromBitmap(nullptr, layer_.Get(), &src_rect))) return false;

    // Cached effect: SetInput/SetValue are cheap property updates; creating an effect
    // per draw was measurable churn on every blurred panel.
    if (!ensure_blur_effect()) return false;
    blur_effect_->SetInput(0, snapshot.Get());
    blur_effect_->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, blur * scale);

    const D2D1_POINT_2F origin = D2D1::Point2F(src_l / scale, src_t / scale);

    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> mask;
    if (radius > 0.0f) {
        if (FAILED(factory_->CreateRoundedRectangleGeometry(to_d2d(rect, radius), &mask))) {
            return false;
        }
    }

    D2D1_LAYER_PARAMETERS1 layer_params = D2D1::LayerParameters1(
        D2D1::InfiniteRect(), mask.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(), 1.0f, nullptr, D2D1_LAYER_OPTIONS1_NONE);
    context_->PushLayer(layer_params, nullptr);

    context_->DrawImage(blur_effect_.Get(), origin, D2D1_INTERPOLATION_MODE_LINEAR,
                        D2D1_COMPOSITE_MODE_SOURCE_OVER);

    if (!tint.is_transparent()) {
        if (!ensure_brush(tint)) {
            context_->PopLayer();
            return true;
        }
        context_->FillRectangle(to_d2d(rect), brush_.Get());
    }

    context_->PopLayer();
    return true;
}

}  // namespace yzk