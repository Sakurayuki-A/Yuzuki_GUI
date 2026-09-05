# Yuzuki DX Roadmap — Closer to Electron

## 0. 进度状态

> 执行中的阶段与完成情况。每条原则「先完成前置阶段，再开始下一阶段」。

| 阶段 | 状态 | 备注 |
| --- | --- | --- |
| **Phase 0 — Consistency Cleanup** | ✅ 完成 | 见下方「# 3 Current DX Problems」内 `### 完成状态` |
| **Phase 1 — On-ramp** | ✅ 完成 | `examples/hello` + `docs/TUTORIAL.md` + README/DEVELOPMENT 同步 |
| **Phase 2 — Design System** | ✅ 完成 | 2.1 Geometry(高度/圆角/border/scrollbar/wheel token + 8 控件迁移);2.2 Typography 语义 token + Label measure 修复;2.4 Elevation(floating/notice shadow token);2.3 Surface(border 宽度统一 token);2.5 Interaction States(flags 契约文档化,合成/中心双路径) |
| Phase 3 — Developer Tools | ✅ 完成 | DebugOverlay(F1:FPS/Frame/Layout/Paint/Memory 悬浮统计);WidgetInspector(F2:widget 树 + 选中属性 + 本体描边);均由处理为通用能力,debug_demo 演示 |
| Phase 4 — API Reference + Cookbook | 🟢 完成 | API.md 完成并通过"照写验证"(Phase 4.1);Cookbook 按"不硬凑 8 页、只做有代表性的"原则推进:Settings(第 1 页 + examples/cookbook_settings)、Login(第 2 页 + examples/cookbook_login)、Chat UI(第 3 页 + examples/cookbook_chat,验证多行输入/Enter 提交/Ctrl+Enter 换行/自动滚底/Send 内嵌工具栏居中,截图目验通过) |
| Phase 4.1 — API Reference | ✅ 完成(验证通过) | docs/API.md(任务导向 10 节,非 header dump)+ examples/api_validate 逐节照写编译运行,截图目验通过 |
| Phase 5 — Framework Maturity | 🟢 主体完成（2026-09-05 盘点） | 5.1 视觉回归+设备丢失钩子、5.2 平台能力四件套+App Shell、5.3 文本深度、5.4 TabControl 已落地；余项：install/export、长跑审计、UIA 扩展，见「### Phase 5 已完成项」 |
| Phase 6 — Hot Reload | ⚪ 未开始 | 以 Phase 5 验收 + API 冻结窗口为入场门槛，见「# Phase 6 — Hot Reload」章节 |

### Phase 0 已完成项

* ✅ 版本号统一为 **0.3.0**（`yuzuki.hpp` / `CMakeLists.txt` / `DEVELOPMENT.md` / README）
* ✅ Value-change callback 统一为 **`on_changed`**（Slider / SpinBox / ComboBox）
* ✅ Button 重复 `min_width` 消除，单一数据源 `Widget::min_size_`
* ✅ README / DEVELOPMENT 删除已失效 example 引用，示例表与磁盘 9 个目录对齐
* ✅ 构建命令指向实际存在的 `playground.exe`
* ✅ Box 补齐 `border_width` / `border_color` / `border` fluent 别名
* ✅ API 命名一致性盘点并入库（见 P0 段两处 `<details>`）

### 验证基线

每阶段完成均需 `cmake --build` 全绿 + `yuzuki_tests.exe` = **548 checks, 0 failures** + `yuzuki_visual_tests.exe` = **93 checks, 0 failures**（24 条视觉基线，`tests/test_visual.cpp`），且本阶段不引入新的性能/内存退化。

### Phase 5 已完成项（2026-09-05 盘点）

