#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/application.hpp>

#include <windows.h>
#include <string>

#include "main_page.hpp"

using namespace yzk;

int main() {
    Theme::set(Theme::make_dark());

    Application& app = Application::instance();

    Window window("Controls Demo", 1100, 780);
    if (!window.create()) return 1;

    {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
        window.backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
    }

    window.set_root(make_controls_page(window));
    window.show();
    return app.run();
}