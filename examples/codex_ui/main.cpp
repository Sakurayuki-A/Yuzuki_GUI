#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/application.hpp>

#include <windows.h>
#include <string>

#include "components.hpp"
#include "main_page.hpp"

using namespace yzk;

int main() {
    // Apply the Codex light theme (colors defined in theme.hpp).
    Theme t = Theme::get();
    t.background = codex::WindowBg;
    t.surface = codex::SidebarBg;
    t.text = codex::Text;
    t.border = codex::Border;
    t.accent = codex::Accent;
    Theme::set(t);

    Application& app = Application::instance();

    Window window("Codex UI", 1120, 760);
    if (!window.create()) return 1;

    {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
        window.backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
        window.backend().add_font_file(utf::to_utf8(dir + L"Phosphor.ttf"));
    }

    window.set_root(make_codex_page());
    window.show();
    return app.run();
}