* ✅ **5.1（部分）**：`tests/test_visual.cpp` 视觉回归上线——离屏渲染 → WIC PNG → FNV-1a 像素哈希 → 24 条基线比对，独立 ctest target；配套 `Window::set_capture/capture_pixels` 捕获链路与 `RenderBackend::recreate_after_loss()` 设备丢失恢复测试钩子
* ✅ **5.2**：`clipboard::set_text/get_text/has_text` 框架 API（自 TextBox 私有实现提升）；`MessageBox`（框架渲染非阻塞模态，Info/Confirm/YesNo/YesNoCancel，Esc/遮罩语义）+ 原生 `open_file_dialog/save_file_dialog`（过滤器/多选/UTF-8）；窗口级快捷键表 `Window::add_accelerator`（编辑键不劫持规则）；二级窗口所有权 `set_owner/set_modal/set_topmost/loop_until_closed`；App Shell（`yuzuki_add_app_shell()`：图标 + PerMonitorV2/ComCtl-v6 manifest 合并 + 字体拷贝，规范见 docs/APP_SHELL.md）
* ✅ **5.3**：Label RichText lite（`**bold**` / `~highlight~` / `[link](action)` 可点击 span + hover 跟踪）；TextBox undo/redo（连续输入合并）、词边界导航（Ctrl+Left/Right/Backspace/Delete）、多行固定高度视口滚动、`enter_submits` / `transparent` 配置
* ✅ **5.4**：TabControl（页托管、Left/Right 键盘导航、on_changed）
* ✅ 测试规模：213 → **548 core + 93 visual**（新增 5.1.2 补充覆盖、5.2.1–5.2.4 分组测试）
* ⏸ 余项：5.1 长跑审计与恢复路径自动化扩展、5.5 CMake install/export（`find_package(yuzuki)`）、UIA 超出 Name/Role 的扩展

---

## 1. Goal

Yuzuki 的长期定位：

> **Electron-like developer experience, Native GUI resource efficiency.**

中文：

> **接近 Electron 的开发体验，保持 Native GUI 的资源占用。**

Yuzuki 不试图复制 Electron 的技术栈，也不引入 Chromium、JavaScript Runtime 或 HTML/CSS 解析体系。

DX 的目标是借鉴 Electron/Web 开发体验中最有价值的部分：

* 低学习成本
* 快速获得第一个可运行结果
* API 容易发现和理解
* 默认行为合理
* 默认 UI 具有一致的设计语言
* 常见需求不需要接触底层
* 高级能力仍然可以逐层深入
* 开发者能够快速修改 UI，而不需要理解 Renderer 内部实现

同时保持 Yuzuki 的核心技术优势：

* Native Window
* C++17
* Direct2D
* 低内存占用
* 快速启动
* 小运行时
* 不依赖 Chromium

---

# 2. Current DX Assessment

当前 Yuzuki 已经具备一定的 DX 基础。

### Existing strengths

目前已经存在较好的 Builder / Fluent API。

当前最接近理想形态的写法：

```cpp
row->add<Button>("Go").on_click(fn).width(120)
```

`box() / .children() / .gap()` 式的声明式容器入口是 Phase 2 的目标形态，尚未实现。

Widget Tree、Layout、Event、Theme 等基础抽象已经存在，因此 Yuzuki 并不是从零开始建立 DX。

当前最大的问题不是“缺少 GUI 基础能力”，而是：

> **已有能力没有被统一成一个稳定、容易学习的开发模型。**

---

# 3. Current DX Problems

## P0 — API Consistency

目前存在 API 命名和使用方式不统一的问题。

例如：

* `on_changed`
* `on_value_changed`
* `on_change`

存在多种 callback 命名方式。

部分 Widget 提供 fluent alias，而其他 Widget 没有。

<details>
<summary>Fluent alias 缺口清单（Phase 0 现状入库，2026-08-28 核查；2026-09-04 裁决关闭：由 API.md canonical 政策取代，不再补齐）</summary>

> 核对对象：每个控件的 `X& set_foo(...)` 是否有对应的无前缀 fluent `X& foo(...)`。
> 已完成补齐：Box 的 `border_width` / `border_color` / `border`（2026-08-28）。
> 尚未补齐（后续阶段）：Button(`text`/`icon_size`/`accent`)、ComboBox(`items`/`selected_index`/`placeholder`)、Slider(`range`/`value`)、SpinBox(`value`/`range`/`step`/`decimals`/`spin_width`)、ProgressBar(`value`/`indeterminate`)、ScrollView(`content`/`scroll_y`/`suggested_height`)、ListView(`items`/`data_source`/`row_delegate`/`selected`/`hovered`/`row_height`/`scroll_y`/`show_border`/`show_scrollbar`)、TextBox(`placeholder`/`config`/`read_only`/`content_inset`)、StackPanel(`orientation`/`stretch_children`/`fill`)、FlexBox(`direction`/`align_main`/`align_cross`)、GridPanel(`column_auto`/`column_star`/`row_auto`/`row_star`/`column_fixed`/`row_fixed`)、WrapPanel(`line_spacing`)、Image(`bitmap`/`scale_mode`/`corner_radius`)、BackdropBlur(`blur`/`tint`/`corner_radius`)、Box(`shadow_color`)。
> 注意：Tooltip / Notification / SpinBox::step 等 setter 返回 `void` 或 getter 已是无参形式，fluent 需要变更为返回 `*this` 的重载，属行为变更——2026-09-04 裁决后不再执行：补齐同名 alias 与 API.md「set_xxx 唯一 canonical」政策冲突，本清单整体关闭，仅作历史记录保留。

