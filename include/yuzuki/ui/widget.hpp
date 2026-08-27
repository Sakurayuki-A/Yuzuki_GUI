#pragma once
#include <yuzuki/core/event.hpp>
#include <yuzuki/core/types.hpp>
#include <yuzuki/ui/animation.hpp>
#include <yuzuki/ui/theme.hpp>

#include <new>
#include <type_traits>
#include <utility>

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
    // Reference overload so fluent chains read naturally:
    //   row->append_child(new Button("Go")->on_click(fn).width(120));
    void append_child(Widget& child) { append_child(&child); }
    void remove_from_parent();
    void clear_children();

    // Lego-style builder: create a child, attach it, and return a reference so
    // the chain keeps working. Replaces the ugly
    //   append_child((new Button("Go"))->on_click(fn).width(120))
    // with
    //   row->add<Button>("Go").on_click(fn).width(120)
    // Ownership is identical to append_child(new T(...)): the widget is heap
    // allocated and this container holds a raw pointer (see ~Widget/clear_children).
    // T must be a Widget subclass and fully defined at the call site.
    template<typename T, typename... Args>
    T& add(Args&&... args) {
        static_assert(std::is_base_of<Widget, T>::value,
                      "Widget::add<T> requires a Widget subclass");
        T* child = new T(std::forward<Args>(args)...);
        append_child(child);
        return *child;
    }

    Window* window() const;
    bool is_root() const { return parent_ == nullptr; }

    const RectF& bounds() const { return bounds_; }
    f32 x() const { return bounds_.left; }
    f32 y() const { return bounds_.top; }
    f32 width() const { return bounds_.width(); }
    f32 height() const { return bounds_.height(); }

    // Fluent short names (Lego-style). width()/height()/margin() with an argument
    // are setters; with no argument they are getters.
    Widget& width(f32 w) { return set_min_width(w); }
    Widget& height(f32 h) { return set_min_height(h); }
    Widget& margin(f32 all) { return set_margin(all); }
    Widget& margin(const Margins& m) { return set_margin(m); }
    Widget& grow(f32 grow) { return set_flex_grow(grow); }

    const Margins& margin() const { return margin_; }
    Widget& set_margin(const Margins& margin) {
        margin_ = margin;
        invalidate();
        return *this;
    }
    Widget& set_margin(f32 all) { return set_margin(Margins{all, all, all, all}); }

    // bounds_ is relative to the parent; do NOT pass parent/global coordinates to
    // set_bounds — they would stack with parent-chain offsets. Custom containers
    // should subclass Layout and implement arrange_content(area).
    Widget& set_bounds(const RectF& rect);
    Widget& set_position(f32 x, f32 y);
    Widget& set_size(f32 w, f32 h);

    Widget& set_min_width(f32 w) {
        min_size_.width = w;
        invalidate();
        return *this;
    }
    Widget& set_min_height(f32 h) {
        min_size_.height = h;
        invalidate();
        return *this;
    }
    Widget& set_max_width(f32 w) {
        max_size_.width = w;
        invalidate();
        return *this;
    }
    Widget& set_max_height(f32 h) {
        max_size_.height = h;
        invalidate();
        return *this;
    }
    Widget& set_min_size(const Size& s) {
        min_size_ = s;
        invalidate();
        return *this;
    }
    Widget& set_max_size(const Size& s) {
        max_size_ = s;
        invalidate();
        return *this;
    }
    const Size& min_size() const { return min_size_; }
    const Size& max_size() const { return max_size_; }

    // Flex grow/shrink factors (FlexBox containers only; default grow=0, shrink=1):
    // grow shares extra main-axis space, shrink contracts when space is short.
    Widget& set_flex_grow(f32 grow) {
        flex_grow_ = grow < 0.0f ? 0.0f : grow;
        invalidate();
        return *this;
    }
    f32 flex_grow() const { return flex_grow_; }
    Widget& set_flex_shrink(f32 shrink) {
        flex_shrink_ = shrink < 0.0f ? 0.0f : shrink;
        invalidate();
        return *this;
    }
    f32 flex_shrink() const { return flex_shrink_; }

    RectF global_bounds() const;

    bool visible() const { return (flags_ & Flag_Visible) != 0; }
    bool enabled() const { return (flags_ & Flag_Enabled) != 0; }
    bool focusable() const { return (flags_ & Flag_Focusable) != 0; }

    Widget& set_visible(bool visible);
    Widget& set_enabled(bool enabled);
    Widget& set_focusable(bool focusable);
    Widget& set_cursor(Cursor cursor) {
        cursor_ = cursor;
        return *this;
    }
    Cursor cursor() const { return cursor_; }

    Widget& set_draggable(bool draggable);
    bool draggable() const { return has_flag(Flag_Draggable); }

    // ===== Visual transforms & opacity (animation foundation) =====
    // Pure-visual state over the layout position: translate, center rotate/scale,
    // opacity ([0,1]; 0 = fully transparent, skipped). No effect on layout/hit-test.
    // set_transition(ms) makes setters tween implicitly; animate_xxx() tweens explicitly.
    Widget& set_transition(f32 ms);
    f32 transition() const { return transition_ms_; }

    Widget& set_opacity(f32 opacity);
    f32 opacity() const { return opacity_.value(); }
    Widget& set_translate(f32 dx, f32 dy);
    Point translate() const { return Point{translate_x_.value(), translate_y_.value()}; }
    Widget& set_rotate_deg(f32 degrees);
    f32 rotate_deg() const { return rotate_deg_.value(); }
    Widget& set_scale(f32 sx, f32 sy);
    f32 scale_x() const { return scale_x_.value(); }
    f32 scale_y() const { return scale_y_.value(); }

    // Pointer-state queries.
    //  - hovered(): Flag_Hovered is maintained centrally by the Window (update_hover
    //    sets/clears it on enter/leave), so ANY widget — including custom subclasses
    //    that never touch events — gets a working hovered().
    //  - pressed(): passive read of Flag_Pressed. The flag is NOT maintained by the
    //    base; interactive controls (Button, RadioButton, NavItem-style selectables)
    //    set it in MouseDown and clear it in MouseUp themselves.
    bool hovered() const { return has_flag(Flag_Hovered); }
    bool pressed() const { return has_flag(Flag_Pressed); }
    // Affine transform from command space to window space for the current visual state
    Transform2D visual_transform() const;
    bool has_visual_state() const {
        return opacity_.value() < 1.0f || translate_x_.value() != 0.0f ||
               translate_y_.value() != 0.0f || rotate_deg_.value() != 0.0f ||
               scale_x_.value() != 1.0f || scale_y_.value() != 1.0f;
    }

    Size measure(Size available, const PaintContext* ctx = nullptr);
    const Size& desired_size() const { return desired_size_; }
