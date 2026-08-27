#pragma once
#include <yuzuki/controls/layout.hpp>

#include <vector>

namespace yzk {

struct GridLength {
    enum class Type { Auto, Star, Fixed };
    Type type = Type::Auto;
    f32 weight = 1.0f;  // Star: share of the leftover space
    f32 value = 0.0f;   // Fixed: exact extent in DIPs
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

    // Un-hide Widget::add<T>() — otherwise GridPanel::add(Widget*, col, row)
    // shadows the Lego-style template from the base class.
    using Widget::add;

    f32 gap() const { return gap_; }
    GridPanel& set_gap(f32 gap);
    GridPanel& gap(f32 gap) { return set_gap(gap); }

    GridPanel& set_column_auto(i32 col);
    GridPanel& set_column_star(i32 col, f32 weight = 1.0f);
    GridPanel& set_row_auto(i32 row);
    GridPanel& set_row_star(i32 row, f32 weight = 1.0f);
    // Fixed tracks take an exact DIP extent regardless of content; they never receive
    // star distribution. Spans crossing a Fixed column use its value verbatim.
    GridPanel& set_column_fixed(i32 col, f32 width);
    GridPanel& set_row_fixed(i32 row, f32 height);

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
    static void apply_fixed(std::vector<f32>& widths, const std::vector<GridLength>& lengths);
};

}  // namespace yzk