</details>

Button 存在与 Widget 层级重复的 `min_width` 相关 API。

这些问题会直接增加 API 学习成本。

目标：

> 同一个概念只能有一种推荐写法。

### 完成状态（Phase 0，2026-08-28）

* ✅ Value-change 类 callback 统一为 `on_changed`：Slider / SpinBox / ComboBox 三处旧命名（`on_change` / `on_value_changed`）已全部收敛，旧名无残留。
* ✅ Button 重复 `min_width` 已消除：单一数据源 `Widget::min_size_`，`Button::set_min_width` 保留为协变链式别名。
* ✅ 版本号统一为 0.3.0（yuzuki.hpp / CMakeLists / DEVELOPMENT.md），README 已同步。
* ✅ README / DEVELOPMENT 中已删除 example（hello / controls_demo / transform_demo / notification_demo）的引用已清空，示例表与 9 个实际目录对齐。
* ✅ **API 统一阶段执行（2026-08-30）**：上述清单 1/2/4 全部落地、清单 3 部分落地（`set_transition` 统一返回引用；Slider/SpinBox 类型差与 GridPanel 后缀为现状，勾选保留）；全量 17 target 构建通过 + 212/0 回归通过。
* ✅ fluents alias 缺口：**已裁决关闭（2026-09-04）**——采纳 API.md 政策：`set_xxx()` 为全库唯一 canonical 写入口；现存同名 alias（`width()`/`margin()` 等）保留不删、不再新增；Phase 0 入库的补齐清单由该政策取代（见下）。

<details>
<summary>Public API 命名一致性缺口（Phase 0 现状入库，2026-08-28 核查）</summary>

> 后续 Phase（建议 "API 统一" 阶段）处理的命名/签名不一致清单，均为破坏性变更。已按 2026-08-30 盘点处理，✅ = 已统一，剩余为本次未动项：

1. **Setter 不返回 `*this`**：✅ `Window` 全部属性 setter（`set_root`/`set_borderless`/`set_focus`/`set_context_menu`/`set_caption`）统一返回 `Window&`；✅ `Icon::set_phosphor_font_file`/`set_provider` 统一返回 `IconProvider&`；✅ `context_menu::clear_items`（及 `add_item`/`add_separator`）返回 `ContextMenu&`，与 `combo_box`/`list_view` 的 `X&` 对齐；✅ `animation.hpp` 的 `set_transition(f32)` 返回 `AnimatablePropertyBase&`，与 `widget.hpp` 的 `Widget&` 对齐。
2. **Getter 名与 setter 不对应**：✅ `FlexBox` 改名为 `align_main()`/`align_cross()`（对应 `set_align_main`/`set_align_cross`）；✅ `Window::set_focus` ↔ `focus()`；✅ `Widget::set_scale` 补 `scale()`（返回 `Point`，`scale_x()/scale_y()` 保留）；✅ SpinBox `minimum()/maximum()` → `min()/max()`，与 Slider 统一。
3. **同名 setter 参数语义/单位不一致**：⏸ `Slider::set_value/set_range`(f32,有界) vs `SpinBox::set_value/set_range`(f64) 保留类型差；⏸ `GridPanel` 的 `set_column_*`/`set_row_*` 保留 `_auto/_star/_fixed` 显式后缀；✅ `Widget::set_transition` 与 `AnimatablePropertyBase::set_transition` 现均返回引用，行为一致（前者广播到各子属性）。
4. **setter 无对应 getter**：✅ `Label::set_align` 补 `align_h()`/`align_v()`；✅ `Overlay::set_dim/set_dim_blurred/set_shadow` 补 `dim()`/`dim_blurred()`/`shadow()`；✅ `Box::set_shadow_color` 补 `shadow_color()`；✅ `Slider` 补单项 `set_min()`/`set_max()`（委托 `set_range`，复用裁剪语义）。
5. **配置结构全公开字段**：⏸ `TextBoxConfig`（`mode`/`read_only`/`max_length`/`height`/`min_lines`/`max_lines`/`enter_submits`/`transparent`）、`ContextMenuItem`、`GridLength`/`GridSlot`、`Theme`（全字段公开，作为全局快照对此尚可辩护）；`Margins` 是 POD 值类型且提供 horizontal()/vertical()，最轻。

</details>

---

## P0 — Documentation / On-ramp

当前缺少完整的新手路径：

