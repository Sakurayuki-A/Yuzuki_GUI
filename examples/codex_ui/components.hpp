#pragma once
#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/icon.hpp>
#include <string>
#include <vector>

#include "theme.hpp"

namespace yzk {

// Panel with background and optional right border; content fills it.
// Inherits Layout so arrange_content receives local coordinates.
class Pane : public Layout {
public:
    Pane(Color bg, f32 fixed_height = 0.0f);
    void set_content(Widget* child);
    void set_border_right(f32 w, Color c);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    Color bg_;
    f32 fixed_height_ = 0.0f;
    f32 border_right_w_ = 0.0f;
    Color border_right_color_;
    Widget* content_ = nullptr;
};

// Rounded button: customizable background/hover/text colors, optional leading icon.
class RoundButton : public Widget {
public:
    RoundButton(String text, Color bg, Color bg_hover, Color text_color);

    void set_icon(IconId id, f32 size = 14.0f);
    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    virtual void on_click() {}

private:
    String text_;
    Color bg_;
    Color bg_hover_;
    Color text_color_;
    IconId icon_ = IconId::None;
    f32 icon_size_ = 14.0f;
    f32 padding_x_ = 14.0f;
    f32 height_ = 28.0f;
};

// Session list item: title + subtitle, hover highlight, click to select (accent bar).
class SessionItem : public Widget {
public:
    SessionItem(String title, String subtitle);

    bool selected() const { return selected_; }
    void set_selected(bool selected);
    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    virtual void on_select() {}

private:
    String title_;
    String subtitle_;
    bool selected_ = false;
};

// Tool-call card (bash/edit/grep): header always visible, click to expand details.
class ToolCard : public Widget {
public:
    ToolCard(String kind, String command, String status = "Completed");

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    void wrap(const PaintContext* ctx, f32 max_width);
    String kind_;
    String command_;
    String status_;
    bool expanded_ = false;
    f32 expand_progress_ = 0.0f;
    std::vector<String> lines_;
    f32 line_h_ = 18.0f;
};

// Message card: user = right gray bubble, assistant = left text, auto-wraps.
class MessageCard : public Widget {
public:
    MessageCard(bool user, String text);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    void wrap(const PaintContext* ctx, f32 max_width);
    bool user_;
    String text_;
    std::vector<String> lines_;
    f32 line_w_ = 0.0f;
    f32 line_h_ = 18.0f;
};

// Bottom input bar: multiline text box + Send button.
// Inherits Layout: arrange_content receives local coordinates.
class Composer : public Layout {
public:
    Composer();

    TextBox* input() const { return input_; }
    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    TextBox* input_ = nullptr;
    RoundButton* send_ = nullptr;
};

}  // namespace yzk