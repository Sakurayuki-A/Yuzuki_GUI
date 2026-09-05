#include <yuzuki/ui/debug_overlay.hpp>
#include <yuzuki/ui/window.hpp>

#if defined(_WIN32)
#include <psapi.h>
#endif

#include <chrono>
#include <cstdio>

namespace yzk {

namespace {

constexpr u32 kPollIntervalMs = 500;

}  // namespace

DebugOverlay::DebugOverlay() = default;
DebugOverlay::~DebugOverlay() = default;

void DebugOverlay::show(Window& win) {
    if (is_open_) return;
    Widget* root = win.root();
    if (!root) return;

    win_ = &win;
    is_open_ = true;
    root->append_child(this);
    maybe_start_timer();
    win.invalidate_all();
}

void DebugOverlay::close() {
    if (!is_open_) return;
    is_open_ = false;
    if (win_) {
        win_->stop_timer(this);
        win_->invalidate_all();
    }
    remove_from_parent();
    win_ = nullptr;
}

void DebugOverlay::maybe_start_timer() {
    if (!win_ || !is_open_) return;
    last_frames_ = win_->frame_stats().frames;
    last_total_ = win_->frame_stats().total_ms;
    last_layout_ = win_->frame_stats().layout_ms;
    last_record_ = win_->frame_stats().record_ms;
    last_replay_ = win_->frame_stats().replay_ms;
    last_cmds_ = win_->frame_stats().commands;
    last_poll_ms_ = static_cast<f64>(
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    win_->start_timer(this, kPollIntervalMs);
}

void DebugOverlay::poll() {
    if (!win_ || !is_open_) return;
    const auto& st = win_->frame_stats();
    const f64 now = static_cast<f64>(
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    const f64 elapsed = now - last_poll_ms_;
    if (elapsed > 0.0 && st.frames > last_frames_) {
        const f64 df = static_cast<f64>(st.frames - last_frames_);
        last_fps_ = static_cast<f32>(df / (elapsed / 1000.0));
        last_total_ = (st.total_ms - last_total_) / static_cast<f32>(df);
        last_layout_ = (st.layout_ms - last_layout_) / static_cast<f32>(df);
        last_record_ = (st.record_ms - last_record_) / static_cast<f32>(df);
        last_replay_ = (st.replay_ms - last_replay_) / static_cast<f32>(df);
        last_cmds_ = static_cast<u64>((st.commands - last_cmds_) / df);
        last_poll_ms_ = now;
        invalidate();
    }
    last_frames_ = st.frames;
    last_poll_ms_ = now;
}

void DebugOverlay::perform_layout(const PaintContext* ctx) {
    if (win_) set_bounds(win_->bounds());
    Widget::perform_layout(ctx);
}

void DebugOverlay::paint_impl(PaintContext& ctx) {
    const f32 ox = ctx.offset_x(), oy = ctx.offset_y();
    ctx.set_offset(ox + bounds_.left, oy + bounds_.top);

    constexpr f32 kPadX = 10.0f, kPadY = 6.0f;
    constexpr f32 kLineH = 15.0f;
    constexpr f32 kMinW = 170.0f;

    char lines[5][48];
    std::snprintf(lines[0], sizeof(lines[0]), "FPS  %.0f", last_fps_);
    std::snprintf(lines[1], sizeof(lines[1]), "Frame %.2f ms", last_total_);
    std::snprintf(lines[2], sizeof(lines[2]), "Layout %.2f ms", last_layout_);
    std::snprintf(lines[3], sizeof(lines[3]), "Paint %.2f ms", last_record_ + last_replay_);
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        std::snprintf(lines[4], sizeof(lines[4]), "Mem %.1f MB", pmc.WorkingSetSize / (1024.0 * 1024.0));
    } else {
        std::snprintf(lines[4], sizeof(lines[4]), "Mem n/a");
    }
#else
    std::snprintf(lines[4], sizeof(lines[4]), "Mem n/a");
#endif

    f32 w = kMinW;
    for (const auto& line : lines) {
        const Size s = ctx.measure_text(line, true);
        if (s.width > w) w = s.width;
    }
    w += kPadX * 2.0f;

    const f32 h = kPadY * 2.0f + kLineH * 5.0f - 3.0f;
    const f32 top = 8.0f;
    panel_ = RectF::make(bounds_.right - w - 8.0f, top, w, h);

    ctx.fill_rounded(panel_, backdrop_, 6.0f);
    ctx.draw_border(panel_, Color{0xFF, 0xFF, 0xFF, 40}, 1.0f, 6.0f);

    f32 y = panel_.top + kPadY;
    const RectF text_rect = RectF::make(panel_.left, panel_.top, panel_.width(), panel_.height());
    for (const auto& line : lines) {
        ctx.draw_text_small(line, RectF::make(text_rect.left + kPadX, y, text_rect.width() - kPadX * 2.0f, kLineH),
                            text_color_);
        y += kLineH;
    }

    ctx.set_offset(ox, oy);
}

void DebugOverlay::on_event(Event& e) {
    if (e.type == EventType::Timer) {
        e.consumed = true;
        poll();
        return;
    }
    Widget::on_event(e);
}

}  // namespace yzk