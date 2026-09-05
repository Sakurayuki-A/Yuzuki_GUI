#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <vector>
#include <string>

namespace yzk {

class Window;

// General widget-tree inspector: lists every widget under the window's root,
// lets you click a row to select it, and shows its geometry / layout / style /
// state / parent / children. A general capability — attach to any window with
// show()/toggle(), independent of examples.
//
// Self-positioned top-left via participates_in_layout() == false. Data is a
// flattened snapshot of the widget tree rebuilt on show() and refreshed on a
// 500 ms timer (so newly-added widgets appear). The selected widget is also
// outlined in its actual on-screen position so it's easy to find.
class WidgetInspector : public Widget {
public:
    WidgetInspector();
    ~WidgetInspector() override;

    WidgetInspector(const WidgetInspector&) = delete;
    WidgetInspector& operator=(const WidgetInspector&) = delete;

    void show(Window& win);
    void close();
    void toggle(Window& win) { is_open_ ? close() : show(win); }
    bool is_open() const { return is_open_; }

    void set_backdrop(const Color& color) { backdrop_ = color; invalidate(); }
    void set_text_color(const Color& color) { text_color_ = color; invalidate(); }
    void set_panel_width(f32 width) { panel_w_ = width; invalidate(); }

protected:
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    bool participates_in_layout() const override { return false; }

private:
    struct Row {
        Widget* widget = nullptr;
        u32 depth = 0;
    };

    void rebuild();
    void collect(Widget* w, u32 depth);
    const Row* hit_row(f32 local_x, f32 local_y) const;

    Window* win_ = nullptr;
    RectF panel_;
    bool is_open_ = false;
    f32 panel_w_ = 300.0f;
    size_t selected_ = SIZE_MAX;
    std::vector<Row> rows_;
    Color backdrop_ = Color{0x14, 0x14, 0x16, 220};
    Color text_color_ = Color{0xE6, 0xE6, 0xE6, 255};
    Color accent_color_ = Color{0x5E, 0xB1, 0xFF, 255};
    Color dim_color_ = Color{0xC8, 0xC8, 0xC8, 255};
};

}  // namespace yzk