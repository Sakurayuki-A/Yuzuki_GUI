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
    void quit(int exit_code);

    void add_window(Window* window);
    void remove_window(Window* window);

private:
    bool register_class();
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool pump_window(Window* window);

    void* instance_ = nullptr;
    bool class_registered_ = false;
    int exit_code_ = 0;
    bool quitting_ = false;
    WindowList windows_;
};

}  // namespace yzk