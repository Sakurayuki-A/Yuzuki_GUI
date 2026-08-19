#pragma once
// Internal D2D backend helpers: type conversions and effect GUIDs.
// For use only by src/render/d2d/*.cpp; not in the public headers.
#include <yuzuki/core/types.hpp>

#include <d2d1_1.h>
#include <d2d1helper.h>

namespace yzk {

inline D2D1_COLOR_F to_d2d(const Color& c) {
    return D2D1::ColorF(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

inline D2D1_RECT_F to_d2d(const RectF& r) {
    return D2D1::RectF(r.left, r.top, r.right, r.bottom);
}

inline D2D1_ROUNDED_RECT to_d2d(const RectF& r, f32 radius) {
    return D2D1::RoundedRect(to_d2d(r), radius, radius);
}

// Transform2D uses the column-vector convention; D2D matrices are row-vector (mutually transposed).
inline D2D1_MATRIX_3X2_F to_d2d(const Transform2D& t) {
    return D2D1::Matrix3x2F(t.m11, t.m21, t.m12, t.m22, t.m31, t.m32);
}

inline constexpr GUID kGaussianBlurClsid = {0x1feb6d69, 0x2fe6, 0x4ac9,
                                            {0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5}};

}  // namespace yzk
