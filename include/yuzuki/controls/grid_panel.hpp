#pragma once
#include <yuzuki/controls/layout.hpp>

#include <vector>

namespace yzk {

struct GridLength {
    enum class Type { Auto, Star };
    Type type = Type::Auto;
    f32 weight = 1.0f;
};

struct GridSlot {
    Widget* child = nullptr;
    i32 col = 0;
    i32 row = 0;
    i32 col_span = 1;
    i32 row_span = 1;
};

class GridPanel : public Layout {
public:
    GridPanel(i32 columns, i32 rows);

    i32 columns() const { return columns_; }
    i32 rows() const { return rows_; }

    void add(Widget* child, i32 col, i32 row, i32 col_span = 1, i32 row_span = 1);

    f32 gap() const { return gap_; }
    void set_gap(f32 gap);

    void set_column_auto(i32 col);
    void set_column_star(i32 col, f32 weight = 1.0f);

    void set_row_auto(i32 row);
    void set_row_star(i32 row, f32 weight = 1.0f);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;

private:
    i32 columns_ = 1;
    i32 rows_ = 1;
    f32 gap_ = 8.0f;
    std::vector<GridLength> col_lengths_;
    std::vector<GridLength> row_lengths_;
    std::vector<GridSlot> slots_;

    void collect_content(std::vector<f32>& col_w, std::vector<f32>& row_h,
                         const PaintContext* ctx) const;
};

}  // namespace yzk