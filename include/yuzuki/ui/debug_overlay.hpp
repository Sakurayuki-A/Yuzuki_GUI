#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class Window;

// Lightweight, always-visible-on-top statistics overlay for debugging any Yuzuki
// window. Shows FPS / frame time / layout / paint (record+replay) / memory. A
// general capability — attach it to any window with show()/toggle(), independent
// of examples.
//
// Self-positioned in the top-right corner (participates_in_layout() == false),
// so it never disturbs the widget tree below it. Samples Window::frame_stats()
// diffs on a 500 ms timer and, on Windows, reports the process working set via
// GetProcessMemoryInfo. Does not own children; paints its own text lines.
class DebugOverlay : public Widget {
public:
    DebugOverlay();
    ~DebugOverlay() override;

    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    void show(Window& win);
    void close();
    void toggle(Window& win) { is_open_ ? close() : show(win); }
    bool is_open() const { return is_open_; }

    // Half-transparent dark backdrop; text uses the current theme's foreground.
    void set_backdrop(const Color& color) { backdrop_ = color; invalidate(); }
    void set_text_color(const Color& color) { text_color_ = color; invalidate(); }

protected:
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    // Top-right overlay: parent layouts must not touch it.
    bool participates_in_layout() const override { return false; }

private:
    void poll();
    void maybe_start_timer();

    Window* win_ = nullptr;
    RectF panel_;
    bool is_open_ = false;
    f32 last_total_ = 0.0f;
    f32 last_layout_ = 0.0f;
    f32 last_record_ = 0.0f;
    f32 last_replay_ = 0.0f;
    f32 last_fps_ = -1.0f;
    u64 last_frames_ = 0;
    u64 last_cmds_ = 0;
    f64 last_poll_ms_ = 0.0;
    Color backdrop_ = Color{0x12, 0x12, 0x12, 200};
    Color text_color_ = Color{0xE6, 0xE6, 0xE6, 255};
};

}  // namespace yzk