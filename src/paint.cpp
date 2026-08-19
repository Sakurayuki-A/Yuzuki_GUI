#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/widget.hpp>

namespace yzk {

PaintContext::PaintContext(RenderBackend& backend, const Theme& theme)
    : backend_(backend), theme_(theme) {
    FontSpec spec;
    spec.family = theme.font_family;
    spec.size = theme.font_size;
    font_ = backend.create_font(spec);

    spec.size = theme.font_size_small;
    font_small_ = backend.create_font(spec);

    spec.size = theme.font_size_title;
    font_title_ = backend.create_font(spec);
}

PaintContext::~PaintContext() {
    while (clip_depth_ > 0) pop_clip();
}

void PaintContext::set_offset(f32 x, f32 y) {
    offset_x_ = x;
    offset_y_ = y;
}

void PaintContext::push_clip(const RectF& rect) {
    ++clip_depth_;
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::PushClip;
    cmd.rect = rect.translated(offset_x_, offset_y_);
    push_command(std::move(cmd));
}

void PaintContext::pop_clip() {
    if (clip_depth_ == 0) return;
    --clip_depth_;
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::PopClip;
    push_command(std::move(cmd));
}

void PaintContext::push_visual(const Transform2D& transform, f32 opacity) const {
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::PushVisual;
    cmd.transform = transform;
    cmd.f1 = opacity;
    push_command(std::move(cmd));
    // Accumulate during record: outer ∘ inner (track uses it for screen-space extents)
    if (visual_stack_.empty()) {
        visual_stack_.push_back(transform);
    } else {
        visual_stack_.push_back(visual_stack_.back() * transform);
    }
}

void PaintContext::pop_visual() const {
    if (visual_stack_.empty()) return;
    visual_stack_.pop_back();
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::PopVisual;
    push_command(std::move(cmd));
}

void PaintContext::begin_widget() const {
    painted_stack_.push_back(RectF{});
    self_stack_.push_back(RectF{});
}

RectF PaintContext::end_widget() const {
    if (painted_stack_.empty()) return RectF{};
    RectF r = painted_stack_.back();
    painted_stack_.pop_back();
    if (!painted_stack_.empty()) painted_stack_.back().unite(r);
    if (!self_stack_.empty()) self_stack_.pop_back();
    return r;
}

void PaintContext::begin_record(const std::vector<RectF>* damage) {
    commands_.clear();
    painted_stack_.clear();
    self_stack_.clear();
    visual_stack_.clear();
    record_damage_ = damage;
}

void PaintContext::end_record() {
    record_damage_ = nullptr;
}

bool PaintContext::paint_culled(const Widget* w) const {
    if (!record_damage_) return false;
    const RectF& pb = w->painted_bounds();
    if (pb.empty()) return false;  // never painted: unknown extent, conservatively not culled
    // After a layout move, painted_bounds_ is stale (old spot); re-check the current
    // visual footprint or widgets scrolled back into damage would be missed.
    const RectF cur = w->visual_footprint();
    if (cur.empty()) return false;  // not laid out / zero size: conservatively not culled
    for (const RectF& d : *record_damage_) {
        // 2px inflation matches begin_damage_rect, covering anti-aliased edges
        const RectF d2 = d.inflated(2.0f, 2.0f);
        if (pb.intersects(d2)) return false;
        if (cur.intersects(d2)) return false;
    }
    return true;
}

void PaintContext::replay(const RectF& damage_rect) {
    const bool partial = !damage_rect.empty();
    std::vector<Transform2D> matrix_stack;
    Transform2D matrix;  // accumulated visual transform (input space -> window space)
    for (const PaintCommand& cmd : commands_) {
        switch (cmd.type) {
            case PaintCommand::Type::PushClip:
                backend_.begin_clip(cmd.rect);
                continue;
            case PaintCommand::Type::PopClip:
                backend_.end_clip();
                continue;
            case PaintCommand::Type::PushVisual:
                matrix_stack.push_back(matrix);
                matrix = matrix * cmd.transform;
                backend_.push_visual(matrix, cmd.f1);
                continue;
            case PaintCommand::Type::PopVisual:
                backend_.pop_visual();
                if (!matrix_stack.empty()) {
                    matrix = matrix_stack.back();
                    matrix_stack.pop_back();
                }
                continue;
            default:
                break;
        }
        // Damage culling: skip commands not intersecting the damage rect (clip nesting is
        // kept by the Push/Pop above). The rect is inflated 2px like begin_damage_rect,
        // or background fills would erase anti-aliased edges (e.g. 1px borders) whose
        // commands were culled.
        if (partial) {
            RectF rc = cmd.rect;
            if (cmd.type == PaintCommand::Type::DrawShadow) {
                const f32 pad = cmd.f2 * kShadowPadFactor;
                rc = rc.inflated(pad, pad);
            }
            if (!matrix.is_identity()) rc = matrix.apply_rect(rc);
            if (!rc.intersects(damage_rect.inflated(2.0f, 2.0f))) continue;
        }
        switch (cmd.type) {
            case PaintCommand::Type::FillRect:
                backend_.fill_rect(cmd.rect, cmd.color_a);
                break;
            case PaintCommand::Type::FillRounded:
                backend_.fill_rounded(cmd.rect, cmd.color_a, cmd.f1);
                break;
            case PaintCommand::Type::FillCircle:
                backend_.fill_circle(cmd.p1, cmd.f1, cmd.color_a);
                break;
            case PaintCommand::Type::FillGradient:
                backend_.fill_gradient(cmd.rect, cmd.color_a, cmd.color_b, cmd.b1, cmd.f1);
                break;
            case PaintCommand::Type::FillRadialGradient:
                backend_.fill_radial_gradient(cmd.p1, cmd.f1, cmd.color_a, cmd.color_b);
                break;
            case PaintCommand::Type::FillSweepGradient:
                backend_.fill_sweep_gradient(cmd.rect, cmd.p1, cmd.f1, cmd.f2, cmd.color_a,
                                             cmd.color_b, cmd.f3);
                break;
            case PaintCommand::Type::DrawShadow:
                backend_.draw_shadow(cmd.rect, cmd.f1, cmd.f2, cmd.color_a);
                break;
            case PaintCommand::Type::BackdropBlur:
                backend_.draw_backdrop_blur(cmd.rect, cmd.f1, cmd.color_a, cmd.f2);
                break;
            case PaintCommand::Type::DrawBorder:
                backend_.draw_border(cmd.rect, cmd.color_a, cmd.f1, cmd.f2);
                break;
            case PaintCommand::Type::DrawLine:
                backend_.draw_line(cmd.p1, cmd.p2, cmd.color_a, cmd.f1);
                break;
            case PaintCommand::Type::DrawBitmap:
                backend_.draw_bitmap(cmd.bitmap, cmd.rect, cmd.f1);
                break;
            case PaintCommand::Type::DrawText:
                backend_.draw_text(cmd.font, cmd.text, cmd.rect, cmd.color_a, cmd.align_h,
                                   cmd.align_v);
                break;
            default:
                break;
        }
    }
}

void PaintContext::fill_rect(const RectF& rect, const Color& color) const {
    if (color.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::FillRect;
    cmd.rect = w;
    cmd.color_a = color;
    push_command(std::move(cmd));
}

void PaintContext::fill_rounded(const RectF& rect, const Color& color, f32 radius) const {
    if (color.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::FillRounded;
    cmd.rect = w;
    cmd.color_a = color;
    cmd.f1 = radius;
    push_command(std::move(cmd));
}

void PaintContext::fill_circle(Point center, f32 radius, const Color& color) const {
    if (color.is_transparent()) return;
    const Point c{center.x + offset_x_, center.y + offset_y_};
    const RectF bounds = RectF::make(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
    track(bounds);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::FillCircle;
    cmd.rect = bounds;  // bounding box required: replay culls against it
    cmd.p1 = c;
    cmd.f1 = radius;
    cmd.color_a = color;
    push_command(std::move(cmd));
}

void PaintContext::fill_gradient(const RectF& rect, const Color& color_a, const Color& color_b,
                                 bool vertical, f32 radius) const {
    if (color_a.is_transparent() && color_b.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::FillGradient;
    cmd.rect = w;
    cmd.color_a = color_a;
    cmd.color_b = color_b;
    cmd.b1 = vertical;
    cmd.f1 = radius;
    push_command(std::move(cmd));
}

void PaintContext::draw_shadow(const RectF& rect, f32 radius, f32 blur, const Color& color) const {
    if (color.is_transparent() || blur <= 0.0f) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    // Shadow extent = target rect + 2.5*blur padding (matches backend texture generation)
    const RectF ext = w.inflated(blur * kShadowPadFactor, blur * kShadowPadFactor);
    track(ext);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawShadow;
    cmd.rect = w;  // base rect; the backend re-inflates by 2.5*blur at replay — culling uses the padded rect
    cmd.f1 = radius;
    cmd.f2 = blur;
    cmd.color_a = color;
    push_command(std::move(cmd));
}

Size PaintContext::bitmap_size(BitmapId id) const {
    return backend_.bitmap_size(id);
}

void PaintContext::draw_bitmap(BitmapId id, const RectF& rect, f32 radius) const {
    if (id == kInvalidBitmap) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawBitmap;
    cmd.rect = w;
    cmd.bitmap = id;
    cmd.f1 = radius;
    push_command(std::move(cmd));
}

void PaintContext::fill_radial_gradient(const Point& center, f32 radius, const Color& color_a,
                                        const Color& color_b) const {
    if (color_a.is_transparent() && color_b.is_transparent()) return;
    const Point c{center.x + offset_x_, center.y + offset_y_};
    const RectF bounds = RectF::make(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
    track(bounds);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::FillRadialGradient;
    cmd.rect = bounds;  // bounding box required: replay culls against it
    cmd.p1 = c;
    cmd.f1 = radius;
    cmd.color_a = color_a;
    cmd.color_b = color_b;
    push_command(std::move(cmd));
}

void PaintContext::fill_sweep_gradient(const RectF& rect, const Point& center, f32 start_angle,
                                       f32 sweep_angle, const Color& color_a, const Color& color_b,
                                       f32 radius) const {
    if (color_a.is_transparent() && color_b.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::FillSweepGradient;
    cmd.rect = w;
    cmd.p1 = Point{center.x + offset_x_, center.y + offset_y_};
    cmd.f1 = start_angle;
    cmd.f2 = sweep_angle;
    cmd.f3 = radius;
    cmd.color_a = color_a;
    cmd.color_b = color_b;
    push_command(std::move(cmd));
}

bool PaintContext::draw_backdrop_blur(const RectF& rect, f32 blur, const Color& tint,
                                      f32 radius) const {
    if (blur <= 0.0f) return false;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::BackdropBlur;
    cmd.rect = w;
    cmd.f1 = blur;
    cmd.f2 = radius;
    cmd.color_a = tint;
    push_command(std::move(cmd));
    return true;
}

void PaintContext::draw_border(const RectF& rect, const Color& color, f32 width, f32 radius) const {
    if (color.is_transparent() || width <= 0.0f) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawBorder;
    cmd.rect = w;
    cmd.color_a = color;
    cmd.f1 = width;
    cmd.f2 = radius;
    push_command(std::move(cmd));
}

void PaintContext::draw_line(Point a, Point b, const Color& color, f32 width) const {
    if (color.is_transparent()) return;
    const Point wa{a.x + offset_x_, a.y + offset_y_};
    const Point wb{b.x + offset_x_, b.y + offset_y_};
    const f32 half = width * 0.5f;
    const RectF bounds = RectF::make(std::min(wa.x, wb.x) - half, std::min(wa.y, wb.y) - half,
                                     std::abs(wa.x - wb.x) + width, std::abs(wa.y - wb.y) + width);
    track(bounds);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawLine;
    cmd.rect = bounds;  // bounding box required: replay culls against it
    cmd.p1 = wa;
    cmd.p2 = wb;
    cmd.color_a = color;
    cmd.f1 = width;
    push_command(std::move(cmd));
}

void PaintContext::draw_text(const String& text, const RectF& rect, const Color& color,
                             TextAlignH align_h, TextAlignV align_v) const {
    if (color.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawText;
    cmd.rect = w;
    cmd.font = font_;
    cmd.color_a = color;
    cmd.align_h = align_h;
    cmd.align_v = align_v;
    cmd.text = text;
    push_command(std::move(cmd));
}

void PaintContext::draw_text_small(const String& text, const RectF& rect, const Color& color,
                                   TextAlignH align_h, TextAlignV align_v) const {
    if (color.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawText;
    cmd.rect = w;
    cmd.font = font_small_;
    cmd.color_a = color;
    cmd.align_h = align_h;
    cmd.align_v = align_v;
    cmd.text = text;
    push_command(std::move(cmd));
}

void PaintContext::draw_text_title(const String& text, const RectF& rect, const Color& color,
                                   TextAlignH align_h, TextAlignV align_v) const {
    if (color.is_transparent()) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawText;
    cmd.rect = w;
    cmd.font = font_title_;
    cmd.color_a = color;
    cmd.align_h = align_h;
    cmd.align_v = align_v;
    cmd.text = text;
    push_command(std::move(cmd));
}

FontId PaintContext::font(const String& family, f32 size, u16 weight, bool italic) const {
    FontSpec spec;
    spec.family = family;
    spec.size = size;
    spec.weight = weight;
    spec.italic = italic;
    const auto it = fonts_.find(spec);
    if (it != fonts_.end()) return it->second;
    const FontId id = backend_.create_font(spec);
    fonts_.emplace(spec, id);
    return id;
}

void PaintContext::draw_text(FontId font, const String& text, const RectF& rect, const Color& color,
                             TextAlignH align_h, TextAlignV align_v) const {
    if (color.is_transparent() || font == kInvalidFont) return;
    const RectF w = rect.translated(offset_x_, offset_y_);
    track(w);
    PaintCommand cmd;
    cmd.type = PaintCommand::Type::DrawText;
    cmd.rect = w;
    cmd.font = font;
    cmd.color_a = color;
    cmd.align_h = align_h;
    cmd.align_v = align_v;
    cmd.text = text;
    push_command(std::move(cmd));
}

Size PaintContext::measure_text(FontId font, const String& text) const {
    return backend_.measure_text(font, text, 1e7f);
}

Size PaintContext::measure_text(const String& text) const {
    return backend_.measure_text(font_, text, 1e7f);
}

Size PaintContext::measure_text(const String& text, bool small) const {
    return backend_.measure_text(small ? font_small_ : font_, text, 1e7f);
}

Size PaintContext::measure_text(const String& text, bool small, f32 max_width) const {
    return backend_.measure_text(small ? font_small_ : font_, text, max_width);
}

}  // namespace yzk