* 缺少稳定的 hello example
* 缺少系统 Tutorial
* 缺少 API Reference
* 缺少 Cookbook
* README 存在已删除 example 的旧引用
* README 示例存在字体加载等实际运行所需步骤缺失
* 文档中的版本号存在不一致

因此当前新人无法通过一条清晰路径完成：

```text
Install
  ↓
First Window
  ↓
Layout
  ↓
Widget
  ↓
Event
  ↓
Theme
  ↓
Small Application
```

目标：

> 新开发者无需阅读框架源码，在 30 分钟内完成第一个具有布局、交互和主题的小型应用。

---

# 4. P0 — Design System

这是当前距离 Electron/Web 开发体验最大的差距之一。

目前：

> Theme 更接近 Color Table，而不是完整 Design System。

同时：

* Look-as-a-property 主要存在于 Box
* 大量 Widget 仍然在 `paint_impl` 中直接决定视觉参数
* 控件存在独立的 magic numbers
* 控件之间缺少统一 spacing
* Typography 缺少语义层级
* Surface / Elevation 不完整
* Interaction states 不统一

典型问题包括：

1. 控件高度、圆角、padding 偏机械
2. Border 太明显
3. 阴影 / 层级关系弱
4. Typography 没有真正建立层级
5. Icon 与文字视觉重量不一致
6. Hover / Pressed / Focus 状态过于简单
7. Surface 层次不足
8. List、Button、Input 像独立控件拼接，而不是统一设计系统
9. Theme 更像颜色表，而不是完整 Design System

目标：

> Widget 不应该自己决定完整的视觉语言。Widget 应该消费 Yuzuki Design System。

---

# 5. DX Principles

后续所有 DX 工作必须遵循以下原则。

## 5.1 Simple by Default

常见需求应该简单。

例如：

```cpp
ui::button("Save")
```

应该能够直接得到一个完整、合理、有状态反馈的 Button。

不应该要求开发者首先配置：

* background
* border
* radius
* hover
* pressed
* typography
* padding

---

## 5.2 Progressive Disclosure

Yuzuki 的 API 分为不同层次。

### Beginner

只接触：

```text
Window
Container
Text
Button
Input
Image
Layout
Event
Theme
```

### Intermediate

可以使用：

```text
Style
Animation
Custom Layout
Custom Widget
Theme Tokens
```

### Advanced

才需要接触：

```text
PaintContext
Renderer
Dirty Region
Coordinate System
Layout Internals
Widget Lifecycle
```

高级能力不能污染新手 API。

---

## 5.3 No Magic Numbers in Components

Widget 不应该大量硬编码：

```text
80
10
16
32
160
...
```

这些数值应该逐步迁移到 Design Tokens。

例如：

```text
Spacing
Radius
Control Height
Typography
Elevation
```

Widget 应该消费语义化 Token，而不是自行创造视觉规范。

---

## 5.4 No CSS Clone

Yuzuki 不以实现完整 CSS 为目标。

第一阶段禁止因为“接近 Web”而引入：

* CSS parser
* Selector engine
* Cascade engine
* CSS specificity
* HTML parser
* DSL parser

目标是：

> **复制 Web 的开发体验，而不是复制 Web 的实现。**

---

## 5.5 No Premature DSL

当前阶段不重新引入 UI DSL / Markup Parser。

C++ Builder / Fluent API 足以作为主要开发入口。

只有当实际 API 已经稳定，并且确认 DSL 能显著降低学习成本时，才重新评估 DSL。

---

## 5.6 Defaults Are a Feature

默认值不是附加功能。

Yuzuki 应该主动提供：

* 合理的 spacing
* 合理的 radius
* 合理的 control height
* 合理的 typography
* 合理的 surface
* 合理的 interaction states
* 合理的 icon sizing
* 合理的 elevation

开发者应该能够“不配置”也得到统一的 UI。

---

# 6. DX Roadmap

## Phase 0 — Consistency Cleanup

目标：

> 清理现有 API 和文档，使 Yuzuki 获得一个干净的 DX baseline。

内容：

1. 统一版本号
2. 修复 README / DEVELOPMENT 中已经删除的 example
3. README 构建步骤统一指向当前有效 playground
4. 统一 callback naming
5. 清理 Button / Widget 重复 API
6. 补齐缺失的 fluent aliases
7. 检查 public API 命名一致性
8. 清理明显已经失效的 DX 文档

约束：

> Phase 0 不进行架构重构。

禁止：

* Style 系统重写
* Renderer 重构
* Layout 重构
* DSL
* 新 Widget 大规模增加

完成标准：

* Public API 命名一致
* README 可以从零构建当前项目
* 文档中的 example 均可验证
* 新开发者不会因为 API 命名差异产生困惑

