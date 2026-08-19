#include <yuzuki/ui/application.hpp>

#include <windows.h>
#include <mmsystem.h>

namespace yzk {

Application::Application() {
    instance_ = GetModuleHandleW(nullptr);
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware();
    }
}

Application::~Application() = default;

Application& Application::instance() {
    static Application app;
    return app;
}

int Application::run() {
    // Raise system timer resolution to 1ms for precise animation timers
    timeBeginPeriod(1);

    MSG msg{};
    // Cap messages per loop so rendering always runs during input floods
    // (frenzied mouse movement) and animations are never starved by the message queue
    constexpr int kMaxMessagesPerFrame = 32;
    for (;;) {
        int drained = 0;
        bool processed = false;
        while (drained < kMaxMessagesPerFrame && PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                timeEndPeriod(1);
                return exit_code_;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            ++drained;
            processed = true;
        }
        // Render on demand once per loop after draining: end_frame's Present blocks on
        // vsync, so the cadence follows the refresh rate. Input floods can't cause
        // render storms (one render per loop) or starve animations (tweens sample
        // real time inside pump).
        bool rendered = false;
        for (Window* window : windows_) {
            if (window->needs_paint()) rendered |= window->pump();
        }
        // No messages and nothing to render: sleep until a message arrives, don't spin CPU
        if (!processed && !rendered) WaitMessage();
    }
}

void Application::quit(int exit_code) {
    exit_code_ = exit_code;
    PostQuitMessage(exit_code);
}

void Application::add_window(Window* window) {
    windows_.push_back(window);
}

void Application::remove_window(Window* window) {
    for (size_t i = 0; i < windows_.size(); ++i) {
        if (windows_[i] == window) {
            windows_.erase(windows_.begin() + static_cast<ptrdiff_t>(i));
            break;
        }
    }
    if (windows_.empty() && !quitting_) {
        quitting_ = true;
        PostQuitMessage(exit_code_);
    }
}

}  // namespace yzk
