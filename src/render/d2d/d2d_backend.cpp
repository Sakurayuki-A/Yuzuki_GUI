// D2D backend: device / swap chain / render target / frame lifecycle.
// Text, bitmaps, shapes, and effects live in d2d_text.cpp / d2d_bitmap.cpp / d2d_shapes.cpp / d2d_effects.cpp
#include "d2d_backend.hpp"
#include "d2d_internal.hpp"

#include <chrono>
#include <cmath>

namespace yzk {

D2DBackend::~D2DBackend() {
    destroy_target();
}

bool D2DBackend::create_target(void* native_window, u32 width_px, u32 height_px, u32 dpi) {
    hwnd_ = static_cast<HWND>(native_window);
    width_px_ = width_px;
    height_px_ = height_px;
    dpi_ = dpi;

    if (!factory_) {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&factory_));
        if (FAILED(hr)) return false;
    }
    if (!dwrite_) {
        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                         reinterpret_cast<IUnknown**>(dwrite_.GetAddressOf()));
        if (FAILED(hr)) return false;
    }
    if (!device_) {
        if (!create_device()) return false;
    }
    if (!swap_chain_) {
        if (!create_swap_chain(native_window, width_px, height_px)) return false;
    }
    return recreate_target();
}

void D2DBackend::destroy_target() {
    if (context_) context_->SetTarget(nullptr);
    target_bitmap_.Reset();
    layer_.Reset();
    blur_snapshot_.Reset();
    blur_snapshot_w_ = 0;
    blur_snapshot_h_ = 0;
    blur_effect_.Reset();
    sweep_snapshot_.Reset();
    sweep_snapshot_w_ = 0;
    sweep_snapshot_h_ = 0;
    sweep_snapshot_dpi_ = 0;
    swap_chain_.Reset();
    context_.Reset();
    device_.Reset();
    brush_.Reset();
    // Everything else is bound to the dead context/device and must not leak into the
    // next one (D2DERR_WRONG_RESOURCE_DOMAIN on the following EndDraw otherwise):
    // clip layers, visual-transform layers, cached geometry, the sweep effect instance.
    clip_layer_.Reset();
    visual_layer_stack_.clear();
    visual_layer_pool_.clear();
    rounded_geometry_.Reset();
    rounded_geometry_rect_ = D2D1_RECT_F{};
    rounded_geometry_radius_ = 0.0f;
    sweep_effect_.Reset();
    sweep_effect_impl_ = nullptr;
    // Invalidate all device-bound caches to avoid dangling bitmaps. Bitmap WIC sources
    // are kept so the device-dependent ID2D1Bitmaps rebuild lazily on next use and
    // BitmapIds stay valid across device loss.
    shadow_cache_.clear();
    pending_shadows_.clear();
    gradient_cache_.clear();
    gradient_lru_.clear();
    for (BitmapEntry& entry : bitmaps_) entry.bitmap.Reset();
    capture_staging_.Reset();
}

bool D2DBackend::create_device() {
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_factory_));
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; dxgi_factory_->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc1{};
        if (SUCCEEDED(adapter->GetDesc1(&desc1))) {
            const bool software = (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
            if (software) {
                adapter.Reset();
                continue;
            }
            break;
        }
    }

    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device;
    hr = D3D11CreateDevice(
        adapter.Get(), adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
        &d3d_device, nullptr, nullptr);
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &d3d_device, nullptr, nullptr);
        if (FAILED(hr)) return false;
    }
    d3d_device_ = d3d_device;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    hr = d3d_device.As(&dxgi_device);
    if (FAILED(hr)) return false;

    hr = factory_->CreateDevice(dxgi_device.Get(), &device_);
    if (FAILED(hr)) return false;

    hr = device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context_);
    if (FAILED(hr)) return false;

    return true;
}

bool D2DBackend::create_swap_chain(void* native_window, u32 width_px, u32 height_px) {
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width_px;
    desc.Height = height_px;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    HRESULT hr = dxgi_factory_->CreateSwapChainForHwnd(
        d3d_device_.Get(), static_cast<HWND>(native_window), &desc, nullptr, nullptr, &swap_chain_);
    if (FAILED(hr) && desc.SwapEffect != DXGI_SWAP_EFFECT_DISCARD) {
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = dxgi_factory_->CreateSwapChainForHwnd(
            d3d_device_.Get(), static_cast<HWND>(native_window), &desc, nullptr, nullptr, &swap_chain_);
    }
    if (FAILED(hr)) return false;

    hr = dxgi_factory_->MakeWindowAssociation(static_cast<HWND>(native_window),
                                              DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) return false;

    return true;
}

