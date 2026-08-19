#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/window.hpp>

namespace yzk {

u64 g_layout_pass = 0;

Widget::~Widget() {
    // Unbind window state pointers first (the parent chain is still intact here, so window() is valid)
    if (Window* win = window()) win->detach_widget(this);
    remove_from_parent();
    clear_children();
}

void Widget::set_parent(Widget* parent) {
    parent_ = parent;
}

void Widget::append_child(Widget* child) {
    if (!child || child->parent_ == this) return;
    if (child->parent_) child->remove_from_parent();

    child->parent_ = this;
    child->next_sibling_ = nullptr;
    child->prev_sibling_ = last_child_;
    if (last_child_) {
        last_child_->next_sibling_ = child;
        last_child_ = child;
    } else {
        first_child_ = last_child_ = child;
    }
    child->last_measure_pass_ = ~0ull;
    last_measure_pass_ = ~0ull;
    child->invalidate();
}

void Widget::remove_from_parent() {
    if (!parent_) return;
    Widget* p = parent_;
    parent_ = nullptr;

    if (p->first_child_ == this) p->first_child_ = next_sibling_;
    if (p->last_child_ == this) p->last_child_ = prev_sibling_;
    if (prev_sibling_) prev_sibling_->next_sibling_ = next_sibling_;
    if (next_sibling_) next_sibling_->prev_sibling_ = prev_sibling_;
    next_sibling_ = nullptr;
    prev_sibling_ = nullptr;
    p->last_measure_pass_ = ~0ull;
    last_measure_pass_ = ~0ull;
    p->invalidate();
}

void Widget::clear_children() {
    Widget* child = first_child_;
    while (child) {
        Widget* next = child->next_sibling_;
        child->parent_ = nullptr;
        child->next_sibling_ = nullptr;
        child->prev_sibling_ = nullptr;
        child->last_measure_pass_ = ~0ull;
        child = next;
    }
    first_child_ = last_child_ = nullptr;
    last_measure_pass_ = ~0ull;
}

Window* Widget::window() const {
    const Widget* w = this;
    while (w->parent_) w = w->parent_;
    return w->is_window() ? static_cast<Window*>(const_cast<Widget*>(w)) : nullptr;
}

void Widget::set_bounds(const RectF& rect) {
    if (bounds_ == rect) return;
    RectF old = bounds_;
    bounds_ = rect;
    invalidate_area(old);
    invalidate_area(bounds_);
    // Visual spill (shadow/glow) moves with the widget; invalidate the old painted extent too
    if (!painted_bounds_.empty()) {
        Window* win = window();
        if (win) win->invalidate_area(painted_bounds_);
    }
}

void Widget::set_position(f32 x, f32 y) {
    set_bounds(RectF::make(x, y, width(), height()));
}

void Widget::set_size(f32 w, f32 h) {
    set_bounds(RectF::make(bounds_.left, bounds_.top, w, h));
}

void Widget::set_draggable(bool draggable) {
    if (draggable) {
        add_flag(Flag_Draggable);
    } else {
        remove_flag(Flag_Draggable);
    }
}

// ===== Visual transforms & opacity =====

Widget::Widget() {
    // Any visual property change (incl. per-frame tween progress) invalidates
    const auto wire = [this](AnimatableProperty<f32>& p) {
        p.set_on_changed([this](const f32&) { invalidate_visual(); });
    };
    wire(opacity_);
    wire(translate_x_);
    wire(translate_y_);
    wire(rotate_deg_);
    wire(scale_x_);
    wire(scale_y_);
}

Transform2D Widget::visual_transform() const {
    if (!has_visual_state()) return Transform2D::identity();
    const RectF gb = global_bounds();
    const f32 cx = gb.left + gb.width() * 0.5f;
    const f32 cy = gb.top + gb.height() * 0.5f;
    // CSS semantics: translate outermost, rotate/scale around the center
    Transform2D t =
        Transform2D::translation(cx + translate_x_.value(), cy + translate_y_.value());
    t = t * Transform2D::rotation_deg(rotate_deg_.value());
    t = t * Transform2D::scaling(scale_x_.value(), scale_y_.value());
    t = t * Transform2D::translation(-cx, -cy);
    return t;
}

RectF Widget::visual_footprint() const {
    const RectF gb = global_bounds();
    if (gb.empty()) return gb;
    if (has_visual_state()) {
        const Transform2D t = visual_transform();
        return t.is_identity() ? gb : t.apply_rect(gb);
    }
    return gb;
}

void Widget::invalidate_visual() {
    Window* win = window();
    if (!win) return;
    // Invalidate the old footprint (last frame's painted extent)
    if (!painted_bounds_.empty()) win->invalidate_area(painted_bounds_);
    // New footprint: global layout rect transformed by the new visual state
    const RectF gb = global_bounds();
    if (!gb.empty()) {
        const Transform2D t = visual_transform();
        win->invalidate_area(t.is_identity() ? gb : t.apply_rect(gb));
    }
}

void Widget::set_transition(f32 ms) {
    transition_ms_ = ms < 0.0f ? 0.0f : ms;
    opacity_.set_transition(transition_ms_);
    translate_x_.set_transition(transition_ms_);
    translate_y_.set_transition(transition_ms_);
    rotate_deg_.set_transition(transition_ms_);
    scale_x_.set_transition(transition_ms_);
    scale_y_.set_transition(transition_ms_);
}

void Widget::set_opacity(f32 opacity) {
    opacity = opacity < 0.0f ? 0.0f : (opacity > 1.0f ? 1.0f : opacity);
    opacity_.set_animated(opacity);
}

void Widget::set_translate(f32 dx, f32 dy) {
    translate_x_.set_animated(dx);
    translate_y_.set_animated(dy);
}

void Widget::set_rotate_deg(f32 degrees) {
    rotate_deg_.set_animated(degrees);
}

void Widget::set_scale(f32 sx, f32 sy) {
    scale_x_.set_animated(sx);
    scale_y_.set_animated(sy);
}

RectF Widget::global_bounds() const {
    RectF r = bounds_;
    const Widget* w = parent_;
    while (w) {
        r = r.translated(w->bounds_.left, w->bounds_.top);
        w = w->parent_;
    }
    return r;
}

void Widget::set_visible(bool visible) {
    const bool was = (flags_ & Flag_Visible) != 0;
    if (was == visible) return;
    if (visible) {
        flags_ |= Flag_Visible;
    } else {
        flags_ &= ~Flag_Visible;
        remove_flag(Flag_Hovered);
        remove_flag(Flag_Pressed);
    }
    invalidate();
}

void Widget::set_enabled(bool enabled) {
    const bool was = (flags_ & Flag_Enabled) != 0;
    if (was == enabled) return;
    if (enabled) {
        flags_ |= Flag_Enabled;
    } else {
        flags_ &= ~Flag_Enabled;
        remove_flag(Flag_Hovered);
        remove_flag(Flag_Pressed);
    }
    invalidate();
}

void Widget::set_focusable(bool focusable) {
    if (focusable) {
        flags_ |= Flag_Focusable;
    } else {
        flags_ &= ~Flag_Focusable;
    }
}

Size Widget::measure(Size available, const PaintContext* ctx) {
    last_measure_pass_ = g_layout_pass;
    Size s = measure_impl(available, ctx);
    if (s.width < min_size_.width) s.width = min_size_.width;
    if (s.height < min_size_.height) s.height = min_size_.height;
    if (s.width > max_size_.width) s.width = max_size_.width;
    if (s.height > max_size_.height) s.height = max_size_.height;
    desired_size_ = s;
    return desired_size_;
}

Size Widget::measure_impl(Size available, const PaintContext* ctx) {
    (void)ctx;
    const Size s = bounds_.size();
    return Size{s.width < available.width ? s.width : available.width,
                s.height < available.height ? s.height : available.height};
}

void Widget::perform_layout(const PaintContext* ctx) {
    // Default simple-container behavior: measure children, then fill them into this area.
    // Complex containers (StackPanel/DockPanel/GridPanel) override this.
    Widget* child = first_child();
    while (child) {
        if (child->visible()) {
            if (child->last_measure_pass_ != g_layout_pass) {
                child->measure(bounds_.size(), ctx);
            }
            // Only fill children not yet laid out (empty bounds); manually
            // positioned children (set_bounds) must not be stretched
            if (child->bounds().empty()) {
                child->set_bounds(bounds_);
            }
            child->perform_layout(ctx);
        }
        child = child->next_sibling();
    }
}

void Widget::paint(PaintContext& ctx) {
    // Widget-level culling: skip the subtree when its last painted extent misses all
    // damage rects, so partial-repaint CPU cost scales with damage size, not UI size.
    if (ctx.paint_culled(this)) return;
    if (opacity_.value() <= 0.0f) return;  // fully transparent: nothing to draw
    ctx.begin_widget();
    const bool visual = has_visual_state();
    if (visual) ctx.push_visual(visual_transform(), opacity_.value());
    paint_impl(ctx);
    if (visual) ctx.pop_visual();
    const RectF self_bb = ctx.self_bounds();
    const RectF pb = ctx.end_widget();
    // painted_bounds_ = visual occupancy (culling/invalidation); it must never shrink
    // with partial-frame culling or reconcile oscillates. Unioning the visual
    // footprint keeps a floor; command extents cover shadows/glows.
    RectF occupied = pb;
    const RectF vf = visual_footprint();
    if (!vf.empty()) occupied.unite(vf);
    if (!occupied.empty()) painted_bounds_ = occupied;
    if (!self_bb.empty()) self_painted_bounds_ = self_bb;
}

void Widget::paint_impl(PaintContext& ctx) {
    const f32 ox = ctx.offset_x();
    const f32 oy = ctx.offset_y();
    ctx.set_offset(ox + bounds_.left, oy + bounds_.top);
    for (Widget* child = first_child_; child; child = child->next_sibling_) {
        if (!child->visible()) continue;
        child->paint(ctx);
    }
    ctx.set_offset(ox, oy);
}

void Widget::on_event(Event& e) {
    (void)e;
}

Widget* Widget::hit_test(f32 x, f32 y) {
    if (!visible()) return nullptr;
    if (x < 0.0f || y < 0.0f || x > bounds_.width() || y > bounds_.height()) return nullptr;
    for (Widget* child = last_child_; child; child = child->prev_sibling_) {
        if (!child->visible()) continue;
        Widget* hit = child->hit_test(x - child->bounds_.left, y - child->bounds_.top);
        if (hit) return hit;
    }
    return this;
}

void Widget::invalidate() {
    // Invalidate the actual painted extent first (covers shadow/glow spill; window coords)
    Window* win = window();
    if (!win) return;
    if (!painted_bounds_.empty()) {
        win->invalidate_area(painted_bounds_);
    } else {
        invalidate_area(bounds_);
    }
}

void Widget::invalidate_area(const RectF& rect) {
    Window* win = window();
    if (!win) return;
    f32 ox = 0.0f, oy = 0.0f;
    const Widget* w = parent_;
    while (w) {
        ox += w->bounds_.left;
        oy += w->bounds_.top;
        w = w->parent_;
    }
    win->invalidate_area(rect.translated(ox, oy));
}

void Widget::request_focus() {
    Window* win = window();
    if (win && focusable()) win->set_focus(this);
}

}  // namespace yzk