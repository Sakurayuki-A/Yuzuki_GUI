#include <yuzuki/controls/text_box.hpp>
#include <yuzuki/core/encoding.hpp>
#include <yuzuki/ui/window.hpp>

#include <windows.h>

namespace yzk {

namespace {
Point to_local(Widget* widget, f32 x, f32 y) {
    const RectF g = widget->global_bounds();
    return Point{x - g.left, y - g.top};
}

FontId text_box_font(Window* win) {
    FontSpec spec;
    spec.family = Theme::get().font_family;
    spec.size = Theme::get().font_size;
    return win->backend().create_font(spec);
}

constexpr f32 kPadding = 6.0f;
constexpr f32 kLineHeight = 20.0f;

bool clipboard_set_text(const WString& text) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    bool ok = false;
    if (mem) {
        void* data = GlobalLock(mem);
        if (data) {
            memcpy(data, text.c_str(), bytes);
            GlobalUnlock(mem);
            ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
        } else {
            GlobalFree(mem);
        }
    }
    CloseClipboard();
    return ok;
}

WString clipboard_get_text() {
    WString result;
    if (!OpenClipboard(nullptr)) return result;
    HANDLE mem = GetClipboardData(CF_UNICODETEXT);
    if (mem) {
        const wchar_t* data = static_cast<const wchar_t*>(GlobalLock(mem));
        if (data) {
            result = data;
            GlobalUnlock(mem);
        }
    }
    CloseClipboard();
    return result;
}
}  // namespace

TextBox::TextBox(String text, TextBoxConfig config) : config_(config) {
    text_ = utf::to_wide(text);
    cursor_ = static_cast<u32>(text_.size());
    sel_start_ = cursor_;
    set_focusable(true);
    set_cursor(Cursor::IBeam);
}

void TextBox::set_config(const TextBoxConfig& config) {
    config_ = config;
    invalidate();
}

void TextBox::set_read_only(bool read_only) {
    if (config_.read_only == read_only) return;
    config_.read_only = read_only;
    invalidate();
}

void TextBox::set_text(const String& text) {
    text_ = utf::to_wide(text);
    if (cursor_ > text_.size()) cursor_ = static_cast<u32>(text_.size());
    if (sel_start_ > text_.size()) sel_start_ = static_cast<u32>(text_.size());
    invalidate();
}

Size TextBox::measure_impl(Size available, const PaintContext* ctx) {
    if (config_.mode == TextBoxMode::MultiLine) {
        f32 height = kLineHeight;
        if (ctx && !text_.empty()) {
            const Size m = ctx->measure_text(utf::to_utf8(text_), false, available.width);
            height = m.height > kLineHeight ? m.height : kLineHeight;
        }
        const f32 line_h = kLineHeight;
        u32 lines = static_cast<u32>(height / line_h + 0.5f);
        if (lines < config_.min_lines) lines = config_.min_lines;
        if (lines > config_.max_lines) lines = config_.max_lines;
        return Size{160.0f, static_cast<f32>(lines) * line_h + 8.0f};
    }
    (void)available;
    (void)ctx;
    return Size{160.0f, config_.height};
}

WString TextBox::display_text() const {
    if (config_.mode != TextBoxMode::Password) return text_;
    WString masked;
    masked.reserve(text_.size());
    for (const wchar_t c : text_) {
        masked.push_back(c == L'\n' ? c : L'\u2022');
    }
    return masked;
}

f32 TextBox::line_height() const {
    return kLineHeight;
}

u32 TextBox::line_index_at(u32 pos) const {
    u32 line = 0;
    const u32 end = pos < static_cast<u32>(text_.size()) ? pos : static_cast<u32>(text_.size());
    for (u32 i = 0; i < end; ++i) {
        if (text_[i] == L'\n') ++line;
    }
    return line;
}

u32 TextBox::line_start(u32 line) const {
    u32 pos = 0;
    u32 current = 0;
    while (current < line && pos < text_.size()) {
        if (text_[pos] == L'\n') ++current;
        ++pos;
    }
    return pos;
}