bool D2DBackend::recreate_target() {
    if (!context_ || !swap_chain_) return false;
    if (target_bitmap_) {
        context_->SetTarget(nullptr);
        target_bitmap_.Reset();
    }
    layer_.Reset();

    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    HRESULT hr = swap_chain_->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) return false;

    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        static_cast<f32>(dpi_), static_cast<f32>(dpi_));
    hr = context_->CreateBitmapFromDxgiSurface(surface.Get(), &props, &target_bitmap_);
    if (FAILED(hr)) return false;

    // 16-bit float intermediate: the whole compositing chain (content -> layer ->
    // blur snapshot -> layer) accumulates in FP16 so soft gradients never quantize
    // into visible bands; the only 8-bit step is the final Present to the swapchain.
    D2D1_BITMAP_PROPERTIES1 layer_props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, D2D1_ALPHA_MODE_PREMULTIPLIED),
        static_cast<f32>(dpi_), static_cast<f32>(dpi_));
    hr = context_->CreateBitmap(D2D1::SizeU(width_px_, height_px_), nullptr, 0, layer_props,
                                &layer_);
    if (FAILED(hr)) return false;

    context_->SetTarget(layer_.Get());
    context_->SetDpi(static_cast<f32>(dpi_), static_cast<f32>(dpi_));
    return true;
}

bool D2DBackend::resize(u32 width_px, u32 height_px) {
    if (width_px == 0 || height_px == 0) {
        width_px_ = width_px;
        height_px_ = height_px;
        return true;
    }
    if (!swap_chain_) {
        width_px_ = width_px;
        height_px_ = height_px;
        return true;
    }
    if (context_ && target_bitmap_) {
        context_->SetTarget(nullptr);
        target_bitmap_.Reset();
    }
    HRESULT hr = swap_chain_->ResizeBuffers(0, width_px, height_px, DXGI_FORMAT_UNKNOWN, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        return false;
    }
    if (FAILED(hr)) return false;
    width_px_ = width_px;
    height_px_ = height_px;
    return recreate_target();
}

void D2DBackend::set_dpi(u32 dpi) {
    if (dpi == dpi_) return;
    dpi_ = dpi;
    destroy_target();
    if (hwnd_ && width_px_ > 0 && height_px_ > 0) {
        create_target(hwnd_, width_px_, height_px_, dpi_);
    }
}

bool D2DBackend::begin_frame(const Color& clear) {
    if (!context_ || !target_bitmap_ || !layer_ || drawing_) return false;
    backdrop_regions_.clear();
    // Render deferred shadow bitmaps here, before BeginDraw: nested SetTarget +
    // BeginDraw/EndDraw inside a frame would produce blank bitmaps. Generate all
    // pending shadows at once; spreading them over frames delays large first-frame
    // shadows by hundreds of milliseconds.
    while (!pending_shadows_.empty()) {
        const ShadowRequest req = pending_shadows_.front();
        pending_shadows_.erase(pending_shadows_.begin());
        if (!render_shadow_bitmap(req.width, req.height, req.radius, req.blur)) break;
    }
    drawing_ = true;

    context_->SetTarget(layer_.Get());
    context_->BeginDraw();
    context_->SetTransform(D2D1::IdentityMatrix());
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    // Grayscale text AA is deliberate, not an override for its own sake: ClearType's
    // per-channel coverage assumes glyphs blend straight onto the physical pixel grid.
    // Rendered into our intermediate layer and composited 1:1, the R/B fringes survive
    // as colored speckles along glyph edges on many panels. Grayscale + device-pixel
    // origin snapping (see draw_text) is the crisp-and-clean combination for a
    // composited-UI pipeline (same tradeoff WPF makes).
    context_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    // Sanity: the visual transform/layer stacks should be empty (push/pop are paired);
    // clear leftovers to avoid cross-frame bleed.
    visual_transform_stack_.clear();
    visual_layer_stack_.clear();

    const D2D1_COLOR_F clear_color = to_d2d(clear);
    context_->Clear(&clear_color);
    return true;
}

