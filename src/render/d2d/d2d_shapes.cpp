// D2D backend: shapes and gradients (rect / rounded / circle / linear / radial / sweep / border / line).
// The sweep gradient uses a custom D2D effect (see sweep_effect.cpp) with a snapshot input rebuilt per draw.
#include "d2d_backend.hpp"
#include "d2d_internal.hpp"

#include <cmath>

namespace yzk {

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

    D2D1_GRADIENT_STOP stops[2] = {
        {0.0f, to_d2d(color_a)},
        {1.0f, to_d2d(color_b)},
    };
    Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stop_collection;
    HRESULT hr = context_->CreateGradientStopCollection(
        stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stop_collection);
    if (FAILED(hr)) return;

    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> gradient;
    const D2D1_POINT_2F start = D2D1::Point2F(rect.left, rect.top);
    const D2D1_POINT_2F end = vertical ? D2D1::Point2F(rect.left, rect.bottom)
                                       : D2D1::Point2F(rect.right, rect.top);
    hr = context_->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(start, end), stop_collection.Get(), &gradient);
    if (FAILED(hr)) return;

    if (radius > 0.0f) {
        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> geometry;
        hr = factory_->CreateRoundedRectangleGeometry(to_d2d(rect, radius), &geometry);
        if (FAILED(hr)) return;
        context_->FillGeometry(geometry.Get(), gradient.Get());
    } else {
        context_->FillRectangle(to_d2d(rect), gradient.Get());
    }
}

void D2DBackend::fill_radial_gradient(const Point& center, f32 radius, const Color& color_a,
                                      const Color& color_b) {
    if (!context_ || !drawing_ || radius <= 0.0f) return;
    if (color_a.is_transparent() && color_b.is_transparent()) return;

    D2D1_GRADIENT_STOP stops[2] = {
        {0.0f, to_d2d(color_a)},
        {1.0f, to_d2d(color_b)},
    };
    Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> stop_collection;
    HRESULT hr = context_->CreateGradientStopCollection(
        stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stop_collection);
    if (FAILED(hr)) return;

    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> brush;
    hr = context_->CreateRadialGradientBrush(
        D2D1::RadialGradientBrushProperties(
            D2D1::Point2F(center.x, center.y), D2D1::Point2F(0.0f, 0.0f), radius, radius),
        stop_collection.Get(), &brush);
    if (FAILED(hr)) return;

    context_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius),
                          brush.Get());
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
    if (!context_ || !drawing_ || !layer_ || rect.empty()) return;
    if (color_a.is_transparent() && color_b.is_transparent()) return;
    if (!ensure_sweep_effect()) return;

    const f32 scale = static_cast<f32>(dpi_) / 96.0f;

    SweepParams params;
    params.center = D2D1::Point2F(center.x - rect.left, center.y - rect.top);
    params.size = D2D1::SizeF(rect.width(), rect.height());
    params.start_angle = start_angle;
    params.sweep_angle = sweep_angle > 0.0f ? sweep_angle : 6.2831853f;
    params.unused = 0.0f;
    params.color_a = to_d2d(color_a);
    params.color_b = to_d2d(color_b);
    sweep_effect_impl_->set_params(params);

    context_->Flush();

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> snapshot;
    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        static_cast<f32>(dpi_), static_cast<f32>(dpi_));
    const UINT32 w = static_cast<UINT32>(std::ceil(rect.width() * scale));
    const UINT32 h = static_cast<UINT32>(std::ceil(rect.height() * scale));
    if (w == 0 || h == 0) return;
    HRESULT hr = context_->CreateBitmap(D2D1::SizeU(w, h), nullptr, 0, props, &snapshot);
    if (FAILED(hr)) return;

    sweep_effect_->SetInput(0, snapshot.Get());

    const D2D1_POINT_2F origin = D2D1::Point2F(rect.left, rect.top);

    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> mask;
    if (radius > 0.0f) {
        hr = factory_->CreateRoundedRectangleGeometry(to_d2d(rect, radius), &mask);
        if (FAILED(hr)) return;
    }

    D2D1_LAYER_PARAMETERS1 layer_params = D2D1::LayerParameters1(
        D2D1::InfiniteRect(), mask.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
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