u32 TextBox::line_end(u32 line) const {
    u32 pos = line_start(line);
    while (pos < text_.size() && text_[pos] != L'\n') ++pos;
    return pos;
}

void TextBox::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const RectF& b = bounds_;

    ctx.fill_rounded(b, theme.surface, theme.corner_radius);
    ctx.draw_border(b, focused_ ? theme.accent : theme.border, 1.0f, theme.corner_radius);

    const bool password = config_.mode == TextBoxMode::Password;

    const WString shown = display_text();
    // Live IME pre-edit is displayed inserted at the caret (password never composes).
    const bool composing = !composition_.empty();
    const u32 comp_caret = cursor_ + composition_cursor_;
    WString visible = shown;
    if (composing) visible.insert(cursor_, composition_);
    const bool empty = visible.empty();
    const WString prefix = empty ? WString() : visible.substr(0, comp_caret);

    const RectF text_rect = RectF::make(b.left + kPadding, b.top, b.width() - kPadding * 2.0f - content_inset_, b.height());
    const Color color = empty ? theme.text_disabled : theme.text;

    if (config_.mode == TextBoxMode::MultiLine) {
        scroll_offset_ = 0.0f;
    } else {
        const f32 prefix_w = prefix.empty() ? 0.0f : ctx.measure_text(utf::to_utf8(prefix)).width;
        const f32 avail = text_rect.width() - 4.0f;
        const f32 target = prefix_w - avail;
        scroll_offset_ = target > 0.0f ? target : 0.0f;
    }
    const f32 ox = -scroll_offset_;

    ctx.push_clip(text_rect);
    const bool multiline = config_.mode == TextBoxMode::MultiLine;
    const f32 draw_w = multiline ? text_rect.width() : 1e7f;
    const RectF draw_rect = RectF::make(text_rect.left + ox,
                                        multiline ? b.top + kPadding : text_rect.top,
                                        draw_w, text_rect.height());
    if (password && !empty) {
        const WString masked = shown;
        const f32 cy = text_rect.top + text_rect.height() * 0.5f;
        for (u32 i = 0; i < masked.size(); ++i) {
            const f32 w = ctx.measure_text(utf::to_utf8(masked.substr(i, 1))).width;
            const f32 cx = text_rect.left + ox +
                           ctx.measure_text(utf::to_utf8(masked.substr(0, i))).width + w * 0.5f;
            ctx.fill_circle(Point{cx, cy}, w * 0.42f, color);
        }
    } else {
        ctx.draw_text(empty ? placeholder_ : utf::to_utf8(visible), draw_rect, color,
                      TextAlignH::Left, multiline ? TextAlignV::Top : TextAlignV::Center);
    }

    // IME pre-edit underline spanning the composition span.
    if (composing) {
        if (Window* win = window()) {
            const FontId font = text_box_font(win);
            const Point p0 = win->backend().caret_position(
                font, utf::to_utf8(visible), draw_w, static_cast<i32>(cursor_));
            const Point p1 = win->backend().caret_position(
                font, utf::to_utf8(visible), draw_w,
                static_cast<i32>(cursor_ + static_cast<u32>(composition_.size())));
            const f32 uy = multiline ? b.top + kPadding + p0.y + line_height() - 2.0f
                                     : b.top + (b.height() - line_height()) / 2.0f +
                                           line_height() - 2.0f;
            f32 ux0 = text_rect.left + ox + p0.x;
            f32 ux1 = text_rect.left + ox + p1.x;
            if (ux1 < ux0) ux1 = ux0 + 1.0f;
            ctx.fill_rect(RectF::make(ux0, uy, ux1 - ux0, 2.0f), theme.accent);
        }
    }

    const u32 sel_begin = selection_begin();
    const u32 sel_end = selection_end();
    if (focused_ && sel_begin != sel_end && !password) {
        if (Window* win = window()) {
            const f32 lay_h = multiline ? 1e7f : text_rect.height();
            const std::vector<TextSelectionRect> rects = win->backend().text_selection_rects(
                text_box_font(win), utf::to_utf8(shown), draw_w, lay_h,
                static_cast<i32>(sel_begin), static_cast<i32>(sel_end));
            for (const TextSelectionRect& s : rects) {
                const f32 r_top = multiline
                                      ? b.top + kPadding + s.rect.top
                                      : text_rect.top + (text_rect.height() - s.rect.height()) / 2.0f +
                                            s.rect.top;
                const RectF r = RectF::make(text_rect.left + ox + s.rect.left, r_top,
                                            s.rect.width(), s.rect.height());
                ctx.fill_rect(r, theme.selection_bg);
            }
        }
    }

    if (focused_ && caret_visible_) {
        f32 caret_x = 0.0f;
        f32 caret_y = 0.0f;
        if (multiline) {
            if (Window* win = window()) {
                const Point cp = win->backend().caret_position(
                    text_box_font(win), utf::to_utf8(visible), text_rect.width(),
                    static_cast<i32>(comp_caret));
                caret_x = text_rect.left + ox + cp.x;
                caret_y = b.top + kPadding + cp.y;
            }
        } else {
            const u32 start = line_start(line_index_at(cursor_));
            const WString line_prefix = visible.substr(start, comp_caret - start);
            caret_x = text_rect.left + ox +
                      (line_prefix.empty() ? 0.0f : ctx.measure_text(utf::to_utf8(line_prefix)).width);
            caret_y = b.top + (b.height() - line_height()) / 2.0f;
        }
        const f32 caret_xr = static_cast<f32>(static_cast<i32>(caret_x + 0.5f));
        ctx.fill_rect(RectF::make(caret_xr, caret_y + 3.0f, 2.0f, line_height() - 6.0f), theme.accent);
    }
    ctx.pop_clip();
}

