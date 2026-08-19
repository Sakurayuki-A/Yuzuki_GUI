#pragma once
#include <yuzuki/ui/widget.hpp>

namespace yzk {

class Layout : public Widget {
public:
    f32 padding() const { return padding_; }
    void set_padding(f32 padding) {
        padding_ = padding;
        invalidate();
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;

protected:
    virtual Size measure_content(Size available, const PaintContext* ctx);
    virtual void arrange_content(const RectF& area, const PaintContext* ctx);

private:
    f32 padding_ = 0.0f;
};

}  // namespace yzk