---

# Phase 1 — On-ramp

目标：

> 新开发者 30 分钟内完成第一个完整的小型 UI。

建立：

```text
examples/hello
docs/TUTORIAL.md
```

Tutorial 路径：

```text
1. 创建 Window
2. 创建第一个 Widget
3. 使用 Layout
4. 添加 Text
5. 添加 Button
6. 添加 Event
7. 修改 Theme
8. 完成一个小型页面
```

Hello example 必须：

* 可以直接编译
* 包含真实运行所需的字体初始化
* 不依赖 playground 中的大量隐藏代码
* README 与 example 保持同步

30-minute test：

一个完全不了解 Yuzuki 的 C++ 开发者应该能够完成：

```text
Window
 └── Column
      ├── Title
      ├── Description
      ├── Button
      └── Input
```

并实现至少一个交互。

---

# Phase 2 — Design System / Look as Property

目标：

> **开发者修改 Widget 外观时，不需要实现或修改 `paint_impl`。**

建立统一的 Style / Look abstraction。

第一阶段至少覆盖：

```text
Fill
Text Color
Border
Radius
Shadow
Color Role
Spacing
Typography
Control Height
```

Style 必须能够被 Widget 消费。

Widget 默认视觉参数逐步从：

```text
hard-coded values
```

迁移到：

```text
Design Tokens
```

---

## 2.1 Geometry Tokens

建立统一尺度，例如：

```text
Spacing:
4
8
12
16
24
32
48
```

Radius：

```text
4
8
12
16
Pill
```

Control Height：

```text
32
40
48
```

实际 token 数值不是本阶段唯一重点。

重点是：

> 整个 Design System 必须共享有限、稳定、可预测的尺度。

---

## 2.2 Typography

建立语义化 Typography：

```text
Display
Headline
Title
Body
Label
Caption
```

Widget 不应该到处直接指定：

```text
font-size: 17
font-weight: 500
```

而应该消费 Typography Role。

---

## 2.3 Surface

建立 Surface 层级：

```text
Background
Surface
Surface Container Low
Surface Container
Surface Container High
Elevated Surface
Overlay
```

减少对明显 Border 的依赖。

---

## 2.4 Elevation

建立统一的 elevation / shadow language。

控件不应该各自决定：

```text
shadow blur = 13
shadow offset = 4
```

而应该使用语义化层级。

---

## 2.5 Interaction States

统一：

```text
Normal
Hover
Pressed
Focused
Disabled
```

不同 Widget 应遵循同一套状态模型。

---

# Phase 3 — Developer Tools

目标：

> 降低调试 UI 的成本。

建立通用 Debug Overlay。

至少包含：

```text
FPS
Frame Time
Layout Time
Paint Time
Memory
```

并提供 Widget Tree Inspector：

```text
Window
└── Column
    ├── Text
    ├── Button
    └── TextBox
```

选择 Widget 后显示：

```text
Bounds
Padding
Layout
Style
State
Parent
Children
```

Debug 工具必须是通用 Yuzuki 能力，而不是只存在于 perf demo。

---

## Hot Reload

Hot Reload 自 2026-09-04 起正式排期为 **Phase 6**，排在新增的 Phase 5（Framework Maturity）之后，以 Phase 5 验收 + API 冻结窗口为入场门槛（详见对应章节）。

优先级序列：

1. API consistency ✅
2. On-ramp ✅
3. Design System ✅
4. Debug Tools ✅
5. Framework Maturity（Phase 5，下一主阶段）
6. Hot Reload（Phase 6，终点站）

---

# Phase 4 — API Reference + Cookbook

目标：

> 让开发者能够从“会用 Yuzuki”进入“能够独立解决问题”。

建立 API Reference。

同时提供 Cookbook：

```text
Login Page
Settings Page
Chat UI
File List
Dashboard
Dialog
Navigation
Form
```

Cookbook 应强调：

> 如何用 Yuzuki 的 Design System 和 Layout 组合实际应用。

而不是单纯展示 API。

---

# Phase 5 — Framework Maturity（框架成熟计划）

> 立项：2026-09-04。Phase 0–4.1 完成了 DX 主线；本阶段回答下一个问题：
>
> **一个开发者能否用 Yuzuki 独立交付一个真实（非玩具）应用，而中途不需要掉出框架回到裸 Win32？**
>
> Hot Reload 移至 Phase 6，以本阶段完成为开工门槛。

## 5.0 为什么 Hot Reload 要等框架成熟