void TextBox::on_event(Event& e) {
    switch (e.type) {
        case EventType::FocusGained:
            focused_ = true;
            caret_visible_ = true;
            if (Window* win = window()) win->start_timer(this, 500);
            invalidate();
            break;

        case EventType::FocusLost:
            focused_ = false;
            composition_.clear();
            composition_cursor_ = 0;
            if (Window* win = window()) win->stop_timer(this);
            invalidate();
            break;

        case EventType::Timer:
            caret_visible_ = !caret_visible_;
            break;

        case EventType::MouseDown:
            if (e.data.mouse.buttons & MouseButton_Left) {
                if (Window* win = window()) win->set_focus(this);
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                sel_start_ = 0;
                cursor_ = 0;
                set_cursor_by_pos(p.x, p.y);
                sel_start_ = cursor_;
                selecting_ = true;
                e.consumed = true;
            }
            break;

        case EventType::MouseMove:
            if (selecting_ && (e.data.mouse.buttons & MouseButton_Left)) {
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                set_cursor_by_pos(p.x, p.y);
                e.consumed = true;
            }
            break;

        case EventType::MouseUp:
            if (selecting_) {
                selecting_ = false;
                e.consumed = true;
            }
            break;

        case EventType::Character:
            if (!focused_ || config_.read_only) break;
            // While an IME composition is live the IME owns the text stream; raw
            // characters would double-insert.
            if (!composition_.empty()) {
                e.consumed = true;
                break;
            }
            // Enter is handled in KeyDown (VK_RETURN) which already inserts the
            // newline; TranslateMessage delivers WM_CHAR '\r' for the same press —
            // ignoring it here prevents double newlines.
            if (e.data.key.chr == L'\r') {
                e.consumed = true;
                break;
            }
            if (e.data.key.chr == '\n') {
                if (config_.mode == TextBoxMode::MultiLine) {
                    if (sel_start_ != cursor_) delete_selection();
                    if (text_.size() < config_.max_length) {
                        text_.insert(cursor_, 1, L'\n');
                        ++cursor_;
                        sel_start_ = cursor_;
                        invalidate();
                    }
                }
            } else if (e.data.key.chr >= 32 && e.data.key.chr != 127) {
                if (sel_start_ != cursor_) delete_selection();
                if (text_.size() < config_.max_length) {
                    text_.insert(cursor_, 1, static_cast<wchar_t>(e.data.key.chr));
                    ++cursor_;
                    sel_start_ = cursor_;
                    invalidate();
                }
            }
            e.consumed = true;
            break;

        case EventType::ImeCompose:
            e.consumed = true;
            if (!focused_) break;
            {
                const WString incoming(e.data.ime.text ? e.data.ime.text : L"",
                                       e.data.ime.length);
                if (incoming.empty()) {
                    // Cancelled / fully converted: clear the pre-edit display.
                    composition_.clear();
                    composition_cursor_ = 0;
                    invalidate();
                    break;
                }
                if (config_.read_only) break;
                // A new composition replaces the current selection when committed.
                if (composition_.empty() && sel_start_ != cursor_) delete_selection();
                composition_ = incoming;
                composition_cursor_ = e.data.ime.cursor > composition_.size()
                                          ? static_cast<u32>(composition_.size())
                                          : e.data.ime.cursor;
                invalidate();
                if (Window* win = window()) win->refresh_ime_anchor();
            }
            break;

        case EventType::ImeCommit:
            e.consumed = true;
            composition_.clear();
            composition_cursor_ = 0;
            if (!focused_ || config_.read_only) break;
            insert_text(WString(e.data.ime.text ? e.data.ime.text : L"", e.data.ime.length));
            invalidate();
            break;

        case EventType::KeyDown: {
            if (!focused_) break;
            // While composing, the IME owns the keyboard: navigation/selection keys
            // would desync the visual caret from the pre-edit insertion point.
            if (!composition_.empty()) {
                e.consumed = true;
                break;
            }
            caret_visible_ = true;
            const bool ctrl = (e.data.key.mods & KeyModifier_Control) != 0;
            const bool shift = (e.data.key.mods & KeyModifier_Shift) != 0;
            const bool has_sel = sel_start_ != cursor_;

            if (ctrl) {
                switch (e.data.key.code) {
                    case 'A':
                        sel_start_ = 0;
                        cursor_ = static_cast<u32>(text_.size());
                        invalidate();
                        break;
                    case 'C':
                        copy_selection();
                        break;
                    case 'V':
                        if (!config_.read_only) paste_from_clipboard();
                        break;
                    case 'X':
                        if (!config_.read_only) cut_selection();
                        break;
                    default:
                        break;
                }
                e.consumed = true;
                break;
            }

            switch (e.data.key.code) {
                case VK_RETURN:
                    if (config_.mode == TextBoxMode::MultiLine && !config_.read_only) {
                        if (sel_start_ != cursor_) delete_selection();
                        if (text_.size() < config_.max_length) {
                            text_.insert(cursor_, 1, L'\n');
                            ++cursor_;
                            sel_start_ = cursor_;
                            invalidate();
                        }
                    }
                    e.consumed = true;
                    break;
                case VK_BACK:
                    if (config_.read_only) break;
                    if (has_sel) {
                        delete_selection();
                    } else {
                        delete_backward();
                    }
                    e.consumed = true;
                    break;
                case VK_DELETE:
                    if (config_.read_only) break;
                    if (has_sel) {
                        delete_selection();
                    } else if (cursor_ < text_.size()) {
                        u32 n = 1;
                        if (cursor_ + 1 < text_.size()) {
                            const wchar_t c = text_[cursor_];
                            const wchar_t next = text_[cursor_ + 1];
                            if (c >= 0xD800 && c <= 0xDBFF && next >= 0xDC00 && next <= 0xDFFF) n = 2;
                        }
                        text_.erase(cursor_, n);
                        sel_start_ = cursor_;
                        invalidate();
                    }
                    e.consumed = true;
                    break;
                case VK_LEFT:
                    if (shift && !has_sel) sel_start_ = cursor_;
                    move_cursor(-1);
                    if (!shift) sel_start_ = cursor_;
                    e.consumed = true;
                    break;
                case VK_RIGHT:
                    if (shift && !has_sel) sel_start_ = cursor_;
                    move_cursor(1);
                    if (!shift) sel_start_ = cursor_;
                    e.consumed = true;
                    break;
                case VK_UP:
                    if (config_.mode == TextBoxMode::MultiLine) {
                        if (shift && !has_sel) sel_start_ = cursor_;
                        move_cursor_vertical(-1);
                        if (!shift) sel_start_ = cursor_;
                    }
                    e.consumed = true;
                    break;
                case VK_DOWN:
                    if (config_.mode == TextBoxMode::MultiLine) {
                        if (shift && !has_sel) sel_start_ = cursor_;
                        move_cursor_vertical(1);
                        if (!shift) sel_start_ = cursor_;
                    }
                    e.consumed = true;
                    break;
                case VK_HOME:
                    if (shift && !has_sel) sel_start_ = cursor_;
                    if (config_.mode == TextBoxMode::MultiLine) {
                        cursor_ = line_start(line_index_at(cursor_));
                    } else {
                        cursor_ = 0;
                    }
                    if (!shift) sel_start_ = cursor_;
                    invalidate();
                    e.consumed = true;
                    break;
                case VK_END:
                    if (shift && !has_sel) sel_start_ = cursor_;
                    if (config_.mode == TextBoxMode::MultiLine) {
                        cursor_ = line_end(line_index_at(cursor_));
                    } else {
                        cursor_ = static_cast<u32>(text_.size());
                    }
                    if (!shift) sel_start_ = cursor_;
                    invalidate();
                    e.consumed = true;
                    break;
                default:
                    break;
            }
            break;
        }

        case EventType::MouseEnter:
            invalidate();
            break;

        case EventType::MouseLeave:
            invalidate();
            break;

        default:
            break;
    }
}

