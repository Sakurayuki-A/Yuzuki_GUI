// D2D backend: image decoding (WIC) and drawing (rounded clipping via layer + cached geometry).
#include "d2d_backend.hpp"
#include "d2d_internal.hpp"

namespace yzk {

BitmapId D2DBackend::load_bitmap(const String& path) {
    if (!context_) return kInvalidBitmap;
    static bool com_ok = [] {
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        return SUCCEEDED(hr) || hr == S_FALSE;
    }();
    if (!com_ok) return kInvalidBitmap;
    if (!wic_factory_) {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&wic_factory_));
        if (!wic_factory_) return kInvalidBitmap;
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    const std::wstring wide = utf::to_wide(path);
    HRESULT hr = wic_factory_->CreateDecoderFromFilename(
        wide.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) return kInvalidBitmap;

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return kInvalidBitmap;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wic_factory_->CreateFormatConverter(&converter);
    if (FAILED(hr)) return kInvalidBitmap;
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0f,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return kInvalidBitmap;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    hr = context_->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &bitmap);
    if (FAILED(hr)) return kInvalidBitmap;

    bitmaps_.push_back(bitmap);
    return static_cast<BitmapId>(bitmaps_.size());
}

Size D2DBackend::bitmap_size(BitmapId id) const {
    if (id == kInvalidBitmap || id > bitmaps_.size()) return Size{};
    const D2D1_SIZE_F size = bitmaps_[id - 1]->GetSize();
    const f32 scale = dpi_ / 96.0f;
    return Size{size.width / scale, size.height / scale};
}

void D2DBackend::draw_bitmap(BitmapId id, const RectF& rect, f32 radius) {
    if (!context_ || !drawing_ || id == kInvalidBitmap || id > bitmaps_.size()) return;

    if (radius <= 0.0f) {
        const D2D1_RECT_F r = to_d2d(rect);
        context_->DrawBitmap(bitmaps_[id - 1].Get(), &r, 1.0f,
                             D2D1_INTERPOLATION_MODE_LINEAR, nullptr, nullptr);
        return;
    }

    // Rounded clipping: reuse the layer and geometry (geometry recreated only when size/radius changes).
    if (!clip_layer_) context_->CreateLayer(&clip_layer_);
    const D2D1_RECT_F r = to_d2d(rect);
    const bool same = rounded_geometry_ &&
                      rounded_geometry_rect_.left == r.left && rounded_geometry_rect_.top == r.top &&
                      rounded_geometry_rect_.right == r.right &&
                      rounded_geometry_rect_.bottom == r.bottom &&
                      rounded_geometry_radius_ == radius;
    if (!same) {
        if (FAILED(factory_->CreateRoundedRectangleGeometry(
                D2D1::RoundedRect(r, radius, radius), &rounded_geometry_))) {
            return;
        }
        rounded_geometry_rect_ = r;
        rounded_geometry_radius_ = radius;
    }

    D2D1_LAYER_PARAMETERS1 params = D2D1::LayerParameters1(
        D2D1::InfiniteRect(), rounded_geometry_.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(), 1.0f, nullptr, D2D1_LAYER_OPTIONS1_NONE);
    context_->PushLayer(&params, clip_layer_.Get());
    context_->DrawBitmap(bitmaps_[id - 1].Get(), &r, 1.0f, D2D1_INTERPOLATION_MODE_LINEAR,
                         nullptr, nullptr);
    context_->PopLayer();
}

}  // namespace yzk
