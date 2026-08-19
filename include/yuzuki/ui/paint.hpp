#pragma once
#include <yuzuki/render/backend.hpp>
#include <yuzuki/ui/theme.hpp>

#include <map>
#include <vector>

namespace yzk {

class Widget;

// Paint command: produced during record (window coords with bounding box),
// clipped against damage rects at replay so only commands to rasterize are run.
struct PaintCommand {
    enum class Type : u8 {
        FillRect,
        FillRounded,
        FillCircle,
        FillGradient,
        FillRadialGradient,
        FillSweepGradient,
        DrawShadow,
        BackdropBlur,
        DrawBorder,
        DrawLine,
        DrawBitmap,
        DrawText,
        PushClip,
        PopClip,
        PushVisual,
        PopVisual,
    };

    Type type;
    RectF rect;  // window-space extent (intersection culling + drawing)
    Color color_a;
    Color color_b;
    f32 f1 = 0.0f;  // radius / blur / width / start_angle / opacity
    f32 f2 = 0.0f;  // blur / sweep_angle / radius
    f32 f3 = 0.0f;  // radius
    Point p1;
    Point p2;
    FontId font = kInvalidFont;
    BitmapId bitmap = kInvalidBitmap;
    bool b1 = false;  // gradient direction (vertical)
    TextAlignH align_h = TextAlignH::Center;
    TextAlignV align_v = TextAlignV::Center;
    String text;
    Transform2D transform;  // PushVisual: the widget's visual transform (input -> window space)
};

class PaintContext {
public:
    PaintContext(RenderBackend& backend, const Theme& theme);
    ~PaintContext();

    PaintContext(const PaintContext&) = delete;
    PaintContext& operator=(const PaintContext&) = delete;

    void push_clip(const RectF& rect);
    void pop_clip();

    // Pushes a visual transform; the widget's (and subtree's) subsequent drawing is
    // transformed, opacity composited via layers. Auto-called by Widget::paint.
    void push_visual(const Transform2D& transform, f32 opacity) const;
    void pop_visual() const;

    void set_offset(f32 x, f32 y);
    f32 offset_x() const { return offset_x_; }
    f32 offset_y() const { return offset_y_; }

    // ===== Command list rendering =====
    // After begin_record, draw_* only record commands (no backend); the widget tree
    // is traversed once per frame. damage = nullptr means a full frame (no culling).
    // replay() re-executes only commands intersecting each damage rect.
    void begin_record(const std::vector<RectF>* damage);
    void end_record();
    void replay(const RectF& damage_rect);
    // Commands recorded this frame (for perf stats)
    size_t command_count() const { return commands_.size(); }
    // Widget-level culling: skips a subtree whose last painted extent misses all damage rects
    bool paint_culled(const Widget* w) const;

    void fill_rect(const RectF& rect, const Color& color) const;
    void fill_rounded(const RectF& rect, const Color& color, f32 radius) const;
    void fill_circle(Point center, f32 radius, const Color& color) const;
    void fill_gradient(const RectF& rect, const Color& color_a, const Color& color_b,
                       bool vertical = true, f32 radius = 0.0f) const;
    void fill_radial_gradient(const Point& center, f32 radius, const Color& color_a,
                              const Color& color_b) const;
    void fill_sweep_gradient(const RectF& rect, const Point& center, f32 start_angle,
                             f32 sweep_angle, const Color& color_a, const Color& color_b,
                             f32 radius = 0.0f) const;
    void draw_shadow(const RectF& rect, f32 radius, f32 blur, const Color& color) const;
    bool draw_backdrop_blur(const RectF& rect, f32 blur, const Color& tint, f32 radius) const;
    void draw_border(const RectF& rect, const Color& color, f32 width, f32 radius) const;
    void draw_line(Point a, Point b, const Color& color, f32 width) const;

    Size bitmap_size(BitmapId id) const;
    void draw_bitmap(BitmapId id, const RectF& rect, f32 radius = 0.0f) const;

    void draw_text(const String& text, const RectF& rect, const Color& color,
                   TextAlignH align_h = TextAlignH::Center, TextAlignV align_v = TextAlignV::Center) const;
    void draw_text_small(const String& text, const RectF& rect, const Color& color,
                         TextAlignH align_h = TextAlignH::Left,
                         TextAlignV align_v = TextAlignV::Top) const;
    void draw_text_title(const String& text, const RectF& rect, const Color& color,
                         TextAlignH align_h = TextAlignH::Left,
                         TextAlignV align_v = TextAlignV::Top) const;

    // Cached FontId per family/size (created once per frame); draw glyphs with it
    FontId font(const String& family, f32 size, u16 weight = 400, bool italic = false) const;
    void draw_text(FontId font, const String& text, const RectF& rect, const Color& color,
                   TextAlignH align_h = TextAlignH::Center,
                   TextAlignV align_v = TextAlignV::Center) const;
    Size measure_text(FontId font, const String& text) const;

    Size measure_text(const String& text) const;
    Size measure_text(const String& text, bool small) const;
    Size measure_text(const String& text, bool small, f32 max_width) const;

    RenderBackend& backend() const { return backend_; }
    const Theme& theme() const { return theme_; }

    // Painted-extent tracking, auto-called by Widget::paint. begin_widget resets the
    // accumulator; end_widget returns the widget's window-space extent and merges it
    // into the parent's (nested). Transformed extents count when a visual is active.
    void begin_widget() const;
    RectF end_widget() const;
    // Current widget's self-only (excl. subtree) painted extent, for own-state invalidation
    RectF self_bounds() const { return self_stack_.empty() ? RectF{} : self_stack_.back(); }

private:
    // Accumulates window-space extents of actual draw calls (stack top = current
    // widget); applies the visual transform when one is active
    void track(const RectF& window_rect) const {
        if (painted_stack_.empty()) return;
        if (!visual_stack_.empty() && !visual_stack_.back().is_identity()) {
            const RectF transformed = visual_stack_.back().apply_rect(window_rect);
            painted_stack_.back().unite(transformed);
            if (!self_stack_.empty()) self_stack_.back().unite(transformed);
        } else {
            painted_stack_.back().unite(window_rect);
            if (!self_stack_.empty()) self_stack_.back().unite(window_rect);
        }
    }
    void push_command(PaintCommand cmd) const {
        commands_.push_back(std::move(cmd));
    }

    RenderBackend& backend_;
    const Theme& theme_;
    FontId font_;
    FontId font_small_;
    FontId font_title_;
    mutable std::map<FontSpec, FontId> fonts_;
    mutable std::vector<RectF> painted_stack_;
    mutable std::vector<RectF> self_stack_;  // current widget's self-only painted extent
    mutable std::vector<Transform2D> visual_stack_;  // visual transforms accumulated during record (right-multiplied)
    mutable std::vector<PaintCommand> commands_;
    const std::vector<RectF>* record_damage_ = nullptr;
    i32 clip_depth_ = 0;
    f32 offset_x_ = 0.0f;
    f32 offset_y_ = 0.0f;
};

}  // namespace yzk