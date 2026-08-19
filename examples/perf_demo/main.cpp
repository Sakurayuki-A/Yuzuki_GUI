#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/application.hpp>

#include <windows.h>
#include <cstdlib>
#include <string>

#include "main_page.hpp"

using namespace yzk;

int main(int argc, char** argv) {
    Theme::set(Theme::make_dark());

    Application& app = Application::instance();

    Window window("Performance Demo", 1280, 800);
    if (!window.create()) return 1;

    {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
        window.backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
    }

    int scenario = -1;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--scenario") scenario = std::atoi(argv[i + 1]);
    }

    window.set_root(make_perf_demo_page(window, scenario));
    window.show();
    return app.run();
}