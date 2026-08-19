#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/overlay.hpp>

#include <vector>

namespace yzk {

class ComboBox : public Widget {
public:
    ComboBox();
    explicit ComboBox(std::vector<String> items);
    ~ComboBox() override;

    void set_items(std::vector<String> items);
    const std::vector<String>& items() const { return items_; }
    void clear_items();

    i32 selected_index() const { return selected_; }
    void set_selected_index(i32 index);
    const String& selected_text() const;
    bool has_selection() const { return selected_ >= 0; }

    bool is_open() const;
    void open_popup();
    void close_popup();

    void set_placeholder(String placeholder) { placeholder_ = std::move(placeholder); }
    const String& placeholder() const { return placeholder_; }

    void set_width(f32 width) { width_ = width; }
    f32 width() const { return width_; }

    virtual void on_change(i32 index) { (void)index; }

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
};

}  // namespace yzk