1. 热重载机制绑定 widget 树重建与模块生命周期——public API 高频破坏性变更时，热重载自身会持续返工；
2. 能力缺口（文件对话框 / 快捷键 / 剪贴板 API）存在时，热重载只是更快地撞到「掉出框架」的悬崖；
3. 没有视觉回归基线时，热重载引入的渲染偏差无法被自动察觉。

顺序：**先补能力缺口 → 冻结 API → 再做热重载。**

## 5.1 Core Hardening — 内核底层加固（第一优先）

> 目标：框架引擎本身先于一切表层能力成熟——渲染、资源、恢复路径、性能全部有回归保护。
> 表层能力（对话框 / 控件 / 文本）都建在内核之上；内核不稳，上层铺得越多返工越大。

内容：

1. **视觉回归测试**：离屏渲染关键页面快照 + 哈希 / 像素对比，进 ctest——后续一切改动的安全网
2. **长跑审计**：demo 连续运行 ≥1h，句柄 / 内存平稳（Phase 3 的 DebugOverlay 就是测量工具）
3. **恢复路径自动化**：设备丢失、DPI 切换、多窗口、IME 场景的回归测试
4. **布局性能重访**：full-tree measure/layout 的 dirty-tracking 评估，触发判据沿用 DEVELOPMENT.md 既定条款（`FrameStats::layout_ms` 平均 >1.5ms 且 >2000 可见控件才动手，否则不动）
5. **资源生命周期**：BitmapId unload 后的悬空引用审计、字体 / 渐变 / 阴影缓存的容量上限行为
6. **Accessibility 最小集**：UIA Name / Role（`PaintCommand::source` 已为此预留）——屏幕阅读器可读 Button / Label
7. 测试规模：212 → **400+ checks**

约束：只加固不重写（harden, not rewrite）；性能优化必须先测量后动手，测量工具优先沉淀为 DebugOverlay 能力。

完成标准：

- ctest 中出现视觉回归套件；长跑审计报告入库 docs；
- 全部现有 demo 在恢复路径测试后无崩溃、无泄漏回归。

## 5.2 Platform Capabilities — 平台能力补全

> 目标：真实应用的完整流程（打开文件 → 编辑 → 复制粘贴 → Ctrl+S 保存 → 退出确认）**零 Win32 代码**。

现状核查（2026-09-05）：Phase 5.2 全部完成（Clipboard / Dialogs / Keyboard accelerators / Secondary windows / App shell）。

内容：

1. **Clipboard** ~~把 text_box.cpp 内部实现提升为框架级公开 API~~ **已完成（2026-09-05）**：`yzk::clipboard::{has_text,set_text,get_text}`（UTF-8）；TextBox / ListView 已统一走框架 API，Ctrl+C/X/V/A 行为一致
2. **Dialogs** ~~基于 Overlay 的 MessageBox…~~ **已完成（2026-09-05）**：Overlay 上的 MessageBox（信息 / 确认 / 三选，Enter=Esc/backdrop 兜底，非阻塞回调，可关动画）；原生 `IFileOpenDialog` / `IFileSaveDialog` 封装（多选、过滤器）；新增 cookbook_dialogs 页演示完整流程。*目验修复（同日）：栈上 fire-and-forget 的 MessageBox 命中悬空 tween（自持有，on_close 释放）；面板居中后子元素偏移对齐面板；MessageText 自绘改用自身 bounds（原写死 0,0 致字体落在窗口左上）*
3. **Keyboard accelerators** **已完成（2026-09-05）**：窗口级 `Accelerator` 表（`add_accelerator` / `remove_accelerator`），WM_KEYDOWN 在普通分发前查表触发；与 focus 协调——文本输入聚焦时纯编辑键 / Ctrl+Z/X/C/V/A / 词导航等自动放行给控件，应用型组合键（Ctrl+S/O 等）照常触发；cookbook_dialogs 已用 Ctrl+S / Ctrl+O 演示
4. **Secondary windows** **已完成（2026-09-05）**：`set_owner` / `set_modal` / `set_topmost`（先行设置或创建后均可）；owner 绑定（随 owner 最小化/置顶）、模态禁用 owner（`EnableWindow`）、`loop_until_closed()` 嵌套消息循环；destroy 时自动恢复 owner；window_demo 二级窗口已改为 owned+modal 演示
5. **应用外壳** **已完成（2026-09-05）**：`resources/app-shell`（`app.rc` 图标 id 1 + `app.manifest` PerMonitorV2 / ComCtl v6）+ `cmake/yuzuki_app_shell.cmake`（图标 / manifest 合并 / 字体自拷贝）+ `examples/app_shell` 模板 + `docs/APP_SHELL.md` 规范启动序列

