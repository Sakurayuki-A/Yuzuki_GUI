#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/overlay.hpp>

#include <functional>
#include <vector>

namespace yzk {

class ComboBox : public Widget {
public:
    ComboBox();
    explicit ComboBox(std::vector<String> items);
    ~ComboBox() override;

    ComboBox& set_items(std::vector<String> items);
    const std::vector<String>& items() const { return items_; }
    ComboBox& clear_items();

    i32 selected_index() const { return selected_; }
    ComboBox& set_selected_index(i32 index);
    const String& selected_text() const;
    bool has_selection() const { return selected_ >= 0; }

    bool is_open() const;
    ComboBox& open_popup();
    ComboBox& close_popup();

    ComboBox& set_placeholder(String placeholder) {
        placeholder_ = std::move(placeholder);
        return *this;
    }
    const String& placeholder() const { return placeholder_; }

    ComboBox& set_width(f32 width) {
        width_ = width;
        return *this;
    }
    f32 width() const { return width_; }

    // Convenience callback registration; the virtual hook below is invoked with it.
    ComboBox& set_on_changed(std::function<void(i32)> cb) {
        on_changed_cb_ = std::move(cb);
        return *this;
    }
    // Fluent short name (Lego-style); same as set_on_changed.
    ComboBox& on_changed(std::function<void(i32)> cb) { return set_on_changed(std::move(cb)); }

    virtual void on_changed(i32 index) {
        if (on_changed_cb_) on_changed_cb_(index);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    class Popup;

    std::vector<String> items_;
    i32 selected_ = -1;
    f32 width_ = 160.0f;
    String placeholder_ = "Select...";
    Popup* popup_ = nullptr;
    std::function<void(i32)> on_changed_cb_;
};

}  // namespace yzk