# App Shell（应用外壳）

> Phase 5.2.5 产物：icon、manifest（DPI / ComCtl 版本）、启动配置的规范入口。

每个 Yuzuki 桌面应用的骨架（外壳）由四部分组成：**窗口图标、DPI manifest、
代码启动序列**、以及**构建接线**。本页是规范入口：新应用从 `examples/app_shell`
复制起步。

## 1. 规范启动序列（代码入口）

```cpp
#include <yuzuki/yuzuki.hpp>
using namespace yzk;

int main() {
    auto& app = Application::instance();   // DPI awareness + 模块句柄
    Window window("My App", 520, 360);     // 逻辑尺寸（DIP）
    if (!window.create()) return 1;        // HWND + 渲染目标（逐显示器 DPI）
    window.backend().add_font_file("LexendDeca-Regular.ttf");
    window.set_root(make_root());          // 构建控件树
    window.show();
    return app.run();                      // 消息泵 + 按需渲染
}
```

`Application::instance()` 已强制 Per-Monitor V2 DPI 感知（`SetProcessDpiAwarenessContext`），
manifest 的存在保证进入应用前系统即按正确 DPI 布置。

## 2. manifest（DPI + ComCtl v6）

`resources/app-shell/app.manifest`：

- `dpiAwareness = PerMonitorV2`：每个显示器独立缩放，与 Window 创建期的逐显示器
  DPI 处理一致；
- 依赖 `Microsoft.Windows.Common-Controls` **v6**：对话框控件主题化（文件对话框等
  由 `IFileOpenDialog` 调起时也受益）;
- 通过链接器 `/MANIFESTINPUT` 与 MSVC 运行时 manifest 合并嵌入（同一 `RT_MANIFEST`
  id 1，无 RC/link 冲突）。

## 3. 图标

`resources/app-shell/yuzuki.ico`（32×32 BGRA，含 alpha）+ `app.rc` 以
`1 ICON "yuzuki.ico"` 嵌入（`RT_GROUP_ICON` id 1 = Explorer / 任务栏关联所需的最低位 id）。

## 4. 构建接线（CMake helper）

`cmake/yuzuki_app_shell.cmake` 提供 `yuzuki_add_app_shell(<target> [FONTS ...])`：

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE yuzuki)
yuzuki_add_app_shell(my_app FONTS icofonts/regular/Phosphor.ttf)
```

它完成：附加 `app.rc`（图标）→ 设置 `/MANIFESTINPUT:app.manifest` → 拷贝框架默认字体
（及可选字体）到 exe 旁。

## 5. 验证

`examples/app_shell` 构建后可用资源检查确认（PowerShell 示例见《验证基线》）：
`RT_GROUP_ICON`(14)/`RT_MANIFEST`(24) 均存在于 `id=1`，且 manifest 文本含
`PerMonitorV2` 与 `Common-Controls`。