#ifdef _DEBUG
    f32 measure_ms() const { return measure_ms_; }
#endif

virtual void perform_layout(const PaintContext* ctx = nullptr);
    // Non-virtual wrapper tracking painted_bounds_ so invalidate covers visuals drawn
    // outside bounds (shadows/glows). Subclasses override paint_impl.
    // CAUTION: inside paint_impl delegate to the base via X::paint_impl(ctx), never
    // paint(ctx) — that re-dispatches back here virtually, causing infinite recursion.
    void paint(PaintContext& ctx);
    virtual void paint_impl(PaintContext& ctx);
    // Event handler: called synchronously during message drain (WndProc → dispatch).
    // CONTRACT: handlers MUST return in <1 ms. Long-running work (I/O, network, decode)
    // must be deferred to the frame boundary; the framework provides no yield point
    // inside the drain loop. Violating this stalls all windows and animations.
    virtual void on_event(Event& e);
    virtual Widget* hit_test(f32 x, f32 y);
    virtual bool is_window() const { return false; }

    void invalidate();
    void invalidate_area(const RectF& rect);

    // Window-space extent actually painted this frame; empty = never painted
    const RectF& painted_bounds() const { return painted_bounds_; }

    // Reset painted_bounds_ to current layout bounds. Call after a permanent shrink
    // (e.g. collapse animation complete) so the culling rect doesn't stay enlarged
    // at the historical maximum. The union semantics in paint() will re-expand on
    // the next frame if needed.
    void reset_painted_bounds();

    // Self-only (excl. subtree) painted extent, for invalidating own visual state
    const RectF& self_painted_bounds() const { return self_painted_bounds_; }
    bool has_self_visual() const { return !self_painted_bounds_.empty(); }

    // Current visual footprint (window coords): layout position after visual transforms.
    // Supplements stale painted_bounds_ after a move for culling checks.
    RectF visual_footprint() const;

    void request_focus();

    // ===== IME (input method editor) =====
    // Whether the focused widget wants composition input (text-entry controls).
    virtual bool wants_ime() const { return false; }
    // Caret rectangle in WINDOW coordinates; anchors the system composition/candidate
    // windows. Empty rect = no anchor (IME UI falls back to the window corner).
    virtual RectF ime_caret_rect() const { return RectF{}; }

    // ===== Floating widgets =====
    // Self-positioning overlays (context menus, popups) must opt out of parent
    // layouts: two writers on bounds_ (panel arrange + own anchor) fight every frame
    // and cause perpetual invalidation.
    virtual bool participates_in_layout() const { return true; }

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
#ifdef _DEBUG
    f32 measure_ms_ = 0.0f;  // cumulative measure_impl time (debug builds only)
#endif
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