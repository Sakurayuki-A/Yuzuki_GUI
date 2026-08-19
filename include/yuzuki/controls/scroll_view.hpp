#pragma once
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/widget.hpp>

namespace yzk {

class ScrollView : public Widget {
public:
    ScrollView() = default;

    void set_content(Widget* content);
    Widget* content() const { return content_; }

    f32 scroll_y() const { return scroll_y_; }
    void set_scroll_y(f32 y);
    void scroll_by(f32 dy);

    bool has_scrollbar() const { return max_scroll_ > 0.0f; }

    f32 suggested_height() const { return suggested_height_; }
    void set_suggested_height(f32 height);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    Widget* hit_test(f32 x, f32 y) override;

private:
    void clamp_scroll();
    bool scrollbar_hit(f32 x) const;

    Widget* content_ = nullptr;
    f32 scroll_y_ = 0.0f;
    f32 max_scroll_ = 0.0f;
    f32 content_height_ = 0.0f;
    f32 suggested_height_ = 220.0f;
    bool dragging_thumb_ = false;
    f32 drag_grab_ = 0.0f;
};

}  // namespace yzk