void TextBox::move_cursor(i32 delta) {
    i32 target = static_cast<i32>(cursor_) + delta;
    if (target < 0) target = 0;
    if (target > static_cast<i32>(text_.size())) target = static_cast<i32>(text_.size());
    cursor_ = static_cast<u32>(target);
    invalidate();
}

void TextBox::move_cursor_vertical(i32 delta_line) {
    Window* win = window();
    if (!win) return;
    const FontId font = text_box_font(win);
    // Same wrap width as painting (text_rect minus inset), or hit-testing wraps differently.
    const f32 width = bounds_.width() - kPadding * 2.0f - content_inset_;
    const Point cp = win->backend().caret_position(font, utf::to_utf8(text_), width,
                                                   static_cast<i32>(cursor_));
    // Real line height from the loaded font: fixed kLineHeight drifts per line with CJK
    // fallback fonts (taller ascent/descent), so up/down skips or sticks after a few lines.
    const f32 measured_lh = win->backend().measure_text(font, "Wg", 1e7f).height;
    const f32 line_h = measured_lh > 1.0f ? measured_lh : line_height();
    f32 target_y = cp.y + static_cast<f32>(delta_line) * line_h;
    // Clamp into the wrapped content so moving past the first/last line sticks there
    // instead of landing on a clamped nearest cluster in an out-of-range row.
    const f32 content_h = win->backend().measure_text(font, utf::to_utf8(text_), width).height;
    if (target_y < 0.0f) target_y = 0.0f;
    if (content_h > 0.0f && target_y > content_h) target_y = content_h;
    i32 index = win->backend().hit_test_text(font, utf::to_utf8(text_), width, cp.x, target_y);
    if (index < 0) index = 0;
    if (index > static_cast<i32>(text_.size())) index = static_cast<i32>(text_.size());
    cursor_ = static_cast<u32>(index);
    invalidate();
}

