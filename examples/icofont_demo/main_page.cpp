#include "main_page.hpp"

#include <yuzuki/ui/icon.hpp>

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>

namespace yzk {

namespace {

// ===== Icon data: name (IconId enum name) + Phosphor codepoint =====
// Codepoints come from the official Phosphor icons.ts, validated against the repo font cmap (100/100)
struct IconEntry {
    const char* name;
    u32 cp;
};

constexpr IconEntry kIcons[] = {
    {"Plus", 0xE3D4},
    {"CaretDown", 0xE136},
    {"CaretUp", 0xE13C},
    {"CaretRight", 0xE13A},
    {"CaretLeft", 0xE138},
    {"Check", 0xE182},
    {"Terminal", 0xE47E},
    {"Code", 0xE1BC},
    {"MagnifyingGlass", 0xE30C},
    {"Gear", 0xE270},
    {"PaperPlane", 0xE394},
    {"PaperPlaneTilt", 0xE398},
    {"Chat", 0xE15C},
    {"ChatCircle", 0xE168},
    {"X", 0xE4F6},
    {"DotsThree", 0xE1FE},
    {"Copy", 0xE1CA},
    {"FileCode", 0xE914},
    {"Question", 0xE3E8},
    {"ArrowDown", 0xE03E},
    {"ArrowUp", 0xE08E},
    {"ArrowRight", 0xE06C},
    {"List", 0xE2F0},
    {"Trash", 0xE4A6},
    {"PencilSimple", 0xE3B4},
    {"User", 0xE4C2},
    {"Users", 0xE4D6},
    {"BookOpen", 0xE0E6},
    {"Stack", 0xE466},
    {"WarningCircle", 0xE4E2},
    {"Info", 0xE2CE},
    {"ArrowClockwise", 0xE036},
    {"Clock", 0xE19A},
    {"CalendarDots", 0xE7B4},
    {"Camera", 0xE10E},
    {"Calendar", 0xE108},
    {"Bell", 0xE0CE},
    {"BellRinging", 0xE5E8},
    {"Bookmark", 0xE0E8},
    {"Briefcase", 0xE0EE},
    {"Browser", 0xE0F4},
    {"Buildings", 0xE102},
    {"Calculator", 0xE538},
    {"CalendarBlank", 0xE10A},
    {"CalendarCheck", 0xE712},
    {"CalendarX", 0xE10C},
    {"ChartBar", 0xE150},
    {"ChartLine", 0xE154},
    {"ChartPie", 0xE158},
    {"ChatCircleText", 0xE16E},
    {"ChatText", 0xE17A},
    {"CheckCircle", 0xE184},
    {"CheckSquare", 0xE186},
    {"Clipboard", 0xE196},
    {"ClipboardText", 0xE198},
    {"Cloud", 0xE1AA},
    {"CloudArrowDown", 0xE1AC},
    {"CloudCheck", 0xE1B0},
    {"Command", 0xE1C4},
    {"CopySimple", 0xE1CC},
    {"Cpu", 0xE610},
    {"Desktop", 0xE560},
    {"DeviceMobile", 0xE1E0},
    {"Download", 0xE20A},
    {"DownloadSimple", 0xE20C},
    {"Envelope", 0xE214},
    {"EnvelopeOpen", 0xE216},
    {"Eye", 0xE220},
    {"EyeClosed", 0xE222},
    {"File", 0xE230},
    {"FileCpp", 0xEB2E},
    {"FileCss", 0xEB34},
    {"FileDoc", 0xEB1E},
    {"FileHtml", 0xEB38},
    {"FileImage", 0xEA24},
    {"FileJs", 0xEB24},
    {"FilePdf", 0xE702},
    {"FilePng", 0xEB18},
    {"FilePy", 0xEB2C},
    {"FileText", 0xE23A},
    {"FileTs", 0xEB26},
    {"FileTxt", 0xEB32},
    {"FileXls", 0xEB22},
    {"FileZip", 0xE958},
    {"Fire", 0xE242},
    {"Flag", 0xE244},
    {"FloppyDisk", 0xE248},
    {"Folder", 0xE24A},
    {"FolderMinus", 0xE254},
    {"FolderOpen", 0xE256},
    {"FolderPlus", 0xE258},
    {"FolderSimple", 0xE25A},
    {"Gift", 0xE276},
    {"GithubLogo", 0xE576},
    {"Globe", 0xE288},
    {"GraduationCap", 0xE62C},
    {"Heart", 0xE2A8},
    {"HeartStraight", 0xE2AA},
    {"Hourglass", 0xE2B2},
    {"House", 0xE2C2},
};

constexpr i32 kIconCount = static_cast<i32>(std::size(kIcons));

// ===== Custom grid (virtualized: only visible cells are drawn) =====
constexpr f32 kCellW = 110.0f;
constexpr f32 kCellH = 96.0f;
constexpr f32 kIconSize = 30.0f;
constexpr f32 kScrollbarWidth = 6.0f;
constexpr f32 kScrollbarMargin = 4.0f;
constexpr f32 kThumbMinHeight = 24.0f;
constexpr f32 kWheelStep = 48.0f;

struct IconGridView : Widget {
    Label* status;
    i32 hovered_ = -1;
    f32 scroll_y_ = 0.0f;
    f32 max_scroll_ = 0.0f;
    bool dragging_thumb_ = false;
    f32 drag_grab_ = 0.0f;

