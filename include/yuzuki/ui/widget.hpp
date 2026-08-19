#pragma once
#include <yuzuki/core/event.hpp>
#include <yuzuki/core/types.hpp>
#include <yuzuki/ui/animation.hpp>
#include <yuzuki/ui/theme.hpp>

namespace yzk {

extern u64 g_layout_pass;

struct Margins {
    f32 left = 0.0f;
    f32 top = 0.0f;
    f32 right = 0.0f;
    f32 bottom = 0.0f;

    f32 horizontal() const { return left + right; }
    f32 vertical() const { return top + bottom; }
};

class Window;
class PaintContext;
class Widget {
public:
    Widget();
    virtual ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    friend class Window;

    void set_parent(Widget* parent);

    Widget* parent() const { return parent_; }
    Widget* first_child() const { return first_child_; }
    Widget* last_child() const { return last_child_; }
    Widget* next_sibling() const { return next_sibling_; }
    Widget* prev_sibling() const { return prev_sibling_; }

    void append_child(Widget* child);
    void remove_from_parent();
    void clear_children();

    Window* window() const;
    bool is_root() const { return parent_ == nullptr; }

    const RectF& bounds() const { return bounds_; }
    f32 x() const { return bounds_.left; }
    f32 y() const { return bounds_.top; }
    f32 width() const { return bounds_.width(); }
    f32 height() const { return bounds_.height(); }

    const Margins& margin() const { return margin_; }
    void set_margin(const Margins& margin) {
        margin_ = margin;
        invalidate();
    }
    void set_margin(f32 all) { set_margin(Margins{all, all, all, all}); }

    // bounds_ is relative to the parent; do NOT pass parent/global coordinates to
    // set_bounds — they would stack with parent-chain offsets. Custom containers
    // should subclass Layout and implement arrange_content(area).
    void set_bounds(const RectF& rect);
    void set_position(f32 x, f32 y);
    void set_size(f32 w, f32 h);

    void set_min_width(f32 w) {
        min_size_.width = w;
        invalidate();
    }
    void set_min_height(f32 h) {
        min_size_.height = h;
        invalidate();
    }
    void set_max_width(f32 w) {
        max_size_.width = w;
        invalidate();
    }
    void set_max_height(f32 h) {
        max_size_.height = h;
        invalidate();
    }
    void set_min_size(const Size& s) {
        min_size_ = s;
        invalidate();
    }
    void set_max_size(const Size& s) {
        max_size_ = s;
        invalidate();
    }
    const Size& min_size() const { return min_size_; }
    const Size& max_size() const { return max_size_; }

    // Flex grow/shrink factors (FlexBox containers only; default grow=0, shrink=1):
    // grow shares extra main-axis space, shrink contracts when space is short.
    void set_flex_grow(f32 grow) {
        flex_grow_ = grow < 0.0f ? 0.0f : grow;
        invalidate();
    }
    f32 flex_grow() const { return flex_grow_; }
    void set_flex_shrink(f32 shrink) {
        flex_shrink_ = shrink < 0.0f ? 0.0f : shrink;
        invalidate();
    }
    f32 flex_shrink() const { return flex_shrink_; }

    RectF global_bounds() const;

    bool visible() const { return (flags_ & Flag_Visible) != 0; }
    bool enabled() const { return (flags_ & Flag_Enabled) != 0; }
    bool focusable() const { return (flags_ & Flag_Focusable) != 0; }

    void set_visible(bool visible);
    void set_enabled(bool enabled);
    void set_focusable(bool focusable);
    void set_cursor(Cursor cursor) { cursor_ = cursor; }
    Cursor cursor() const { return cursor_; }

    void set_draggable(bool draggable);
    bool draggable() const { return has_flag(Flag_Draggable); }

    // ===== Visual transforms & opacity (animation foundation) =====
    // Pure-visual state over the layout position: translate, center rotate/scale,
    // opacity ([0,1]; 0 = fully transparent, skipped). No effect on layout/hit-test.
    // set_transition(ms) makes setters tween implicitly; animate_xxx() tweens explicitly.
    void set_transition(f32 ms);
    f32 transition() const { return transition_ms_; }

