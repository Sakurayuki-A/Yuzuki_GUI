#pragma once
#include <yuzuki/core/types.hpp>

#include <d2d1_1.h>
#include <d2d1effectauthor.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace yzk {

constexpr GUID kSweepGradientClsid = {
    0x0f8f2a5c, 0x9b3a, 0x4e7d, {0xa1, 0xc6, 0x5d, 0x2e, 0x8b, 0x44, 0xf7, 0x90}};

struct SweepParams {
    D2D1_POINT_2F center;
    D2D1_SIZE_F size;
    f32 start_angle;
    f32 sweep_angle;
    f32 unused;
    f32 pad;  // keep float4 16-byte aligned (matches HLSL cbuffer layout)
    D2D1_COLOR_F color_a;
    D2D1_COLOR_F color_b;
};

// Custom interface to set gradient parameters without XML property plumbing.
class __declspec(uuid("0F8F2A5C-9B3A-4E7D-A1C6-5D2E8B44F790")) ISweepGradient : public IUnknown {
public:
    virtual HRESULT set_params(const SweepParams& params) = 0;
};

class __declspec(uuid("0F8F2A5C-9B3A-4E7D-A1C6-5D2E8B44F790")) SweepGradientEffect final
    : public ID2D1EffectImpl,
      public ID2D1DrawTransform,
      public ISweepGradient {
public:
    SweepGradientEffect() = default;
    ~SweepGradientEffect() = default;

    SweepGradientEffect(const SweepGradientEffect&) = delete;
    SweepGradientEffect& operator=(const SweepGradientEffect&) = delete;

    static HRESULT register_effect(ID2D1Factory1* factory);
    static HRESULT create(ID2D1DeviceContext* context, ID2D1Effect** out_effect,
                          SweepGradientEffect** out_impl);

    // ID2D1EffectImpl
    HRESULT Initialize(ID2D1EffectContext* context, ID2D1TransformGraph* graph) override;
    HRESULT PrepareForRender(D2D1_CHANGE_TYPE change_type) override;
    HRESULT SetGraph(ID2D1TransformGraph* graph) override;

    // ID2D1DrawTransform
    HRESULT SetDrawInfo(ID2D1DrawInfo* draw_info) override;

    // ID2D1Transform
    HRESULT MapInputRectsToOutputRect(const D2D1_RECT_L* input_rects,
                                      const D2D1_RECT_L* input_opaque_sub_rects,
                                      UINT32 input_count, D2D1_RECT_L* output_rect,
                                      D2D1_RECT_L* output_opaque_sub_rect) override;
    HRESULT MapOutputRectToInputRects(const D2D1_RECT_L* output_rect, D2D1_RECT_L* input_rects,
                                      UINT32 input_count) const override;
    HRESULT MapInvalidRect(UINT32 input_index, D2D1_RECT_L invalid_input_rect,
                           D2D1_RECT_L* invalid_output_rect) const override;

    // ID2D1TransformNode
    UINT32 GetInputCount() const override;

    // ISweepGradient
    HRESULT set_params(const SweepParams& params) override;

    // IUnknown
    HRESULT QueryInterface(const IID& iid, void** object) override;
    ULONG AddRef() override;
    ULONG Release() override;

private:
    Microsoft::WRL::ComPtr<ID2D1EffectContext> context_;
    Microsoft::WRL::ComPtr<ID2D1DrawInfo> draw_info_;
    SweepParams params_{};
    ULONG ref_count_ = 1;
};

constexpr GUID kSweepShaderId = {
    0xa9b1c2d3, 0x4e5f, 0x6a7b, {0x8c, 0x9d, 0x0e, 0x1f, 0x2a, 0x3b, 0x4c, 0x5d}};

}  // namespace yzk