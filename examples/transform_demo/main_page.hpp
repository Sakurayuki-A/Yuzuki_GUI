#pragma once
#include <yuzuki/yuzuki.hpp>

namespace demo {

yzk::Color bg();
yzk::Color card_bg();
yzk::Color text();
yzk::Color text_secondary();
yzk::Color accent();
yzk::Color green();
yzk::Color amber();
yzk::Color cyan();
yzk::Color pink();
yzk::Color red();

}  // namespace demo

yzk::Widget* make_transform_page(yzk::Window& win);