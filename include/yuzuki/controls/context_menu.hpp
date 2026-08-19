#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>
#include <vector>

namespace yzk {

class Window;

struct ContextMenuItem {
    String text;
    std::function<void()> action;
    bool separator = false;
    bool enabled = true;
};

// Context menu: pops at the cursor, highlights on hover, executes on click, closes on outside click / Esc
class ContextMenu : public Widget {
public:
    ContextMenu() = default;

    void add_item(const String& text, std::function<void()> action);
    void add_separator();
    void clear_items();
    const std::vector<ContextMenuItem>& items() const { return items_; }

    void open(Window& win, f32 x, f32 y);
    void close();
    bool is_open() const { return open_; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    Widget* hit_test(f32 x, f32 y) override;

private:
    i32 index_at(f32 x, f32 y) const;

    std::vector<ContextMenuItem> items_;
    i32 hover_ = -1;
    f32 progress_ = 0.0f;
    f32 open_x_ = 0.0f;
    f32 open_y_ = 0.0f;
    bool open_ = false;
};

}  // namespace yzk