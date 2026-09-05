#pragma once
#include <yuzuki/core/encoding.hpp>
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>
#include <vector>

namespace yzk {

enum class TextBoxMode : u8 {
    SingleLine,
    Password,
    MultiLine,
};

struct TextBoxConfig {
    TextBoxMode mode = TextBoxMode::SingleLine;
    bool read_only = false;
    u32 max_length = 4096;
    // Fixed widget height. Multiline uses it as the viewport height (overflow
    // scrolls internally); max_lines only sizes the fallback when height is unset.
    f32 height = 32.0f;
    u32 min_lines = 1;  // reserved; no longer participates in measurement
    u32 max_lines = 6;
    // MultiLine only: Enter submits (callback), Ctrl+Enter inserts a newline.
    bool enter_submits = false;
    // Skip drawing this control's own surface/border; the host draws the frame
    // around it (e.g. an input embedded inside a custom composer shell who owns
    // the full rounded box + border look).
    bool transparent = false;
};

class TextBox : public Widget {
public:
    explicit TextBox(String text = String(), TextBoxConfig config = TextBoxConfig{});

    String text() const { return utf::to_utf8(text_); }
    TextBox& set_text(const String& text);
    TextBox& text(const String& text) { return set_text(text); }

    TextBox& set_placeholder(const String& placeholder) {
        placeholder_ = placeholder;
        return *this;
    }
    const String& placeholder() const { return placeholder_; }

    const TextBoxConfig& config() const { return config_; }
    TextBox& set_config(const TextBoxConfig& config);

    // Enter-to-submit callback for SingleLine, and for MultiLine when
    // TextBoxConfig::enter_submits is set (Ctrl+Enter then inserts a newline).
    TextBox& set_on_commit(std::function<void()> cb) {
        on_commit_cb_ = std::move(cb);
        return *this;
    }
    TextBox& on_commit(std::function<void()> cb) { return set_on_commit(std::move(cb)); }

    bool read_only() const { return config_.read_only; }
    TextBox& set_read_only(bool read_only);

    TextBox& set_content_inset(f32 inset) {
        content_inset_ = inset;
        return *this;
    }
    f32 content_inset() const { return content_inset_; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

    // ===== IME =====
    // Composition input for SingleLine / MultiLine; password fields opt out entirely.
    bool wants_ime() const override { return config_.mode != TextBoxMode::Password; }
    RectF ime_caret_rect() const override;

private:
    void move_cursor(i32 delta);
    void move_cursor_vertical(i32 delta_line);
    void delete_backward();
    void set_cursor_by_pos(f32 x, f32 y);

    void delete_selection();
    void copy_selection() const;
    void cut_selection();
    void paste_from_clipboard();
    void insert_text(const WString& text);
    u32 selection_begin() const { return cursor_ < sel_start_ ? cursor_ : sel_start_; }
    u32 selection_end() const { return cursor_ < sel_start_ ? sel_start_ : cursor_; }

    // ===== Undo / redo (5.3.1) =====
    enum class EditKind { Typing, Paste, Cut, Delete };
    struct Snapshot {
        WString text;
        u32 cursor = 0;
        u32 sel = 0;
    };
    // Records the pre-edit state unless this edit coalesces into the previous
    // group (consecutive typing/backspace runs merge into one undo step).
    void begin_edit(EditKind kind);
    void undo();
    void redo();
    // Word-boundary navigation/delete (Ctrl+Left/Right/Backspace/Delete):
    // returns the adjacent word boundary index.
    i32 word_jump(i32 delta) const;

    std::vector<Snapshot> undo_stack_;
    std::vector<Snapshot> redo_stack_;
    u64 last_edit_ms_ = 0;
    EditKind last_kind_ = EditKind::Typing;

    WString display_text() const;
    u32 line_index_at(u32 pos) const;
    u32 line_start(u32 line) const;
    u32 line_end(u32 line) const;
    // Line height derived from the loaded font's metrics (baseline anchor); a fixed
    // pixel value is only a last-resort fallback before any rendering context exists.
    f32 font_line_height(const PaintContext* ctx) const;
    f32 line_height() const;

    WString text_;
    String placeholder_;
    TextBoxConfig config_;
    std::function<void()> on_commit_cb_;
    u32 cursor_ = 0;
    u32 sel_start_ = 0;
    bool focused_ = false;
    bool caret_visible_ = true;
    bool selecting_ = false;
    f32 scroll_offset_ = 0.0f;
    // MultiLine vertical scroll (px): keeps the caret line inside the fixed-height
    // viewport as content outgrows it, using the backend's wrap-aware caret y.
    f32 scroll_v_ = 0.0f;
    f32 content_inset_ = 0.0f;

    // Live IME pre-edit string displayed (virtually) at cursor_; cleared on commit/cancel.
    WString composition_;
    u32 composition_cursor_ = 0;
};

}  // namespace yzk