约束：只包装系统 API，不引第三方；每项能力必须有可运行用例（debug_demo 或新 cookbook 页）。

完成标准：

- 新 cookbook 页「Dialogs」演示上述完整流程；— **已完成（2026-09-05）**：`cookbook_dialogs`（打开 → 编辑 → 复制粘贴 → 保存 → 退出确认，全框架 API 零 Win32）
- `explorer` 升级为使用原生文件对话框；— **已完成（2026-09-05）**：路径栏加 Browse 按钮（`open_file_dialog` 封装）
- 回归：核心 505 checks / 视觉 76 checks（含 MessageBox 渲染基线；5.2.3/5.2.4 测试已并入）；app_shell 资源检查（RT_MANIFEST id 1 含 PerMonitorV2 + ComCtl，RT_GROUP_ICON id 1）通过

## 5.3 Text & Editing Depth — 文本能力加深

1. TextBox：undo/redo 栈、词级导航 / 选择、Home/End 语义（**已完成 2026-09-05**：undo/redo 快照栈 1000 上限，连打/连删 500ms 内合并为一步，redo 由新编辑作废；Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y；Ctrl+左右词级跳转 / Ctrl+Shift 扩展选择 / Ctrl+Backspace·Ctrl+Delete 词级删除；Ctrl+Home/End 文首文尾；既有 Home/End 行语义保留。测试 +5 组）
2. RichText lite：Label / Notification 支持一段文本内多样式 span（bold / color / link）（**已完成 2026-09-05**：`Label::set_rich_text` + `on_span_click`。标记 `**bold**`、`~highlight~`（accent 色）、`[label](action)`（accent 色 + 下划线，可点击；悬停手型，MouseDown 触发回调带 action）；跨字体段测量/绘制/对齐一致；测试 +1 组 + 视觉基线 `label_rich_spans`）
3. 在既有 d2d_text 命中测试基础上补回归（**已完成 2026-09-05**：+`test_d2d_text_hit_test`（视觉测试，真实后端）——caret→hit 往返 ±1 cluster、wrap 尾部 caret 落次行且命中闭环、首行 caret x 单调）

完成标准：cookbook_chat 升级——消息支持链接与高亮 span，输入框支持 undo。（**已完成 2026-09-05**：cookbook_chat 顶栏/欢迎消息演示 **bold / ~highlight~ / link 点击回调回显；输入框经 5.3.1 支持 Ctrl+Z 撤销 + 词导航**；回归：核心 **548 checks** / 视觉 **93 checks（24 基线，含 5.4 tab_control）**；17 个示例全部启动冒烟通过）

## 5.4 Control Set Completion — 克制的控件补全

原则先于清单：

> 控件数量不是指标。每个新增控件必须同时满足：消费 design token、有 cookbook 用例、有示例使用。三者缺一，不做。

清单（按刚需频率排序）：

1. **TabControl** — 设置页 / 多面板刚需（**已完成 2026-09-05**：`TabControl::add_tab(title, page)` + `set_selected_index` + `set_on_changed`；标签条（surface 底 + border 分隔线 + accent 选中下划线，高=control_height）+ Fill 内容区，仅活动页可见；←→ 键循环切换（focusable，Header MouseDown 亦切换）；`remove_tab`/`clear_tabs` 负责子控件内存；uia role "tab control"/"tab"。三原则自检：消费 token✓（surface/border/accent/control_height/radius_sm/control_radius）、cookbook 用例✓（cookbook_settings 改成 4 分页 TabControl）、示例使用✓（同）。测试：核心 +22 checks → 548、视觉 +5 checks `tab_control` → 93（24 基线））
2. **TreeView** — 虚拟化，复用 ListView 的 DataSource 模式
3. **Splitter** — 编辑器布局刚需，explorer 可直接受益
4. **MenuBar / Toolbar / StatusBar** — 桌面应用骨架，与 5.1 快捷键表联动
5. **Dialog 模板** — 标题 + 内容 + 按钮区，基于 5.1 的模态窗口

不做（留给用户用现有控件组合）：DatePicker、ColorPicker、图表控件。

## 5.5 Distribution — 分发

1. CMake install / export，`find_package(yuzuki)` 可用（现状核查：CMakeLists 无 install 规则）
2. 应用模板：图标 + manifest + 字体打包 + CMake 骨架，clone 即起步
3. 版本政策文档：什么变更算破坏性、迁移说明格式
4. （可选）vcpkg port

## Phase 5 总体验收

