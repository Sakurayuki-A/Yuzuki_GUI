#pragma once
#include <yuzuki/ui/widget.hpp>

namespace yzk {

class Layout : public Widget {
public:
    f32 padding() const { return padding_; }
    Layout& set_padding(f32 padding) {
        padding_ = padding;
        invalidate();
        return *this;
    }
    // Fluent short name (Lego-style); same as set_padding.
    Layout& padding(f32 padding) { return set_padding(padding); }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;

protected:
    virtual Size measure_content(Size available, const PaintContext* ctx);
    virtual void arrange_content(const RectF& area, const PaintContext* ctx);

private:
    f32 padding_ = 0.0f;
};

}  // namespace yzk