    explicit IconGridView(Label* status_label) : status(status_label) {}

    i32 columns() const {
        const f32 usable = bounds_.width() - (max_scroll_ > 0.0f ? kScrollbarWidth + kScrollbarMargin : 0.0f);
        return std::max(1, static_cast<i32>(usable / kCellW));
    }
    i32 rows() const { return (kIconCount + columns() - 1) / columns(); }
    f32 content_height() const { return static_cast<f32>(rows()) * kCellH; }

    i32 cell_at(f32 x, f32 y) const {
        if (x < 0.0f || x >= bounds_.width() - (max_scroll_ > 0.0f ? kScrollbarWidth + kScrollbarMargin : 0.0f)) {
            return -1;
        }
        const i32 col = static_cast<i32>(x / kCellW);
        const i32 row = static_cast<i32>((y + scroll_y_) / kCellH);
        const i32 idx = row * columns() + col;
        if (col < 0 || col >= columns() || row < 0 || idx >= kIconCount) return -1;
        return idx;
    }

    void update_max_scroll() {
        const f32 content = content_height();
        const f32 view = bounds_.height();
        max_scroll_ = content > view ? content - view : 0.0f;
        if (scroll_y_ > max_scroll_) scroll_y_ = max_scroll_;
        if (scroll_y_ < 0.0f) scroll_y_ = 0.0f;
    }

    void set_scroll_y(f32 y) {
        update_max_scroll();
        if (scroll_y_ == y) return;
        scroll_y_ = y;
        invalidate();
    }

    void copy_codepoint(i32 index) {
        const IconEntry& entry = kIcons[index];
        wchar_t buf[16];
        swprintf(buf, 16, L"U+%04X", entry.cp);
        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            const size_t bytes = (wcslen(buf) + 1) * sizeof(wchar_t);
            if (HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
                void* mem = GlobalLock(h);
                if (mem) {
                    memcpy(mem, buf, bytes);
                    GlobalUnlock(h);
                    SetClipboardData(CF_UNICODETEXT, h);
                } else {
                    GlobalFree(h);
                }
            }
            CloseClipboard();
        }
        char status_buf[96];
        std::snprintf(status_buf, sizeof(status_buf), "Copied %s U+%04X - paste it anywhere",
                      entry.name, entry.cp);
        status->set_text(status_buf);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)ctx;
        return Size{available.width > 0.0f ? available.width : 0.0f,
                    available.height > 0.0f ? available.height : 0.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& theme = ctx.theme();
        const RectF& b = bounds_;
        update_max_scroll();

        const f32 view_h = b.height();
        const i32 cols = columns();
        const i32 total_rows = rows();

        ctx.push_clip(b);

        const i32 first_row = std::max(0, static_cast<i32>(std::floor(scroll_y_ / kCellH)));
        const i32 last_row = std::min(total_rows - 1, static_cast<i32>(std::ceil((scroll_y_ + view_h) / kCellH)));

        const FontId name_font = ctx.font(theme.font_family, 11.0f);
        const FontId cp_font = ctx.font(theme.font_family, 10.0f);
        const FontId icon_font = ctx.font(icon_family, kIconSize);

        for (i32 row = first_row; row <= last_row; ++row) {
            const f32 cell_top = b.top + static_cast<f32>(row) * kCellH - scroll_y_;
            for (i32 col = 0; col < cols; ++col) {
                const i32 idx = row * cols + col;
                if (idx >= kIconCount) break;
                const RectF cell = RectF::make(b.left + static_cast<f32>(col) * kCellW, cell_top, kCellW, kCellH);

                if (idx == hovered_) {
                    const RectF hl = RectF::make(cell.left + 4.0f, cell.top + 4.0f, cell.width() - 8.0f,
                                                 cell.height() - 8.0f);
                    ctx.fill_rounded(hl, theme.accent.with_alpha(36), 8.0f);
                }

                const IconEntry& entry = kIcons[idx];
                const RectF icon_rect = RectF::make(cell.left, cell.top + 12.0f, cell.width(), kIconSize + 10.0f);
                ctx.draw_text(icon_font, icon_glyph(static_cast<IconId>(entry.cp)), icon_rect,
                              idx == hovered_ ? theme.accent : theme.text);

                // Name: shrink the font to fit the cell width when 11px overflows
                const RectF name_rect = RectF::make(cell.left + 4.0f, cell.top + 54.0f,
                                                    cell.width() - 8.0f, 14.0f);
                const Size name_size = ctx.measure_text(name_font, entry.name);
                const FontId nf = name_size.width > name_rect.width()
                                      ? ctx.font(theme.font_family,
                                                 std::max(7.0f, 11.0f * name_rect.width() / name_size.width))
                                      : name_font;
                ctx.draw_text(nf, entry.name, name_rect, theme.text_secondary);

                char cp_buf[12];
                std::snprintf(cp_buf, sizeof(cp_buf), "U+%04X", entry.cp);
                const RectF cp_rect = RectF::make(cell.left, cell.top + 70.0f, cell.width(), 12.0f);
                ctx.draw_text(cp_font, cp_buf, cp_rect, theme.text_disabled);
            }
        }

        ctx.pop_clip();

        if (max_scroll_ > 0.0f) {
            const f32 track_x = b.right - kScrollbarMargin - kScrollbarWidth;
            const f32 track_top = b.top + kScrollbarMargin;
            const f32 track_h = b.height() - kScrollbarMargin * 2.0f;
            const f32 ratio = track_h / content_height();
            const f32 thumb_h = std::max(ratio * track_h, kThumbMinHeight);
            const f32 thumb_y = track_top + (track_h - thumb_h) * (scroll_y_ / max_scroll_);

            ctx.fill_rounded(RectF::make(track_x, track_top, kScrollbarWidth, track_h),
                             theme.surface_container_high, kScrollbarWidth / 2.0f);
            ctx.fill_rounded(RectF::make(track_x, thumb_y, kScrollbarWidth, thumb_h),
                             theme.border_hover, kScrollbarWidth / 2.0f);
        }
    }