void TextBox::set_cursor_by_pos(f32 x, f32 y) {
    if (config_.mode == TextBoxMode::MultiLine) {
        Window* win = window();
        if (!win) return;
        const FontId font = text_box_font(win);
        const f32 lx = x - kPadding;
        const f32 ly = y - kPadding;
        // Same wrap width as painting, so the clicked line breaks match what is drawn.
        i32 index = win->backend().hit_test_text(font, utf::to_utf8(text_),
                                                 bounds_.width() - kPadding * 2.0f -
                                                     content_inset_,
                                                 lx, ly);
        if (index < 0) index = 0;
        if (index > static_cast<i32>(text_.size())) index = static_cast<i32>(text_.size());
        cursor_ = static_cast<u32>(index);
        invalidate();
        return;
    }
    Window* win = window();
    if (!win) return;
    const FontId font = text_box_font(win);
    const WString shown = display_text();
    const f32 local_x = x - kPadding + scroll_offset_;
    i32 index = win->backend().hit_test_text(font, utf::to_utf8(shown), 1e7f, local_x, 0.0f);
    if (index < 0) index = 0;
    if (index > static_cast<i32>(shown.size())) index = static_cast<i32>(shown.size());
    cursor_ = static_cast<u32>(index);
    invalidate();
}

void TextBox::delete_backward() {
    if (cursor_ == 0) return;
    u32 n = 1;
    if (cursor_ >= 2) {
        const wchar_t c = text_[cursor_ - 1];
        const wchar_t prev = text_[cursor_ - 2];
        if (c >= 0xDC00 && c <= 0xDFFF && prev >= 0xD800 && prev <= 0xDBFF) n = 2;
    }
    cursor_ -= n;
    text_.erase(cursor_, n);
    sel_start_ = cursor_;
    invalidate();
}

