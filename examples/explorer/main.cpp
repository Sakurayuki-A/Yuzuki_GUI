#include "app.hpp"
#include <yuzuki/yuzuki.hpp>

int main() {
    auto& app = yzk::Application::instance();

    yzk::Window win("Yuzuki Explorer", 980, 640);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    win.set_root(yzk::make_explorer(win));
    win.show();

    return app.run();
}