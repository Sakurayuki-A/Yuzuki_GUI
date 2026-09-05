#include <yuzuki/controls/label.hpp>
#include <yuzuki/core/encoding.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/window.hpp>

namespace yzk {

namespace {
f32 estimate_text_width(const String& text, f32 font_size) {
    return static_cast<f32>(utf::to_wide(text).size()) * font_size * 0.55f;
}

f32 base_font_size(const Label& label, const Theme& theme) {
    return label.small() ? theme.type_caption : theme.font_size;
}
}  // namespace

Label::Label(String text) : text_(std::move(text)) {}

Label& Label::set_text(const String& text) {
    if (text_ == text) return *this;
    text_ = text;
    geometry_valid_ = false;
    if (rich_) parse_rich();
    invalidate();
    return *this;
}

Label& Label::set_text_color(const Color& color) {
    if (text_color_ == color) return *this;
    text_color_ = color;
    invalidate();
    return *this;
}

Label& Label::set_text_role(TextRole role) {
    if (text_role_ == role) return *this;
    text_role_ = role;
    invalidate();
    return *this;
}

Label& Label::set_small(bool small) {
    if (small_ == small) return *this;
    small_ = small;
    geometry_valid_ = false;
    invalidate();
    return *this;
}

Label& Label::set_bold(bool bold) {
    if (bold_ == bold) return *this;
    bold_ = bold;
    invalidate();
    return *this;
}

Label& Label::set_align(TextAlignH align_h, TextAlignV align_v) {
    align_h_ = align_h;
    align_v_ = align_v;
    geometry_valid_ = false;
    invalidate();
    return *this;
}

// ===== RichText lite (5.3.2) =====

Label& Label::set_rich_text(bool rich) {
    if (rich_ == rich) return *this;
    rich_ = rich;
    geometry_valid_ = false;
    if (rich_) {
        parse_rich();
    } else {
        runs_.clear();
    }
    invalidate();
    return *this;
}

Label& Label::set_on_span_click(std::function<void(const String& action)> cb) {
    on_span_click_cb_ = std::move(cb);
    return *this;
}

// Split `source` into visible runs. Markup (non-nested, no escaping):
//   **bold**, ~highlight~, [label](action).
void Label::parse_rich() {
    runs_.clear();
    const String& src = text_;
    const size_t n = src.size();
    size_t plain_begin = 0;  // start of the current unstyled run (source bytes)

    auto flush_plain = [&](size_t end_byte) {
        if (end_byte > plain_begin) {
            String seg = src.substr(plain_begin, end_byte - plain_begin);
            // A run spanning lines keeps its explicit newlines for multi-line draw.
            runs_.push_back(Run{seg, 400, false, false, String()});
        }
    };

    size_t i = 0;
    while (i + 1 < n) {
        const bool bold_open = src[i] == '*' && src[i + 1] == '*';
        const bool high_open = src[i] == '~';
        bool link_open = false;
        if (src[i] == '[') {
            // Cheap lookahead for ][...](...) so '[' inside plain text stays literal.
            const size_t close = src.find(']', i);
            if (close != String::npos && close + 1 < n && src[close + 1] == '(') {
                const size_t paren = src.find(')', close + 1);
                link_open = paren != String::npos;
            }
        }

        if (!bold_open && !high_open && !link_open) {
            ++i;
            continue;
        }

        if (bold_open) {
            const size_t close = src.find("**", i + 2);
            const size_t end = close == String::npos ? n : close;
            flush_plain(i);
            runs_.push_back(Run{src.substr(i + 2, end - i - 2), 700, false, false, String()});
            i = close == String::npos ? n : close + 2;
            plain_begin = i;
            continue;
        }
        if (high_open) {
            const size_t close = src.find('~', i + 1);
            const size_t end = close == String::npos ? n : close;
            flush_plain(i);
            runs_.push_back(Run{src.substr(i + 1, end - i - 1), 400, false, true, String()});
            i = close == String::npos ? n : close + 1;
            plain_begin = i;
            continue;
        }
        // link_open
        const size_t close = src.find(']', i);
        const size_t open_paren = close + 1;
        const size_t paren = src.find(')', close + 1);
        const String label = src.substr(i + 1, close - i - 1);
        const String action = src.substr(open_paren + 1, paren - open_paren - 1);
        flush_plain(i);
        runs_.push_back(Run{label, 400, true, false, action});
        i = paren + 1;
        plain_begin = i;
    }
    flush_plain(n);
}

void Label::relayout_runs(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 size = base_font_size(*this, theme);
    const FontId regular = ctx.font(theme.font_family, size, 400);
    const FontId bold = ctx.font(theme.font_family, size, 700);

    // Single horizontal pass (Label is not a wrapping control even in rich mode).
    f32 const_w = ctx.measure_text(regular, "W").width * 0.5f;
    (void)const_w;  // reserved for a small inter-font gap; measured widths suffice
    f32 x = 0.0f;
    f32 line_height = 0.0f;
    for (Run& r : runs_) {
        const Size m = ctx.measure_text(r.weight == 700 ? bold : regular, r.str);
        // Inter-run gap prevents adjacent glyph edges touching between font switches.
        const f32 gap = r.weight == 700 ? 0.0f : 0.0f;
        (void)gap;
        r.width = m.width;
        r.x = x;
        x += r.width;
        line_height = std::max(line_height, m.height);
    }
    const f32 w = x;
    const f32 h = line_height;

    // Apply alignment offsets.
    f32 ox = 0.0f, oy = 0.0f;
    if (align_h_ == TextAlignH::Center) ox = (bounds_.width() - w) / 2.0f;
    else if (align_h_ == TextAlignH::Right) ox = bounds_.width() - w;
    if (align_v_ == TextAlignV::Center) oy = (bounds_.height() - h) / 2.0f;
    else if (align_v_ == TextAlignV::Bottom) oy = bounds_.height() - h;
    for (Run& r : runs_) {
        r.x += ox;
        r.width = r.width;
        r.top = oy;
        r.height = h;
    }
    geometry_valid_ = true;
}

u32 Label::run_at(f32 local_x, f32 local_y) const {
    for (const Run& r : runs_) {
        if (local_x >= r.x && local_x <= r.x + r.width && local_y >= r.top &&
            local_y <= r.top + r.height) {
            const u32 idx = static_cast<u32>(&r - runs_.data());
            if (runs_[idx].link) return idx;
        }
    }
    return static_cast<u32>(runs_.size());
}

bool Label::hit_link(f32 x, f32 y) {
    if (!rich_ || !on_span_click_cb_) return false;
    if (!geometry_valid_) return false;
    const u32 idx = run_at(x, y);
    if (idx >= runs_.size()) return false;
    on_span_click_cb_(runs_[idx].action);
    return true;
}

void Label::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseMove: {
            if (!rich_) break;
            const RectF g = global_bounds();
            const u32 idx = geometry_valid_
                                ? run_at(e.data.mouse.x - g.left, e.data.mouse.y - g.top)
                                : static_cast<u32>(runs_.size());
            const bool over_link = idx < runs_.size();
            if (over_link != hovered_link_) {
                hovered_link_ = over_link;
                set_cursor(over_link ? Cursor::Hand : Cursor::Arrow);
            }
            break;
        }
        case EventType::MouseDown: {
            if (!rich_ || !on_span_click_cb_) break;
            if ((e.data.mouse.buttons & MouseButton_Left) == 0) break;
            const RectF g = global_bounds();
            if (hit_link(e.data.mouse.x - g.left, e.data.mouse.y - g.top)) {
                e.consumed = true;
            }
            break;
        }
        case EventType::MouseLeave:
            if (hovered_link_) {
                hovered_link_ = false;
                set_cursor(Cursor::Arrow);
            }
            break;
        default:
            break;
    }
    Widget::on_event(e);
}

