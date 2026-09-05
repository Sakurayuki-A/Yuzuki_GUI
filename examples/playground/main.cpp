#include "app.hpp"
#include <yuzuki/yuzuki.hpp>

int main() {
    auto& app = yzk::Application::instance();

    yzk::Window win("Playground", 1000, 700);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");
    win.backend().add_font_file("Phosphor.ttf");

    win.set_root(yzk::make_playground(win));
    win.show();

    return app.run();
}