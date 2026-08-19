#include "components.hpp"

#include <utility>

namespace yzk {

namespace {
Point to_local(Widget* widget, f32 x, f32 y) {
    const RectF g = widget->global_bounds();
    return Point{x - g.left, y - g.top};
}
}  // namespace

// ===================== Pane =====================

Pane::Pane(Color bg, f32 fixed_height) : bg_(bg), fixed_height_(fixed_height) {}

void Pane::set_content(Widget* child) {
    content_ = child;
    append_child(child);
}

void Pane::set_border_right(f32 w, Color c) {
    border_right_w_ = w;
    border_right_color_ = c;
}

Size Pane::measure_content(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, fixed_height_};
}

void Pane::arrange_content(const RectF& area, const PaintContext* ctx) {
    if (content_) {
        content_->set_bounds(area);
        content_->perform_layout(ctx);
    }
}

void Pane::paint_impl(PaintContext& ctx) {
    ctx.fill_rect(bounds_, bg_);
    if (border_right_w_ > 0.0f) {
        ctx.draw_line(Point{bounds_.right - border_right_w_ * 0.5f, bounds_.top},
                      Point{bounds_.right - border_right_w_ * 0.5f, bounds_.bottom},
                      border_right_color_, border_right_w_);
    }
    Widget::paint_impl(ctx);
}

// ===================== RoundButton =====================

RoundButton::RoundButton(String text, Color bg, Color bg_hover, Color text_color)
    : text_(std::move(text)), bg_(bg), bg_hover_(bg_hover), text_color_(text_color) {
    set_cursor(Cursor::Hand);
}

void RoundButton::set_icon(IconId id, f32 size) {
    icon_ = id;
    icon_size_ = size;
    invalidate();
}

Size RoundButton::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    const f32 icon_w = icon_ != IconId::None ? icon_size_ + 6.0f : 0.0f;
    const f32 w = ctx->measure_text(text_).width + icon_w + padding_x_ * 2.0f;
    return Size{w, height_};
}

void RoundButton::paint_impl(PaintContext& ctx) {
    Color fill = bg_;
    if (has_flag(Flag_Hovered)) fill = bg_hover_;
    if (has_flag(Flag_Pressed)) fill = bg_hover_;

    const f32 radius = height_ * 0.5f;
    ctx.fill_rounded(bounds_, fill, radius);
    const f32 icon_w = icon_ != IconId::None ? icon_size_ + 6.0f : 0.0f;
    const RectF text_rect = RectF::make(bounds_.left + padding_x_ + icon_w, bounds_.top,
                                        bounds_.width() - padding_x_ * 2.0f - icon_w,
                                        bounds_.height());
    ctx.draw_text(text_, text_rect, text_color_);
    if (icon_ != IconId::None) {
        const FontId icon_font = ctx.font(icon_family, icon_size_);
        ctx.draw_text(icon_font, icon_glyph(icon_),
                      RectF::make(bounds_.left + padding_x_, bounds_.top, icon_size_,
                                  bounds_.height()),
                      text_color_);
    }
}

void RoundButton::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseDown:
            if (e.data.mouse.buttons & MouseButton_Left) {
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                if (p.x >= 0.0f && p.y >= 0.0f && p.x <= bounds_.width() && p.y <= bounds_.height()) {
                    add_flag(Flag_Pressed);
                    invalidate();
                    e.consumed = true;
                }
            }
            break;

        case EventType::MouseUp:
            if (has_flag(Flag_Pressed)) {
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                const bool inside = p.x >= 0.0f && p.y >= 0.0f && p.x <= bounds_.width() && p.y <= bounds_.height();
                remove_flag(Flag_Pressed);
                invalidate();
                if (inside) on_click();
                e.consumed = true;
            }
            break;

        case EventType::Click:
            e.consumed = true;
            break;

        default:
            break;
    }
}

// ===================== SessionItem =====================

SessionItem::SessionItem(String title, String subtitle)
    : title_(std::move(title)), subtitle_(std::move(subtitle)) {
    set_cursor(Cursor::Hand);
}

Size SessionItem::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, 44.0f};
}

void SessionItem::set_selected(bool selected) {
    if (selected_ == selected) return;
    selected_ = selected;
    invalidate();
}

void SessionItem::paint_impl(PaintContext& ctx) {
    Color bg = codex::SidebarBg;
    if (selected_) {
        bg = codex::CardBg;
    } else if (has_flag(Flag_Hovered)) {
        bg = codex::HoverBg;
    }
    ctx.fill_rect(bounds_, bg);
    if (selected_) {
        ctx.fill_rect(RectF::make(bounds_.left, bounds_.top, 3.0f, bounds_.height()), codex::Accent);
    }

    const RectF title_rect = RectF::make(bounds_.left + 14.0f, bounds_.top, bounds_.width() - 28.0f, 24.0f);
    ctx.draw_text(title_, title_rect, codex::Text, TextAlignH::Left, TextAlignV::Center);
    const RectF sub_rect = RectF::make(bounds_.left + 14.0f, bounds_.top + 22.0f, bounds_.width() - 28.0f, 18.0f);
    ctx.draw_text_small(subtitle_, sub_rect, codex::TextSecondary);
}

