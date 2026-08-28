#pragma once
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/widget.hpp>

#include <functional>
#include <vector>

namespace yzk {

class ListView : public Widget {
public:
    // Data source interface: rows by index. Use for large lists (10k+);
    // the list holds a non-owning pointer; call invalidate() after data changes.
    class DataSource {
    public:
        virtual ~DataSource() = default;
        virtual i32 count() const = 0;
        virtual String text_at(i32 index) const = 0;
    };

    // Custom row rendering; the highlight background is already painted,
    // the delegate only draws row content. Non-owning pointer.
    class RowDelegate {
    public:
        virtual ~RowDelegate() = default;
        virtual void draw(ListView& view, PaintContext& ctx, i32 index,
                          const RectF& row_rect) = 0;
    };

    ListView() = default;

    // Internal vector mode (default); set_items clears any data source
    ListView& set_items(const std::vector<String>& items);
    ListView& add_item(const String& item);
    ListView& clear_items();
    const std::vector<String>& items() const { return items_; }

    // Data source mode; items() is no longer valid. Rows are fetched on
    // demand (visible rows only; virtualization avoids full traversal).
    ListView& set_data_source(DataSource* source);
    DataSource* data_source() const { return source_; }

    // Row rendering; nullptr = default text rows
    ListView& set_row_delegate(RowDelegate* delegate);
    RowDelegate* row_delegate() const { return delegate_; }

    i32 count() const { return source_ ? source_->count() : static_cast<i32>(items_.size()); }
    String text_at(i32 index) const {
        return source_ ? source_->text_at(index) : items_[static_cast<u32>(index)];
    }

    i32 selected() const { return selected_; }
    i32 hovered() const { return hovered_; }
    ListView& set_selected(i32 index);
    ListView& set_hovered(i32 index);

    f32 row_height() const { return row_height_; }
    ListView& set_row_height(f32 height);

    f32 scroll_y() const { return scroll_y_; }
    ListView& set_scroll_y(f32 y);
    ListView& scroll_by(f32 dy);

    ListView& set_show_border(bool show) {
        show_border_ = show;
        invalidate();
        return *this;
    }
    ListView& set_show_scrollbar(bool show) {
        show_scrollbar_ = show;
        invalidate();
        return *this;
    }

    // Convenience callback registration; the virtual hook below is invoked with it.
    ListView& set_on_selected(std::function<void(i32)> cb) {
        on_selected_cb_ = std::move(cb);
        return *this;
    }
    // Fluent short name (Lego-style); same as set_on_selected.
    ListView& on_selected(std::function<void(i32)> cb) { return set_on_selected(std::move(cb)); }

    virtual void on_selected(i32 index) {
        if (on_selected_cb_) on_selected_cb_(index);
    }

    // Double-click (or Enter) activation callback; the virtual hook below fires with it.
    ListView& set_on_activate(std::function<void(i32)> cb) {
        on_activate_cb_ = std::move(cb);
        return *this;
    }
    // Fluent short name (Lego-style); same as set_on_activate.
    ListView& on_activate(std::function<void(i32)> cb) { return set_on_activate(std::move(cb)); }

    virtual void on_activate(i32 index) {
        if (on_activate_cb_) on_activate_cb_(index);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    Widget* hit_test(f32 x, f32 y) override;

private:
    i32 row_at_y(f32 y) const;
    f32 content_height() const { return static_cast<f32>(count()) * row_height_; }
    void update_max_scroll();
    void clamp_scroll();
    bool scrollbar_hit(f32 x) const;

    std::vector<String> items_;
    DataSource* source_ = nullptr;
    RowDelegate* delegate_ = nullptr;
    i32 selected_ = -1;
    i32 hovered_ = -1;
    f32 row_height_ = 28.0f;
    bool show_border_ = true;
    bool show_scrollbar_ = true;

    f32 scroll_y_ = 0.0f;
    f32 max_scroll_ = 0.0f;
    bool dragging_thumb_ = false;
    f32 drag_grab_ = 0.0f;
    std::function<void(i32)> on_selected_cb_;
    std::function<void(i32)> on_activate_cb_;
};

}  // namespace yzk