Size Label::measure_impl(Size available, const PaintContext* ctx) {
    if (text_.empty()) return Size{2.0f, 4.0f};
    if (rich_) {
        if (ctx) {
            relayout_runs(const_cast<PaintContext&>(*ctx));
        }
        f32 w = 0.0f;
        f32 h = 0.0f;
        const Theme& theme = Theme::get();
        const f32 size = base_font_size(*this, theme);
        for (const Run& r : runs_) {
            if (ctx) {
                const FontId f = ctx->font(theme.font_family, size, r.weight);
                const Size m = ctx->measure_text(f, r.str);
                w += m.width;
                h = std::max(h, m.height);
            } else {
                w += estimate_text_width(r.str, size);
                h = std::max(h, size * 1.4f);
            }
        }
        return Size{w + 2.0f, h};
    }
    if (ctx) {
        // Label is single-line: measure with the same wrap=false contract as paint
        // (explicit '\n' still yields multiple lines; auto word-wrapping never applies),
        // so the laid-out box matches the glyphs actually drawn.
        (void)available;
        const Size measured = ctx->measure_text(text_, small_, 1e7f, false);
        return Size{measured.width + 2.0f, measured.height};
    }
    const f32 font_size = small_ ? Theme::get().type_caption : Theme::get().type_body;
    return Size{estimate_text_width(text_, font_size) + 2.0f, font_size * 1.4f};
}

void Label::paint_impl(PaintContext& ctx) {
    Color color = text_color_;
    if (color.is_transparent()) {
        const Theme& theme = ctx.theme();
        color = text_role_ == TextRole::Secondary ? theme.text_secondary
              : text_role_ == TextRole::Disabled ? theme.text_disabled
                                                 : theme.text;
    }

    if (rich_) {
        relayout_runs(ctx);
        const Theme& theme = ctx.theme();
        const f32 size = base_font_size(*this, theme);
        const Color accent = theme.accent;
        for (const Run& r : runs_) {
            if (r.str.empty()) continue;
            const FontId f = ctx.font(theme.font_family, size, r.weight);
            const bool link = r.link;
            const Color c = link || r.highlight ? accent : color;
            const RectF rect = RectF::make(bounds_.left + r.x, bounds_.top + r.top, r.width, r.height);
            ctx.draw_text(f, r.str, rect, c, TextAlignH::Left, TextAlignV::Top, false);
            if (link) {
                // Link underline aligned at the text baseline region's bottom edge.
                ctx.draw_line(Point{rect.left, rect.top + rect.height() - 1.0f},
                              Point{rect.right, rect.top + rect.height() - 1.0f}, c, 1.0f);
            }
        }
        return;
    }

    if (small_) {
        ctx.draw_text_small(text_, bounds_, color, align_h_, align_v_);
    } else {
        ctx.draw_text(text_, bounds_, color, align_h_, align_v_);
    }
}

}  // namespace yzk