void SessionItem::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseUp:
            if (e.data.mouse.buttons == 0 || (e.data.mouse.buttons & MouseButton_Left)) {
                selected_ = true;
                invalidate();
                on_select();
                e.consumed = true;
            }
            break;

        default:
            break;
    }
}

// ===================== ToolCard =====================

ToolCard::ToolCard(String kind, String command, String status)
    : kind_(std::move(kind)), command_(std::move(command)), status_(std::move(status)) {
    set_cursor(Cursor::Hand);
}

void ToolCard::wrap(const PaintContext* ctx, f32 max_width) {
    lines_.clear();
    if (max_width < 40.0f) max_width = 40.0f;
    String line;
    f32 line_w = 0.0f;
    String word;
    const char* p = command_.c_str();

    auto flush = [&]() {
        if (!line.empty()) lines_.push_back(line);
        line.clear();
        line_w = 0.0f;
    };
    auto push_word = [&]() {
        if (word.empty()) return;
        const f32 ww = ctx->measure_text(word).width;
        if (!line.empty() && line_w + 6.0f + ww > max_width) flush();
        if (!line.empty()) {
            line += "   ";
            line_w += 6.0f;
        }
        line += word;
        line_w += ww;
        word.clear();
    };

    while (*p) {
        if (*p == ' ' || *p == '\n') {
            push_word();
            if (*p == '\n') flush();
            ++p;
        } else {
            word += *p++;
        }
    }
    push_word();
    flush();
    if (lines_.empty()) lines_.push_back(String());
}

Size ToolCard::measure_impl(Size available, const PaintContext* ctx) {
    line_h_ = ctx->measure_text("Wg").height;
    f32 h = 34.0f;
    if (expanded_ || expand_progress_ > 0.0f) {
        const f32 inner_w = (available.width > 80.0f ? available.width : 80.0f) - 48.0f;
        wrap(ctx, inner_w);
        h += (8.0f + (f32)lines_.size() * line_h_ + 12.0f) * expand_progress_;
    }
    return Size{0.0f, h};
}

void ToolCard::paint_impl(PaintContext& ctx) {
    const f32 radius = 10.0f;
    const bool hovered = has_flag(Flag_Hovered);
    ctx.fill_rounded(bounds_, codex::CardBg, radius);
    ctx.draw_border(bounds_, hovered ? codex::Border : codex::CodeBorder, 1.0f, radius);

    // Header: status dot + kind + status text + expand arrow
    ctx.fill_circle(Point{bounds_.left + 16.0f, bounds_.top + 17.0f}, 4.0f, codex::Success);
    ctx.draw_text_small(kind_,
                        RectF::make(bounds_.left + 28.0f, bounds_.top, 160.0f, 34.0f),
                        codex::Text, TextAlignH::Left, TextAlignV::Center);

    const f32 sw = ctx.measure_text(status_, true).width;
    ctx.draw_text_small(status_,
                        RectF::make(bounds_.right - sw - 30.0f, bounds_.top, sw, 34.0f),
                        codex::TextSecondary, TextAlignH::Left, TextAlignV::Center);

    const FontId icon_font = ctx.font(icon_family, 12.0f);
    ctx.draw_text(icon_font,
                  icon_glyph(expanded_ ? IconId::CaretDown : IconId::CaretRight),
                  RectF::make(bounds_.right - 26.0f, bounds_.top, 18.0f, 34.0f),
                  codex::TextSecondary);

    if (expand_progress_ > 0.0f) {
        const f32 body_h = bounds_.height() - 34.0f;
        if (body_h > 4.0f) {
            const RectF body = RectF::make(bounds_.left + 12.0f, bounds_.top + 34.0f,
                                           bounds_.width() - 24.0f, body_h);
            ctx.fill_rounded(body, codex::CodeBg, 6.0f);
            ctx.draw_border(body, codex::CodeBorder, 1.0f, 6.0f);
            const f32 content_h = body.height() - 12.0f;
            const size_t visible = (size_t)(content_h / line_h_);
            for (size_t i = 0; i < visible && i < lines_.size(); ++i) {
                ctx.draw_text_small(lines_[i],
                                    RectF::make(body.left + 10.0f, body.top + 6.0f + (f32)i * line_h_,
                                                body.width() - 20.0f, line_h_),
                                    codex::Text);
            }
        }
    }
}

void ToolCard::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseUp:
            if (e.data.mouse.buttons == 0 || (e.data.mouse.buttons & MouseButton_Left)) {
                expanded_ = !expanded_;
                AnimationSystem& as = AnimationSystem::instance();
                as.tween(expand_progress_, expanded_ ? 1.0f : 0.0f, 180.0f,
                         expanded_ ? Easing::OutCubic : Easing::InCubic,
                         [this](f32 v) {
                             expand_progress_ = v;
                             invalidate();
                         });
                invalidate();
                e.consumed = true;
            }
            break;

        default:
            break;
    }
}