bool D2DBackend::begin_partial_frame(const Color& clear) {
    if (!context_ || !target_bitmap_ || !layer_ || drawing_) return false;
    backdrop_regions_.clear();
    damage_clear_ = clear;
    drawing_ = true;

    context_->SetTarget(layer_.Get());
    context_->BeginDraw();
    context_->SetTransform(D2D1::IdentityMatrix());
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    // Same rationale as begin_frame: grayscale text AA for the composited pipeline.
    context_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    // Sanity: the visual transform/layer stacks should be empty (push/pop are paired);
    // clear leftovers to avoid cross-frame bleed.
    visual_transform_stack_.clear();
    visual_layer_stack_.clear();
    return true;
}

bool D2DBackend::begin_damage_rect(const RectF& rect) {
    if (!context_ || !drawing_) return false;
    // Inflate the dirty rect by 2 DIP to cover antialiased edges, avoiding 1px residue at damage boundaries.
    RectF r = rect.inflated(2.0f, 2.0f);
    if (r.empty()) return false;
    D2D1_RECT_F clip = to_d2d(r);
    context_->PushAxisAlignedClip(&clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    // Clear only the dirty area: FillRectangle respects clipping, Clear does not.
    if (ensure_brush(damage_clear_)) {
        D2D1_RECT_F fill = to_d2d(r);
        context_->FillRectangle(&fill, brush_.Get());
    }
    return true;
}

void D2DBackend::end_damage_rect() {
    if (context_ && drawing_) context_->PopAxisAlignedClip();
}

bool D2DBackend::end_frame() {
    if (!drawing_) return false;
    drawing_ = false;

    if (clip_pushed_) {
        context_->PopAxisAlignedClip();
        clip_pushed_ = false;
    }

    HRESULT hr = context_->EndDraw();
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        return handle_device_lost();
    }
    if (FAILED(hr)) return false;

    context_->SetTarget(target_bitmap_.Get());
    context_->SetTransform(D2D1::IdentityMatrix());
    // Composite pass: layer_ -> swapchain buffer, full screen every frame. Timed so
    // the offscreen-compositing cost is measured, not assumed (composite_ms_avg()).
    const auto composite_t0 = std::chrono::steady_clock::now();
    context_->BeginDraw();
    context_->DrawImage(layer_.Get(), D2D1::Point2F(0.0f, 0.0f), D2D1_INTERPOLATION_MODE_LINEAR,
                        D2D1_COMPOSITE_MODE_SOURCE_OVER);
    hr = context_->EndDraw();
    composite_ms_total_ +=
        std::chrono::duration<f64, std::milli>(std::chrono::steady_clock::now() - composite_t0)
            .count();
    ++composite_count_;
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        return handle_device_lost();
    }
    if (FAILED(hr)) return false;
    context_->SetTarget(layer_.Get());

    // Pixel capture: copy back buffer to staging texture before Present()
    // (FLIP_DISCARD discards the back buffer after Present).
    if (capture_frame_ && d3d_device_ && swap_chain_) {
        Microsoft::WRL::ComPtr<IDXGISurface> dxgi_surface;
        if (SUCCEEDED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&dxgi_surface)))) {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
            if (SUCCEEDED(dxgi_surface.As(&back_buffer))) {
                if (!capture_staging_) {
                    D3D11_TEXTURE2D_DESC desc{};
                    back_buffer->GetDesc(&desc);
                    desc.Usage = D3D11_USAGE_STAGING;
                    desc.BindFlags = 0;
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                    desc.MiscFlags = 0;
                    d3d_device_->CreateTexture2D(&desc, nullptr, &capture_staging_);
                }
                if (capture_staging_) {
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx;
                    d3d_device_->GetImmediateContext(&ctx);
                    if (ctx) ctx->CopyResource(capture_staging_.Get(), back_buffer.Get());
                }
            }
        }
    }

    // Deferred shadows are rendered here, after the drawing session ended (EndDraw
    // succeeded), where nested SetTarget + BeginDraw/EndDraw is safe; request an
    // immediate repaint so shadows appear instead of waiting for the next natural
    // invalidation (up to hundreds of milliseconds).
    shadow_work_done_ = false;
    if (!pending_shadows_.empty()) {
        // Generate all pending shadows at once; splitting across frames delays large
        // first-frame shadows by hundreds of milliseconds.
        while (!pending_shadows_.empty()) {
            const ShadowRequest req = pending_shadows_.front();
            pending_shadows_.erase(pending_shadows_.begin());
            if (render_shadow_bitmap(req.width, req.height, req.radius, req.blur)) {
                shadow_work_done_ = true;
            }
        }
    }

    // Present: the layer_ framebuffer is copied to the swapchain in full every frame
    // (FLIP_DISCARD gives no backbuffer persistence, so partial presents can never
    // apply here — the dirty-rect policy lives entirely in the layer replay above).
    hr = swap_chain_->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        return handle_device_lost();
    }
    return SUCCEEDED(hr);
}

