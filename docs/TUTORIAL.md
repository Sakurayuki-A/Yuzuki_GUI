# Yuzuki Tutorial

> 30 分钟完成第一个完整的小型 UI。

本教程按 [DX Roadmap](`# Yuzuki DX Roadmap — Closer to Electron.md`) 中的 Phase 1 路径编写。
每一步都可以编译运行。完整代码见 `examples/hello/main.cpp`。

## 前置条件

* Windows 10+
* Visual Studio 2022（含 CMake 支持）
* C++17

```bat
:: 首次配置
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\examples\Release\hello.exe
```

## 第 1 步 — 创建 Window

```cpp
#include <yuzuki/yuzuki.hpp>

using namespace yzk;

int main() {
    auto& app = Application::instance();
    Window win("Hello", 480, 320);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");  // 字体必须加载，否则文字不显示
    ...
    win.show();
    return app.run();
}
```

要点：

* `Application::instance()` 全局唯一。
* `Window` 的尺寸是 DIP。
* `add_font_file` 加载 UI 字体——示例的字体文件由 CMake `POST_BUILD` 复制到 exe 目录。

## 第 2 步 — 创建第一个 Widget

Yuzuki 是 retained-mode：你构建一棵 widget 树，框架负责度量、布局与绘制。

```cpp
auto* root = new DockPanel;
auto* column = new StackPanel(Orientation::Vertical);
column->set_spacing(12.0f).set_padding(24.0f);
column->set_stretch_children(true);
root->dock(column, Dock::Fill);
```

> 注意：`Layout` 是布局器基类（`measure_content` 为空实现），不直接作为根容器使用。
> 根容器用 `DockPanel`，子容器用 `StackPanel` / `FlexBox` 等。这一条若搞错，
> 窗口会只有背景色、看不到任何控件。

## 第 3 步 — 使用 Layout

`StackPanel` 是纵向/横向堆叠的布局器：

```cpp
auto* column = new StackPanel(Orientation::Vertical);
column->set_spacing(12.0f);   // 子项间距
column->set_padding(24.0f);   // 内边距
column->set_stretch_children(false);  // 子项按自身大小，不拉伸
```

框架内置 `StackPanel` / `FlexBox` / `GridPanel` / `WrapPanel` / `DockPanel` 等布局。

## 第 4 步 — 添加 Text

```cpp
auto* title = new Label("Hello, Yuzuki");
title->set_bold(true);
column->append_child(title);

auto* hint = new Label("Type something and press Go.");
hint->set_text_role(TextRole::Secondary);   // 语义色,跟随主题切换
column->append_child(hint);
```

## 第 5 步 — 添加 Button

```cpp
auto* go = new Button("Go");
column->append_child(go);
```

## 第 6 步 — 添加 Event

事件用回调注册(统一 `set_on_xxx(cb)`;`on_click`、`on_changed` 是等价短写法)。Button 是 `set_on_click`,输入框变化用 `set_on_changed`(Slider / SpinBox / ComboBox 统一)。

```cpp
auto* input = new TextBox(String(), TextBoxConfig{});
input->set_placeholder("Your name...");
column->append_child(input);

auto* echo = new Label("");
echo->set_text_role(TextRole::Secondary);
column->append_child(echo);

go->set_on_click([input, echo, &win]() {
    const String text = input->text();
    echo->set_text(text.empty() ? "Please type something." : "Hello, " + text + "!");
    win.invalidate_all();   // 文本变化后刷新
});
```

链式写法(所有 setter 返回 `*this`):

```cpp
column->add<Button>("Go").set_on_click([input, echo, &win]() {
    // ...
});
```

## 第 7 步 — 修改 Theme

主题是全局的，切换后调用 `invalidate_all` 重绘：

```cpp
bool dark = true;
auto* theme_toggle = new Button("Theme: Dark");
theme_toggle->set_accent(false).set_on_click([&win, &dark, theme_toggle]() {
    dark = !dark;
    Theme::set(dark ? Theme::make_dark() : Theme::make_light());
    win.invalidate_all();
    theme_toggle->set_text(dark ? "Theme: Dark" : "Theme: Light");
});
```

## 第 8 步 — 完成一个小型页面

组合前面所有部分，完整代码见 `examples/hello/main.cpp`。运行：

```bat
build\examples\Release\hello.exe
```

## 之后去哪

* 浏览 `examples/playground`——每个控件、布局与事件的交互式演练场。
* 看 `examples/codex_ui`——一个用框架构建的完整聊天 UI。
* 想查某个 API 的具体签名 → [API.md](API.md)(按任务组织的速查表)。