// ===================== MessageCard =====================

MessageCard::MessageCard(bool user, String text) : user_(user), text_(std::move(text)) {}

void MessageCard::wrap(const PaintContext* ctx, f32 max_width) {
    lines_.clear();
    line_w_ = 0.0f;
    if (max_width < 40.0f) max_width = 40.0f;
    String line;
    f32 line_w = 0.0f;
    String word;
    const char* p = text_.c_str();

    auto flush = [&]() {
        if (!line.empty()) {
            lines_.push_back(line);
            if (line_w > line_w_) line_w_ = line_w;
        }
        line.clear();
        line_w = 0.0f;
    };
    auto push_word = [&]() {
        if (word.empty()) return;
        const f32 ww = ctx->measure_text(word).width;
        if (!line.empty() && line_w + 4.0f + ww > max_width) flush();
        if (!line.empty()) {
            line += ' ';
            line_w += 4.0f;
        }
        line += word;
        line_w += ww;
        word.clear();
    };

    while (*p) {
        if (*p == ' ' || *p == '\n') {
            push_word();
            if (*p == '\n') flush();
            ++p;
        } else {
            word += *p++;
        }
    }
    push_word();
    flush();
    if (lines_.empty()) lines_.push_back(String());
}

Size MessageCard::measure_impl(Size available, const PaintContext* ctx) {
    line_h_ = ctx->measure_text("Wg").height;
    const f32 avail = available.width > 0.0f ? available.width : 600.0f;
    const f32 max_w = avail - 64.0f;
    wrap(ctx, max_w);
    return Size{0.0f, (f32)lines_.size() * line_h_ + 22.0f};
}

void MessageCard::paint_impl(PaintContext& ctx) {
    const f32 pad_x = 16.0f;
    const f32 pad_y = 11.0f;
    f32 x;
    if (user_) {
        const f32 bw = line_w_ + 28.0f;
        const RectF bubble = RectF::make(bounds_.right - pad_x - bw, bounds_.top + pad_y, bw,
                                         bounds_.height() - pad_y * 2.0f);
        ctx.fill_rounded(bubble, codex::UserBubbleBg, 12.0f);
        x = bubble.left + 14.0f;
    } else {
        const RectF card = RectF::make(bounds_.left + pad_x, bounds_.top + pad_y,
                                       bounds_.width() - pad_x * 2.0f, bounds_.height() - pad_y * 2.0f);
        ctx.fill_rounded(card, codex::WindowBg, 12.0f);
        ctx.draw_border(card, codex::CodeBorder, 1.0f, 12.0f);
        x = card.left + 14.0f;
    }
    const f32 text_w = line_w_ + 4.0f;
    for (size_t i = 0; i < lines_.size(); ++i) {
        ctx.draw_text(lines_[i],
                      RectF::make(x, bounds_.top + pad_y + (f32)i * line_h_, text_w, line_h_),
                      codex::Text, TextAlignH::Left, TextAlignV::Center);
    }
}

// ===================== Composer =====================

Composer::Composer() {
    TextBoxConfig cfg;
    cfg.mode = TextBoxMode::MultiLine;
    cfg.min_lines = 1;
    cfg.max_lines = 4;
    input_ = new TextBox(String(), cfg);
    input_->set_placeholder("Ask Codex to solve your issues...");
    input_->set_content_inset(10.0f);
    send_ = new RoundButton("Send", codex::ButtonBg, codex::ButtonBgHover, codex::ButtonText);
    send_->set_icon(IconId::PaperPlaneTilt, 15.0f);
    append_child(input_);
    append_child(send_);
}

Size Composer::measure_content(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, 110.0f};
}

void Composer::paint_impl(PaintContext& ctx) {
    ctx.draw_line(Point{bounds_.left, bounds_.top + 0.5f},
                  Point{bounds_.right, bounds_.top + 0.5f}, codex::Border, 1.0f);
    Widget::paint_impl(ctx);
}

void Composer::arrange_content(const RectF& area, const PaintContext* ctx) {
    const f32 inset = 14.0f;
    const f32 send_h = 32.0f;
    const RectF r = RectF::make(area.left + inset, area.top + inset,
                                area.width() - inset * 2.0f, area.height() - inset * 2.0f);
    const Size send_size = send_->measure(Size{1e7f, send_h}, ctx);
    const f32 send_w = send_size.width;
    send_->set_bounds(RectF::make(r.right - send_w, r.bottom - send_h, send_w, send_h));
    input_->set_bounds(RectF::make(r.left, r.top, r.width() - send_w - 10.0f, r.height()));
    input_->perform_layout(ctx);
    send_->perform_layout(ctx);
}

}  // namespace yzk