    void on_event(Event& e) override {
        const RectF g = global_bounds();
        switch (e.type) {
            case EventType::Wheel:
                if (max_scroll_ > 0.0f) {
                    set_scroll_y(scroll_y_ - static_cast<f32>(e.data.mouse.wheel_delta) / 120.0f * kWheelStep);
                    e.consumed = true;
                }
                break;

            case EventType::MouseDown:
                if (enabled() && (e.data.mouse.buttons & MouseButton_Left)) {
                    const f32 x = e.data.mouse.x - g.left;
                    const f32 y = e.data.mouse.y - g.top;
                    if (scrollbar_hit(x)) {
                        const f32 track_top = kScrollbarMargin;
                        const f32 track_h = bounds_.height() - kScrollbarMargin * 2.0f;
                        const f32 ratio = track_h / content_height();
                        const f32 thumb_h = std::max(ratio * track_h, kThumbMinHeight);
                        const f32 thumb_y = track_top + (track_h - thumb_h) * (scroll_y_ / max_scroll_);
                        if (y >= thumb_y && y <= thumb_y + thumb_h) {
                            dragging_thumb_ = true;
                            drag_grab_ = y - thumb_y;
                        } else {
                            const f32 dy = track_h > thumb_h ? (y - thumb_y - thumb_h) : 0.0f;
                            set_scroll_y(dy / (track_h - thumb_h) * max_scroll_);
                        }
                        e.consumed = true;
                    } else {
                        const i32 idx = cell_at(x, y);
                        if (idx >= 0) {
                            copy_codepoint(idx);
                            e.consumed = true;
                        }
                    }
                }
                break;

            case EventType::MouseMove:
                if (dragging_thumb_ && max_scroll_ > 0.0f) {
                    const f32 y = e.data.mouse.y - g.top;
                    const f32 track_top = kScrollbarMargin;
                    const f32 track_h = bounds_.height() - kScrollbarMargin * 2.0f;
                    const f32 ratio = track_h / content_height();
                    const f32 thumb_h = std::max(ratio * track_h, kThumbMinHeight);
                    const f32 range = track_h - thumb_h;
                    if (range > 0.0f) {
                        set_scroll_y((y - drag_grab_ - track_top) / range * max_scroll_);
                    }
                    e.consumed = true;
                    break;
                }
                {
                    const i32 idx = cell_at(e.data.mouse.x - g.left, e.data.mouse.y - g.top);
                    if (idx != hovered_) {
                        hovered_ = idx;
                        invalidate();
                    }
                }
                break;

            case EventType::MouseUp:
                if (dragging_thumb_) {
                    dragging_thumb_ = false;
                    e.consumed = true;
                }
                break;

            case EventType::MouseLeave:
                if (hovered_ != -1) {
                    hovered_ = -1;
                    invalidate();
                }
                break;

            default:
                break;
        }
    }

    Widget* hit_test(f32 x, f32 y) override {
        if (!visible()) return nullptr;
        if (x < 0.0f || y < 0.0f || x > bounds_.width() || y > bounds_.height()) return nullptr;
        return this;
    }

    bool scrollbar_hit(f32 x) const {
        if (max_scroll_ <= 0.0f) return false;
        return x >= bounds_.width() - (kScrollbarWidth + kScrollbarMargin);
    }
};

}  // namespace

Widget* make_icofont_page(Window& window) {
    (void)window;
    auto root = new DockPanel;

    auto title = new Label("Phosphor Icons (100 common)");
    title->set_bold(true);
    root->dock(title, Dock::Top);

    auto status = new Label("Click an icon to copy its codepoint (U+XXXX, same as the IconId enum value)");
    status->set_text_role(TextRole::Secondary);
    status->set_small(true);
    status->set_align(TextAlignH::Left, TextAlignV::Center);
    root->dock(status, Dock::Bottom);

    auto grid = new IconGridView(status);
    root->dock(grid, Dock::Fill);

    return root;
}

}  // namespace yzk