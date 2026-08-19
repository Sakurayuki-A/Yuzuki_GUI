#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string>
#include <limits>

namespace yzk {

using String = std::string;
using WString = std::wstring;

using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

constexpr f32 kHuge = std::numeric_limits<f32>::max() / 4.0f;

struct Point {
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Size {
    f32 width = 0.0f;
    f32 height = 0.0f;

    constexpr bool empty() const { return width <= 0.0f || height <= 0.0f; }
};

struct RectF {
    f32 left = 0.0f;
    f32 top = 0.0f;
    f32 right = 0.0f;
    f32 bottom = 0.0f;

    static constexpr RectF make(f32 x, f32 y, f32 w, f32 h) {
        return RectF{x, y, x + w, y + h};
    }

    constexpr f32 width() const { return right - left; }
    constexpr f32 height() const { return bottom - top; }
    constexpr Size size() const { return Size{width(), height()}; }
    constexpr Point top_left() const { return Point{left, top}; }
    constexpr bool empty() const { return width() <= 0.0f || height() <= 0.0f; }

    constexpr bool contains(f32 x, f32 y) const {
        return x >= left && x < right && y >= top && y < bottom;
    }

    constexpr bool contains(const Point& p) const { return contains(p.x, p.y); }

    constexpr bool intersects(const RectF& o) const {
        return left < o.right && o.left < right && top < o.bottom && o.top < bottom;
    }

    constexpr bool contains_rect(const RectF& o) const {
        return o.left >= left && o.right <= right && o.top >= top && o.bottom <= bottom;
    }

    RectF intersect(const RectF& o) const {
        return RectF{max(left, o.left), max(top, o.top), min(right, o.right), min(bottom, o.bottom)};
    }

    void unite(const RectF& o) {
        if (empty()) { *this = o; return; }
        if (o.empty()) return;
        left = min(left, o.left);
        top = min(top, o.top);
        right = max(right, o.right);
        bottom = max(bottom, o.bottom);
    }

    RectF inflated(f32 dx, f32 dy) const {
        return RectF{left - dx, top - dy, right + dx, bottom + dy};
    }

    RectF translated(f32 dx, f32 dy) const {
        return RectF{left + dx, top + dy, right + dx, bottom + dy};
    }

    bool operator==(const RectF& o) const {
        return left == o.left && top == o.top && right == o.right && bottom == o.bottom;
    }

    bool operator!=(const RectF& o) const { return !(*this == o); }

private:
    static constexpr f32 min(f32 a, f32 b) { return a < b ? a : b; }
    static constexpr f32 max(f32 a, f32 b) { return a > b ? a : b; }
};

struct CornerRadius {
    f32 top_left = 0.0f;
    f32 top_right = 0.0f;
    f32 bottom_right = 0.0f;
    f32 bottom_left = 0.0f;

    constexpr CornerRadius() = default;
    constexpr CornerRadius(f32 uniform) : top_left(uniform), top_right(uniform), bottom_right(uniform), bottom_left(uniform) {}
};

// 2D affine transform (column-vector convention): x' = m11*x + m12*y + m31, y' = m21*x + m22*y + m32.
// Composition: A * B means "apply B, then A" (matrices are transposes of D2D's row-vector layout).
struct Transform2D {
    f32 m11 = 1.0f, m12 = 0.0f;
    f32 m21 = 0.0f, m22 = 1.0f;
    f32 m31 = 0.0f, m32 = 0.0f;

    static constexpr Transform2D identity() { return {}; }

    static Transform2D translation(f32 dx, f32 dy) {
        return Transform2D{1.0f, 0.0f, 0.0f, 1.0f, dx, dy};
    }

    static Transform2D rotation_deg(f32 degrees) {
        const f32 r = degrees * 3.14159265358979323846f / 180.0f;
        const f32 c = std::cos(r);
        const f32 s = std::sin(r);
        return Transform2D{c, s, -s, c, 0.0f, 0.0f};
    }

    static Transform2D scaling(f32 sx, f32 sy) {
        return Transform2D{sx, 0.0f, 0.0f, sy, 0.0f, 0.0f};
    }

    // Apply transform t about center (cx, cy): T(cx,cy) * t * T(-cx,-cy)
    static Transform2D around(f32 cx, f32 cy, const Transform2D& t) {
        return Transform2D::translation(cx, cy) * t * Transform2D::translation(-cx, -cy);
    }

    bool is_identity() const {
        return m11 == 1.0f && m12 == 0.0f && m21 == 0.0f && m22 == 1.0f && m31 == 0.0f &&
               m32 == 0.0f;
    }

    Transform2D operator*(const Transform2D& o) const {
        return Transform2D{
            m11 * o.m11 + m12 * o.m21, m11 * o.m12 + m12 * o.m22,
            m21 * o.m11 + m22 * o.m21, m21 * o.m12 + m22 * o.m22,
            m11 * o.m31 + m12 * o.m32 + m31, m21 * o.m31 + m22 * o.m32 + m32};
    }

    Point apply(f32 x, f32 y) const {
        return Point{m11 * x + m12 * y + m31, m21 * x + m22 * y + m32};
    }

    // Transform the four corners and take the AABB (screen-space bounding box)
    RectF apply_rect(const RectF& r) const {
        const Point a = apply(r.left, r.top);
        const Point b = apply(r.right, r.top);
        const Point c = apply(r.right, r.bottom);
        const Point d = apply(r.left, r.bottom);
        return RectF{std::min(std::min(a.x, b.x), std::min(c.x, d.x)),
                     std::min(std::min(a.y, b.y), std::min(c.y, d.y)),
                     std::max(std::max(a.x, b.x), std::max(c.x, d.x)),
                     std::max(std::max(a.y, b.y), std::max(c.y, d.y))};
    }
};

struct Color {
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 255;

    constexpr Color() = default;
    constexpr Color(u8 r, u8 g, u8 b, u8 a = 255) : r(r), g(g), b(b), a(a) {}

    static constexpr Color rgba(u32 value) {
        return Color{static_cast<u8>((value >> 24) & 0xFF),
                     static_cast<u8>((value >> 16) & 0xFF),
                     static_cast<u8>((value >> 8) & 0xFF),
                     static_cast<u8>(value & 0xFF)};
    }

    constexpr Color with_alpha(u8 alpha) const { return Color{r, g, b, alpha}; }
    constexpr bool is_transparent() const { return a == 0; }
    constexpr bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    constexpr bool operator!=(const Color& o) const { return !(*this == o); }
};

enum class TextAlignH : u8 { Left, Center, Right };
enum class TextAlignV : u8 { Top, Center, Bottom };

enum class Cursor : u8 { Arrow, Hand, IBeam, Cross, SizeAll };

}  // namespace yzk