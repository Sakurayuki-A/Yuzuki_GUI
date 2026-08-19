// D2D backend: font registration / text layout cache / measuring / hit testing / drawing.
// Layouts are cached with LRU eviction (2048 entries); a cache hit reuses the same IDWriteTextLayout.
#include "d2d_backend.hpp"

#include <dwrite_3.h>

namespace yzk {

bool D2DBackend::add_font_file(const String& path) {
    if (!dwrite_ || path.empty()) return false;

    const WString wide_path = utf::to_wide(path);

    Microsoft::WRL::ComPtr<IDWriteFactory3> factory3;
    HRESULT hr = dwrite_.As(&factory3);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDWriteFontFile> font_file;
    hr = factory3->CreateFontFileReference(wide_path.c_str(), nullptr, &font_file);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder> builder_base;
    hr = factory3->CreateFontSetBuilder(&builder_base);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder1> builder;
    builder_base.As(&builder);
    if (!builder) return false;

    hr = builder->AddFontFile(font_file.Get());
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDWriteFontSet> font_set;
    hr = builder->CreateFontSet(&font_set);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDWriteFontCollection1> collection1;
    hr = factory3->CreateFontCollectionFromFontSet(font_set.Get(), &collection1);
    if (FAILED(hr)) return false;
    collection1.As(&font_collection_);
    return true;
}

FontId D2DBackend::create_font(const FontSpec& spec) {
    if (!dwrite_) return kInvalidFont;

    const auto it = font_cache_.find(spec);
    if (it != font_cache_.end()) return it->second;

    IDWriteFontCollection* collection = nullptr;
    if (font_collection_) {
        const WString family = utf::to_wide(spec.family);
        UINT32 index = 0;
        BOOL exists = FALSE;
        font_collection_->FindFamilyName(family.c_str(), &index, &exists);
        if (exists) collection = font_collection_.Get();
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    HRESULT hr = dwrite_->CreateTextFormat(
        utf::to_wide(spec.family).c_str(), collection,
        static_cast<DWRITE_FONT_WEIGHT>(spec.weight),
        spec.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, spec.size, L"", &format);
    if (FAILED(hr)) return kInvalidFont;

    fonts_.push_back(std::move(format));
    const FontId id = static_cast<FontId>(fonts_.size());
    font_cache_[spec] = id;
    return id;
}

bool D2DBackend::get_layout(FontId font, const std::wstring& wide, f32 width, f32 height,
                            Microsoft::WRL::ComPtr<IDWriteTextLayout>* out_layout,
                            DWRITE_TEXT_METRICS* out_metrics) {
    if (font == kInvalidFont || font > fonts_.size() || wide.empty()) return false;

    const TextLayoutKey key{font, wide, width, height};
    auto it = layout_cache_.find(key);
    if (it != layout_cache_.end()) {
        // Cache hit: move to MRU position (list head).
        lru_order_.splice(lru_order_.begin(), lru_order_, it->second.lru_it);
        *out_layout = it->second.layout;
        *out_metrics = it->second.metrics;
        return true;
    }

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = dwrite_->CreateTextLayout(
        wide.c_str(), static_cast<UINT32>(wide.size()), fonts_[font - 1].Get(), width, height,
        &layout);
    if (FAILED(hr)) return false;

    DWRITE_TEXT_METRICS metrics{};
    hr = layout->GetMetrics(&metrics);
    if (FAILED(hr)) return false;

    // LRU eviction: remove the least-recently-used (list tail) entry when over capacity.
    constexpr size_t kMaxLayouts = 2048;
    while (layout_cache_.size() >= kMaxLayouts) {
        const TextLayoutKey victim = lru_order_.back();
        lru_order_.pop_back();
        layout_cache_.erase(victim);
    }

    auto entry_it = layout_cache_.try_emplace(key).first;
    entry_it->second.layout = layout;
    entry_it->second.metrics = metrics;
    lru_order_.push_front(key);
    entry_it->second.lru_it = lru_order_.begin();

    *out_layout = layout;
    *out_metrics = metrics;
    return true;
}

Size D2DBackend::measure_text(FontId font, const String& text, f32 max_width) {
    if (font == kInvalidFont || text.empty()) return Size{};
    if (max_width <= 0.0f) max_width = 1e7f;

    const WString wide = utf::to_wide(text);
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    DWRITE_TEXT_METRICS metrics{};
    if (!get_layout(font, wide, max_width, 1e7f, &layout, &metrics)) return Size{};

    return Size{metrics.width, metrics.height};
}

i32 D2DBackend::hit_test_text(FontId font, const String& text, f32 width, f32 x, f32 y) {
    if (font == kInvalidFont || font > fonts_.size() || text.empty()) return 0;
    if (width <= 0.0f) width = 1e7f;

    const WString wide = utf::to_wide(text);
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    DWRITE_TEXT_METRICS dummy{};
    if (!get_layout(font, wide, width, 1e7f, &layout, &dummy)) return 0;

    BOOL is_trailing = FALSE;
    BOOL is_inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics{};
    HRESULT hr = layout->HitTestPoint(x, y, &is_trailing, &is_inside, &metrics);
    if (FAILED(hr)) return 0;

    i32 pos = static_cast<i32>(metrics.textPosition);
    if (pos > static_cast<i32>(wide.size())) pos = static_cast<i32>(wide.size());
    if (is_trailing && pos < static_cast<i32>(wide.size())) ++pos;
    return pos;
}

Point D2DBackend::caret_position(FontId font, const String& text, f32 width, i32 pos) {
    if (font == kInvalidFont || font > fonts_.size() || text.empty()) return Point{};
    if (width <= 0.0f) width = 1e7f;
    if (pos < 0) pos = 0;
    const UINT32 size = static_cast<UINT32>(utf::to_wide(text).size());
    if (pos > static_cast<i32>(size)) pos = static_cast<i32>(size);

    const WString wide = utf::to_wide(text);
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    DWRITE_TEXT_METRICS dummy{};
    if (!get_layout(font, wide, width, 1e7f, &layout, &dummy)) return Point{};

    FLOAT x = 0.0f;
    FLOAT y = 0.0f;
    DWRITE_HIT_TEST_METRICS metrics{};
    HRESULT hr = layout->HitTestTextPosition(static_cast<UINT32>(pos), FALSE, &x, &y, &metrics);
    if (FAILED(hr)) return Point{};

    return Point{x, y};
}

std::vector<TextSelectionRect> D2DBackend::text_selection_rects(FontId font, const String& text,
                                                                f32 width, f32 height, i32 begin,
                                                                i32 end) {
    std::vector<TextSelectionRect> result;
    if (font == kInvalidFont || font > fonts_.size() || text.empty()) return result;
    if (width <= 0.0f) width = 1e7f;
    if (height <= 0.0f) height = 1e7f;

    const WString wide = utf::to_wide(text);
    const UINT32 size = static_cast<UINT32>(wide.size());
    if (begin < 0) begin = 0;
    if (end > static_cast<i32>(size)) end = static_cast<i32>(size);
    if (begin >= end) return result;

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    DWRITE_TEXT_METRICS dummy{};
    if (!get_layout(font, wide, width, height, &layout, &dummy)) return result;

    const UINT32 length = static_cast<UINT32>(end - begin);
    std::vector<DWRITE_HIT_TEST_METRICS> metrics(length);
    UINT32 actual = 0;
    HRESULT hr = layout->HitTestTextRange(static_cast<UINT32>(begin), length, 0.0f, 0.0f,
                                          metrics.data(), length, &actual);
    if (FAILED(hr)) return result;
    metrics.resize(actual);
    if (metrics.empty()) return result;

    TextSelectionRect cur;
    cur.rect = RectF::make(metrics[0].left, metrics[0].top, metrics[0].width, metrics[0].height);
    cur.begin = begin + static_cast<i32>(metrics[0].textPosition - begin);
    cur.end = cur.begin + static_cast<i32>(metrics[0].length);
    for (UINT32 i = 1; i < actual; ++i) {
        const DWRITE_HIT_TEST_METRICS& m = metrics[i];
        if (m.length == 0) continue;
        const f32 left = m.left;
        const f32 top = m.top;
        if (std::abs(top - cur.rect.top) < 0.5f &&
            std::abs(left - (cur.rect.left + cur.rect.width())) < 0.5f) {
            cur.rect = RectF::make(cur.rect.left, cur.rect.top, left + m.width - cur.rect.left,
                                   cur.rect.height());
            cur.end = begin + static_cast<i32>(m.textPosition - begin + m.length);
        } else {
            result.push_back(cur);
            cur.rect = RectF::make(left, top, m.width, m.height);
            cur.begin = begin + static_cast<i32>(m.textPosition - begin);
            cur.end = cur.begin + static_cast<i32>(m.length);
        }
    }
    result.push_back(cur);
    return result;
}

void D2DBackend::draw_text(FontId font, const String& text, const RectF& rect,
                           const Color& color, TextAlignH align_h, TextAlignV align_v) {
    if (font == kInvalidFont || font > fonts_.size() || text.empty()) return;
    if (!ensure_brush(color)) return;

    const WString wide = utf::to_wide(text);
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    DWRITE_TEXT_METRICS metrics{};
    if (!get_layout(font, wide, rect.width(), rect.height(), &layout, &metrics)) return;

    D2D1_POINT_2F origin{};
    switch (align_h) {
        case TextAlignH::Left: origin.x = rect.left; break;
        case TextAlignH::Center: origin.x = rect.left + (rect.width() - metrics.width) / 2.0f; break;
        case TextAlignH::Right: origin.x = rect.right - metrics.width; break;
    }
    switch (align_v) {
        case TextAlignV::Top: origin.y = rect.top; break;
        case TextAlignV::Center: origin.y = rect.top + (rect.height() - metrics.height) / 2.0f; break;
        case TextAlignV::Bottom: origin.y = rect.bottom - metrics.height; break;
    }

    context_->DrawTextLayout(origin, layout.Get(), brush_.Get());
}

}  // namespace yzk