void TextBox::insert_text(const WString& text) {
    if (sel_start_ != cursor_) delete_selection();
    for (const wchar_t c : text) {
        if (text_.size() >= config_.max_length) break;
        if (config_.mode != TextBoxMode::MultiLine && c == L'\n') continue;
        text_.insert(cursor_, 1, c);
        ++cursor_;
    }
    sel_start_ = cursor_;
}

RectF TextBox::ime_caret_rect() const {
    Window* win = window();
    if (!win || !focused_) return RectF{};
    const RectF g = global_bounds();
    const bool multiline = config_.mode == TextBoxMode::MultiLine;
    // The anchor sits at the in-composition caret: display string with the live
    // pre-edit inserted, caret index extended by the composition cursor offset.
    WString visible = display_text();
    if (!composition_.empty()) visible.insert(cursor_, composition_);
    const f32 lay_w =
        multiline ? g.width() - kPadding * 2.0f - content_inset_ : 1e7f;
    const FontId font = text_box_font(win);
    const Point cp = win->backend().caret_position(
        font, utf::to_utf8(visible), lay_w,
        static_cast<i32>(cursor_ + composition_cursor_));
    const f32 x = g.left + kPadding - scroll_offset_ + cp.x;
    const f32 y = multiline ? g.top + kPadding + cp.y
                            : g.top + (g.height() - line_height()) / 2.0f;
    return RectF::make(x, y, 2.0f, line_height());
}

void TextBox::delete_selection() {
    const u32 begin = selection_begin();
    const u32 end = selection_end();
    if (begin == end) return;
    text_.erase(begin, end - begin);
    cursor_ = begin;
    sel_start_ = cursor_;
    invalidate();
}

void TextBox::copy_selection() const {
    const u32 begin = selection_begin();
    const u32 end = selection_end();
    if (begin == end) return;
    clipboard_set_text(text_.substr(begin, end - begin));
}

void TextBox::cut_selection() {
    const u32 begin = selection_begin();
    const u32 end = selection_end();
    if (begin == end) return;
    clipboard_set_text(text_.substr(begin, end - begin));
    text_.erase(begin, end - begin);
    cursor_ = begin;
    sel_start_ = cursor_;
    invalidate();
}

void TextBox::paste_from_clipboard() {
    WString pasted = clipboard_get_text();
    if (pasted.empty()) return;
    if (sel_start_ != cursor_) delete_selection();
    const u32 room = static_cast<u32>(text_.size()) < config_.max_length ? config_.max_length - static_cast<u32>(text_.size()) : 0;
    if (pasted.size() > room) pasted.resize(room);
    text_.insert(cursor_, pasted);
    cursor_ += static_cast<u32>(pasted.size());
    sel_start_ = cursor_;
    invalidate();
}

}  // namespace yzk