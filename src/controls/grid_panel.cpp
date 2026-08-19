#include <yuzuki/controls/grid_panel.hpp>

namespace yzk {

GridPanel::GridPanel(i32 columns, i32 rows) {
    columns_ = columns > 0 ? columns : 1;
    rows_ = rows > 0 ? rows : 1;
    col_lengths_.assign(static_cast<size_t>(columns_), GridLength{});
    row_lengths_.assign(static_cast<size_t>(rows_), GridLength{});
}

void GridPanel::add(Widget* child, i32 col, i32 row, i32 col_span, i32 row_span) {
    if (!child) return;
    if (col < 0) col = 0;
    if (row < 0) row = 0;
    if (col_span < 1) col_span = 1;
    if (row_span < 1) row_span = 1;
    append_child(child);
    slots_.push_back(GridSlot{child, col, row, col_span, row_span});
}

void GridPanel::set_gap(f32 gap) {
    if (gap_ == gap) return;
    gap_ = gap;
    invalidate();
}

void GridPanel::set_column_auto(i32 col) {
    if (col < 0 || col >= columns_) return;
    col_lengths_[static_cast<size_t>(col)] = GridLength{};
    invalidate();
}

void GridPanel::set_column_star(i32 col, f32 weight) {
    if (col < 0 || col >= columns_) return;
    GridLength len;
    len.type = GridLength::Type::Star;
    len.weight = weight > 0.0f ? weight : 1.0f;
    col_lengths_[static_cast<size_t>(col)] = len;
    invalidate();
}

void GridPanel::set_row_auto(i32 row) {
    if (row < 0 || row >= rows_) return;
    row_lengths_[static_cast<size_t>(row)] = GridLength{};
    invalidate();
}

void GridPanel::set_row_star(i32 row, f32 weight) {
    if (row < 0 || row >= rows_) return;
    GridLength len;
    len.type = GridLength::Type::Star;
    len.weight = weight > 0.0f ? weight : 1.0f;
    row_lengths_[static_cast<size_t>(row)] = len;
    invalidate();
}

void GridPanel::collect_content(std::vector<f32>& col_w, std::vector<f32>& row_h,
                               const PaintContext* ctx) const {
    col_w.assign(static_cast<size_t>(columns_), 0.0f);
    row_h.assign(static_cast<size_t>(rows_), 0.0f);
    for (const GridSlot& slot : slots_) {
        if (!slot.child->visible()) continue;
        const Size s = slot.child->measure(Size{1e7f, 1e7f}, ctx);
        const Margins m = slot.child->margin();
        const f32 per_col =
            (s.width + m.horizontal() - static_cast<f32>(slot.col_span - 1) * gap_) /
            static_cast<f32>(slot.col_span);
        const size_t cEnd = slot.col + slot.col_span > columns_
                                ? static_cast<size_t>(columns_)
                                : static_cast<size_t>(slot.col + slot.col_span);
        for (size_t c = static_cast<size_t>(slot.col); c < cEnd; ++c) {
            if (per_col > col_w[c]) col_w[c] = per_col;
        }
        const f32 per_row =
            (s.height + m.vertical() - static_cast<f32>(slot.row_span - 1) * gap_) /
            static_cast<f32>(slot.row_span);
        const size_t rEnd = slot.row + slot.row_span > rows_
                                ? static_cast<size_t>(rows_)
                                : static_cast<size_t>(slot.row + slot.row_span);
        for (size_t r = static_cast<size_t>(slot.row); r < rEnd; ++r) {
            if (per_row > row_h[r]) row_h[r] = per_row;
        }
    }
}

Size GridPanel::measure_content(Size available, const PaintContext* ctx) {
    if (slots_.empty()) return Size{0.0f, 0.0f};
    std::vector<f32> col_w;
    std::vector<f32> row_h;
    collect_content(col_w, row_h, ctx);

    constexpr f32 kUnbounded = 1e7f;

    f32 used_w = 0.0f;
    for (f32 cw : col_w) used_w += cw;
    const f32 gaps_w = gap_ * static_cast<f32>(columns_ - 1);
    f32 w = used_w + gaps_w;
    if (available.width < kUnbounded) {
        bool has_star = false;
        for (i32 c = 0; c < columns_; ++c) {
            if (col_lengths_[static_cast<size_t>(c)].type == GridLength::Type::Star) {
                has_star = true;
                break;
            }
        }
        if (has_star) {
            const f32 remaining_w = available.width - used_w - gaps_w;
            if (remaining_w > 0.0f) w = used_w + gaps_w + remaining_w;
        }
    }

    f32 used_h = 0.0f;
    for (f32 rh : row_h) used_h += rh;
    const f32 gaps_h = gap_ * static_cast<f32>(rows_ - 1);
    f32 h = used_h + gaps_h;
    if (available.height < kUnbounded) {
        bool has_star = false;
        for (i32 r = 0; r < rows_; ++r) {
            if (row_lengths_[static_cast<size_t>(r)].type == GridLength::Type::Star) {
                has_star = true;
                break;
            }
        }
        if (has_star) {
            const f32 remaining_h = available.height - used_h - gaps_h;
            if (remaining_h > 0.0f) h = used_h + gaps_h + remaining_h;
        }
    }

    return Size{w, h};
}

void GridPanel::arrange_content(const RectF& area, const PaintContext* ctx) {
    std::vector<f32> content_w;
    std::vector<f32> content_h;
    collect_content(content_w, content_h, ctx);

    std::vector<f32> col_w(static_cast<size_t>(columns_), 0.0f);
    f32 star_weight = 0.0f;
    f32 used = 0.0f;
    for (i32 c = 0; c < columns_; ++c) {
        col_w[static_cast<size_t>(c)] = content_w[static_cast<size_t>(c)];
        used += content_w[static_cast<size_t>(c)];
        if (col_lengths_[static_cast<size_t>(c)].type == GridLength::Type::Star) {
            star_weight += col_lengths_[static_cast<size_t>(c)].weight;
        }
    }
    const f32 gaps = gap_ * static_cast<f32>(columns_ - 1);
    const f32 remaining = area.width() - used - gaps;
    if (star_weight > 0.0f && remaining > 0.0f) {
        for (i32 c = 0; c < columns_; ++c) {
            if (col_lengths_[static_cast<size_t>(c)].type == GridLength::Type::Star) {
                col_w[static_cast<size_t>(c)] +=
                    remaining * col_lengths_[static_cast<size_t>(c)].weight / star_weight;
            }
        }
    }

    std::vector<f32> col_x(static_cast<size_t>(columns_), area.left);
    for (i32 c = 1; c < columns_; ++c) {
        col_x[static_cast<size_t>(c)] =
            col_x[static_cast<size_t>(c - 1)] + col_w[static_cast<size_t>(c - 1)] + gap_;
    }

    std::vector<f32> row_h(static_cast<size_t>(rows_), 0.0f);
    f32 star_row_weight = 0.0f;
    f32 used_h = 0.0f;
    for (i32 r = 0; r < rows_; ++r) {
        row_h[static_cast<size_t>(r)] = content_h[static_cast<size_t>(r)];
        used_h += content_h[static_cast<size_t>(r)];
        if (row_lengths_[static_cast<size_t>(r)].type == GridLength::Type::Star) {
            star_row_weight += row_lengths_[static_cast<size_t>(r)].weight;
        }
    }
    const f32 gaps_h = gap_ * static_cast<f32>(rows_ - 1);
    const f32 remaining_h = area.height() - used_h - gaps_h;
    if (star_row_weight > 0.0f && remaining_h > 0.0f) {
        for (i32 r = 0; r < rows_; ++r) {
            if (row_lengths_[static_cast<size_t>(r)].type == GridLength::Type::Star) {
                row_h[static_cast<size_t>(r)] +=
                    remaining_h * row_lengths_[static_cast<size_t>(r)].weight / star_row_weight;
            }
        }
    }

    std::vector<f32> row_y(static_cast<size_t>(rows_), area.top);
    for (i32 r = 1; r < rows_; ++r) {
        row_y[static_cast<size_t>(r)] =
            row_y[static_cast<size_t>(r - 1)] + row_h[static_cast<size_t>(r - 1)] + gap_;
    }

    for (const GridSlot& slot : slots_) {
        const size_t c0 = static_cast<size_t>(slot.col);
        const size_t r0 = static_cast<size_t>(slot.row);
        const size_t cEnd = slot.col + slot.col_span > columns_
                                ? static_cast<size_t>(columns_)
                                : static_cast<size_t>(slot.col + slot.col_span);
        const size_t rEnd = slot.row + slot.row_span > rows_
                                ? static_cast<size_t>(rows_)
                                : static_cast<size_t>(slot.row + slot.row_span);
        f32 w = 0.0f;
        for (size_t c = c0; c < cEnd; ++c) w += col_w[c];
        w += gap_ * static_cast<f32>(cEnd - c0 - 1);
        f32 h = 0.0f;
        for (size_t r = r0; r < rEnd; ++r) h += row_h[r];
        h += gap_ * static_cast<f32>(rEnd - r0 - 1);
        const Margins m = slot.child->margin();
        slot.child->set_bounds(RectF::make(
            col_x[c0] + m.left, row_y[r0] + m.top,
            w > m.horizontal() ? w - m.horizontal() : 0.0f,
            h > m.vertical() ? h - m.vertical() : 0.0f));
        slot.child->perform_layout(ctx);
    }
}

}  // namespace yzk