- [ ] 5.1 全部完成（硬性——内核加固是其余一切的地基）
- [x] 5.2 全部完成（2026-09-05：Clipboard / Dialogs / Accelerators / Secondary windows / App shell）
- [ ] 5.3 / 5.4 各至少 2 项落地，其余有明确「不做」裁决记录
- [ ] 5.1 视觉回归上线 + 400 checks
- [ ] 5.5 `find_package(yuzuki)` 可用
- [ ] 每阶段收尾：cmake 全绿 + 回归 0 失败 + 无性能退化（沿用现有验证基线条款）

---

# Phase 6 — Hot Reload（终点站，有门槛）

入场门槛（全部满足才开工）：

1. Phase 5 验收通过
2. **API 冻结窗口**：连续 4 周无 public API 破坏性变更
3. 视觉回归基线稳定：连续两周无未解释 diff
4. 方案预研完成并三选一论证：
   - UI 层编译为 DLL，host 进程 watch + 卸载重载（Live++ / CR 风格）
   - 树状态序列化快照 + 进程内重建
   - watch + 增量重链 + 自动重启恢复

不变约束：**不引入 JS runtime / 脚本语言来换热重载**——那是背离 Electron 目标（复制体验而非实现），不是靠近。

---

# 7. Success Metrics

Yuzuki DX 不以“API 数量”作为主要指标。

采用以下指标：

## Time to First Window

从安装 / 获取源码到第一个窗口。

目标：

> < 10 minutes

---

## Time to First Interactive UI

完成：

```text
Window
Layout
Text
Button
Event
```

目标：

> < 20 minutes

---

## Time to First Themed UI

完成：

```text
Typography
Spacing
Surface
Theme
Interaction State
```

目标：

> < 30 minutes

---

## Framework Concepts Required

新人完成第一个 UI 时，不应该需要理解：

```text
Renderer
PaintContext
Dirty Region
Coordinate Space
Widget Lifecycle
```

这些应该属于 Progressive Disclosure 的高级层。

---

## Lines of Code

比较典型 UI 在 Yuzuki 中所需要的代码量。

目标：

> 常见 UI 不应该因为框架本身而产生大量 boilerplate。

---

# 8. Definition of Done

DX Roadmap 不以“代码合并”作为完成标准。

一个阶段只有同时满足：

```text
API
+
Documentation
+
Example
+
Visual Result
+
Developer Test
```

才能视为完成。

尤其是 Phase 1 和 Phase 2。

如果一个 API 在代码层面看起来简单，但新人实际使用仍然困难，则不能认为 DX 已完成。

---

# 9. Non-Goals

本路线当前不追求：

* 完整 CSS
* HTML
* JavaScript Runtime
* Electron API compatibility
* 完整浏览器能力
* UI DSL
* CSS Selector
* CSS Cascade
* 热重载优先
* 大规模 Widget 数量扩张

Yuzuki 的目标不是成为 Electron 的替代品。

目标是：

> **学习成本接近现代 Web UI 开发，同时保持 Native GUI 的运行时效率。**

---

# 10. Development Order

严格按照以下顺序推进：

```text
Phase 0
Consistency Cleanup
        ↓
Phase 1
On-ramp
        ↓
Phase 2
Design System
        ↓
Phase 3
Developer Tools
        ↓
Phase 4
API Reference + Cookbook
        ↓
Phase 5
Framework Maturity
        ↓
Phase 6
Hot Reload
```

不得跳过前置阶段直接大规模实现后续系统。

尤其禁止在 Phase 0 / Phase 1 尚未稳定时重新引入 DSL、CSS Parser 或大规模 Renderer 重构；禁止在 Phase 5 验收通过前提前实现 Hot Reload 的任何底层钩子。

---

# 11. Final DX Vision

Yuzuki 最终希望让开发者形成这样的开发体验：

```text
“我想做一个窗口。”
        ↓
创建 Window
        ↓
添加 Layout
        ↓
添加 Widget
        ↓
绑定 Event
        ↓
运行
```

开发者不需要首先学习 Yuzuki 的 Renderer、坐标系统、绘制生命周期和内部 Widget 实现。

当开发者需要进一步控制 UI 时，再逐层进入：

```text
Simple API
    ↓
Builder / Fluent API
    ↓
Theme / Style
    ↓
Custom Widget
    ↓
Custom Layout
    ↓
Paint / Renderer
```

这就是 Yuzuki 的 Progressive Disclosure。

最终目标不是：

> “Yuzuki 拥有多少功能。”

而是：

> **“一个第一次接触 Yuzuki 的开发者，需要学习多少东西，才能做出一个漂亮、交互完整的 Native UI？”**

Yuzuki DX 的最终方向：

> **Electron-like developer experience. Native GUI resource efficiency.**