bool D2DBackend::handle_device_lost() {
    destroy_target();
    if (hwnd_ && width_px_ > 0 && height_px_ > 0) {
        create_target(hwnd_, width_px_, height_px_, dpi_);
    }
    return false;
}

bool D2DBackend::recreate_after_loss() {
    destroy_target();
    return hwnd_ && width_px_ > 0 && height_px_ > 0 &&
           create_target(hwnd_, width_px_, height_px_, dpi_);
}

void D2DBackend::begin_clip(const RectF& rect) {
    if (!context_ || !drawing_) return;
    D2D1_RECT_F clip = to_d2d(rect);
    context_->PushAxisAlignedClip(&clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void D2DBackend::end_clip() {
    if (!context_ || !drawing_) return;
    context_->PopAxisAlignedClip();
}

void D2DBackend::push_visual(const Transform2D& transform, f32 opacity) {
    if (!context_ || !drawing_) return;
    D2D1_MATRIX_3X2_F cur;
    context_->GetTransform(&cur);
    visual_transform_stack_.push_back(cur);
    if (opacity < 1.0f) {
        // Opacity < 1: draw content into a layer first, then composite with the opacity
        // (the layer composites in the transform space captured at push time).
        Microsoft::WRL::ComPtr<ID2D1Layer> layer;
        if (!visual_layer_pool_.empty()) {
            layer = visual_layer_pool_.back();
            visual_layer_pool_.pop_back();
        } else {
            context_->CreateLayer(&layer);
        }
        D2D1_LAYER_PARAMETERS1 params{};
        params.contentBounds = D2D1::InfiniteRect();
        params.opacity = opacity;
        params.maskTransform = D2D1::IdentityMatrix();
        context_->PushLayer(params, layer.Get());
        visual_layer_stack_.push_back(layer);
    }
    // Command coordinates are in input space: set the accumulated transform so subsequent
    // commands (including clips) are mapped to window space by D2D.
    context_->SetTransform(to_d2d(transform));
}

void D2DBackend::pop_visual() {
    if (!context_ || !drawing_) return;
    // Restore the transform before PopLayer so the layer composites in the outer
    // (restored) transform space.
    if (!visual_transform_stack_.empty()) {
        context_->SetTransform(visual_transform_stack_.back());
        visual_transform_stack_.pop_back();
    }
    if (!visual_layer_stack_.empty()) {
        context_->PopLayer();
        visual_layer_pool_.push_back(visual_layer_stack_.back());
        visual_layer_stack_.pop_back();
    }
}

bool D2DBackend::ensure_brush(const Color& color) {
    if (brush_) {
        brush_->SetColor(to_d2d(color));
        return true;
    }
    HRESULT hr = context_->CreateSolidColorBrush(to_d2d(color), &brush_);
    return SUCCEEDED(hr);
}

bool D2DBackend::capture_pixels(std::vector<u8>& bgra_out, u32& width, u32& height) {
    if (!capture_staging_) return false;
    D3D11_TEXTURE2D_DESC desc{};
    capture_staging_->GetDesc(&desc);
    width = desc.Width;
    height = desc.Height;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx;
    d3d_device_->GetImmediateContext(&ctx);
    if (!ctx) return false;
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = ctx->Map(capture_staging_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) return false;
    const u32 bpp = 4;  // BGRA8
    bgra_out.resize(static_cast<size_t>(width) * height * bpp);
    if (mapped.RowPitch == width * bpp) {
        memcpy(bgra_out.data(), mapped.pData, bgra_out.size());
    } else {
        const u8* src = static_cast<const u8*>(mapped.pData);
        u8* dst = bgra_out.data();
        for (u32 y = 0; y < height; ++y) {
            memcpy(dst, src + y * mapped.RowPitch, width * bpp);
            dst += width * bpp;
        }
    }
    ctx->Unmap(capture_staging_.Get(), 0);
    return true;
}

void D2DBackend::release_capture() { capture_staging_.Reset(); }

}  // namespace yzk


