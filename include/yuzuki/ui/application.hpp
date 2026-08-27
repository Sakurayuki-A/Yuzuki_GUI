#pragma once
#include <yuzuki/ui/window.hpp>

namespace yzk {

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static Application& instance();

    int run();

    // Requests application exit with the given code; delivered via PostQuitMessage.
    // (Bundled demos exit by closing their last window instead — quit() itself has no
    // in-repo caller; it exists for embedding hosts that need an explicit exit code.)
    void quit(int exit_code);

    void add_window(Window* window);
    void remove_window(Window* window);

    // Forces a full repaint of every window (theme switch restyling).
    static void invalidate_all_windows();

private:
    void* instance_ = nullptr;
    int exit_code_ = 0;
    bool quitting_ = false;
    WindowList windows_;
    static Application* s_instance_;
};

}  // namespace yzk