    void set_opacity(f32 opacity);
    f32 opacity() const { return opacity_.value(); }
    void set_translate(f32 dx, f32 dy);
    Point translate() const { return Point{translate_x_.value(), translate_y_.value()}; }
    void set_rotate_deg(f32 degrees);
    f32 rotate_deg() const { return rotate_deg_.value(); }
    void set_scale(f32 sx, f32 sy);
    f32 scale_x() const { return scale_x_.value(); }
    f32 scale_y() const { return scale_y_.value(); }
    // Affine transform from command space to window space for the current visual state
    Transform2D visual_transform() const;
    bool has_visual_state() const {
        return opacity_.value() < 1.0f || translate_x_.value() != 0.0f ||
               translate_y_.value() != 0.0f || rotate_deg_.value() != 0.0f ||
               scale_x_.value() != 1.0f || scale_y_.value() != 1.0f;
    }

    void set_tag(void* tag) { tag_ = tag; }
    void* tag() const { return tag_; }

    Size measure(Size available, const PaintContext* ctx = nullptr);
    const Size& desired_size() const { return desired_size_; }

virtual void perform_layout(const PaintContext* ctx = nullptr);
    // Non-virtual wrapper tracking painted_bounds_ so invalidate covers visuals drawn
    // outside bounds (shadows/glows). Subclasses override paint_impl.
    // CAUTION: inside paint_impl delegate to the base via X::paint_impl(ctx), never
    // paint(ctx) — that re-dispatches back here virtually, causing infinite recursion.
    void paint(PaintContext& ctx);
    virtual void paint_impl(PaintContext& ctx);
    virtual void on_event(Event& e);
    virtual Widget* hit_test(f32 x, f32 y);
    virtual bool is_window() const { return false; }

    void invalidate();
    void invalidate_area(const RectF& rect);

    // Window-space extent actually painted this frame; empty = never painted
    const RectF& painted_bounds() const { return painted_bounds_; }

    // Self-only (excl. subtree) painted extent, for invalidating own visual state
    const RectF& self_painted_bounds() const { return self_painted_bounds_; }
    bool has_self_visual() const { return !self_painted_bounds_.empty(); }

    // Current visual footprint (window coords): layout position after visual transforms.
    // Supplements stale painted_bounds_ after a move for culling checks.
    RectF visual_footprint() const;

    void request_focus();

protected:
    // Invalidates old visual extent plus the new transformed footprint (animation use)
    void invalidate_visual();
    virtual Size measure_impl(Size available, const PaintContext* ctx);
    enum Flags : u32 {
        Flag_Visible = 1 << 0,
        Flag_Enabled = 1 << 1,
        Flag_Focusable = 1 << 2,
        Flag_Hovered = 1 << 3,
        Flag_Pressed = 1 << 4,
        Flag_Draggable = 1 << 5,
    };

    void add_flag(u32 flag) { flags_ |= flag; }
    void remove_flag(u32 flag) { flags_ &= ~flag; }
    bool has_flag(u32 flag) const { return (flags_ & flag) != 0; }

    Widget* parent_ = nullptr;
    Widget* first_child_ = nullptr;
    Widget* last_child_ = nullptr;
    Widget* next_sibling_ = nullptr;
    Widget* prev_sibling_ = nullptr;

    RectF bounds_{0, 0, 0, 0};
    Margins margin_;
    u32 flags_ = Flag_Visible | Flag_Enabled;
    Cursor cursor_ = Cursor::Arrow;
    void* tag_ = nullptr;
    // Window-space extent drawn this frame (auto-tracked by the paint wrapper)
    RectF painted_bounds_{0, 0, 0, 0};
    // Self-only painted extent; only it is invalidated for own-state changes
    RectF self_painted_bounds_{0, 0, 0, 0};
    // Committed baseline: Window diffs painted_bounds_ against it and re-invalidates
    RectF painted_bounds_committed_{0, 0, 0, 0};
    Size min_size_{0.0f, 0.0f};
    Size max_size_{1e7f, 1e7f};
    Size desired_size_{0.0f, 0.0f};
    f32 flex_grow_ = 0.0f;
    f32 flex_shrink_ = 1.0f;
    u64 last_measure_pass_ = ~0ull;
    // AnimatableProperty<f32>: tweening with safe destruction (alive guard);
    // on_changed invalidates on any change
    f32 transition_ms_ = 0.0f;
    AnimatableProperty<f32> opacity_{1.0f};
    AnimatableProperty<f32> translate_x_{0.0f};
    AnimatableProperty<f32> translate_y_{0.0f};
    AnimatableProperty<f32> rotate_deg_{0.0f};
    AnimatableProperty<f32> scale_x_{1.0f};
    AnimatableProperty<f32> scale_y_{1.0f};
};

}  // namespace yzk