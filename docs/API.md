# Yuzuki API Reference

面向**应用开发者**的 API 速查——"我要做 X,该调用什么"。按任务组织,不是按头文件。

> 学习如何使用 → 看 [TUTORIAL.md](TUTORIAL.md)(30 分钟从零搭出完整页面,
> 用推荐 fluent 写法,步骤化)。
> 这里只回答"某个 API 具体是什么、签名、怎么用"。
> 所有示例统一 canonical `set_xxx()` / `set_on_xxx()`(规则见下)。

- 命名空间:`yzk`,包含 `#include <yuzuki/yuzuki.hpp>` 即可。
- 一切**写操作**都用 `set_xxx(value)`(canonical,返回 `*this` 可链式);**读操作**用无参 `xxx()`。见下面"API 风格约定"。
- 新控件通过 `parent->add<T>(args...)` 创建并挂载(等价于 `new T + append_child`)。**注意返回 `T&`(不是指针)**,接 `.` 而不是 `->`:`auto& b = col->add<Button>("Go"); b.set_text(...)`。

> **API 风格约定(唯一)
> - **Getter 一律无参 `xxx()`**:如 `w->width()`、`l->bold()`、`s->value()`、
>   `b->padding()`。它们不带参数,只读,可安全查询。
> - **Setter 一律 `set_xxx(v)`**:这是全库每个控件都提供了的 **canonical 写入口**,
>   返回 `*this`,支持链式调用——本参考所有写操作只用它。
> - **同名便捷 setter(陷阱,避免)**:部分控件还有与 getter **同名的带参重载**
>   (如 `width(120)`、`bold(true)`、`padding(8)`),靠"有参数"与 getter 区分。
>   它只在部分控件存在,不构成契约,且易与 getter 混淆;本参考一律不展示。
> - **回调**:`set_on_xxx(cb)` 是 canonical,`on_xxx(cb)` 是等价别名。
> - **动作动词**(`add/remove/dock/show/open/close/clear/tween/append_child`)
>   本身就是唯一 API,无 `set_` 前缀,不冲突。

---

## 1. 最小窗口骨架

```cpp
#include <yuzuki/yuzuki.hpp>
using namespace yzk;

int main() {
    auto& app = Application::instance();
    Window win("标题", 480, 320);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");  // 字体随 exe 分发
    auto* root = new DockPanel;
    auto* col = new StackPanel(Orientation::Vertical);
    col->set_spacing(12.0f).set_padding(24.0f);
    root->dock(col, Dock::Fill);
    col->add<Label>("Hello").set_bold(true);
    win.set_root(root);
    win.show();
    return app.run();
}
```

骨架要点(逐项解释见对应章节):

| 这一段是干嘛的 | 想深入查这里 |
|---|---|
| 后接布局 | §2 布局 |
| 后接控件 | §3 控件速查 |
| 后接事件/回调 | §4 交互 |
| 主题/外观 | §5 主题与设计系统 |
| `app.run()` 循环/窗口生命周期 | README + TUTORIAL 第 1 步 |

> 想从头学怎么搭一个页面 → [TUTORIAL.md](TUTORIAL.md),不在这里重复。

## 2. 搭界面:布局

《我的组件怎么排》— 选一个容器当根或子容器。

| 容器 | 一句话 | 关键参数 | 典型场景 |
|------|--------|----------|----------|
| `DockPanel` | 子节点停靠到上/下/左/右/填满 | `dock(widget, Dock::Left/Top/Right/Bottom/Fill)`、`set_gap(f32)` | 编辑器布局:左工具栏+右面板+中间 Fill |
| `StackPanel` | 单向线性排列(默认垂直) | `set_orientation(Horizontal/Vertical)`、`set_spacing`、`set_stretch_children` | 侧边栏、表单、工具栏 |
| `FlexBox` | CSS flexbox 语义 | `set_align_main(SpaceBetween…)`、`set_align_cross`、子项 `set_flex_grow`/`set_flex_shrink` | 顶栏(两端对齐)、导航条 |
| `GridPanel` | 行列网格,可跨行跨列 | `GridPanel(cols, rows)`、`add(w, col, row, col_span, row_span)`、`set_gap`、`set_column_auto/star/fixed` | 表单对齐、数据表格 |
| `WrapPanel` | 行满自动换行 | `set_spacing`、`set_line_spacing` | 标签云、图标网格 |

> 记住 `Layout` 是基类(measure/arrange 为空),**不能当根容器**——至少用一个具名容器。

常用布局参数(margin/size,所有控件都有):

```cpp
w->set_margin(Margins{0, 4, 0, 4});   // 外边距(四边)
w->set_margin(8.0f);                    // fluent:四边统一
w->set_min_width(120.0f);               // 最小宽度;set_min_height 同理
w->set_flex_grow(1.0f);                 // FlexBox 下生效
```

## 3. 控件速查

**文本 / 标题**

```cpp
auto& label = col->add<Label>("Title");
label.set_bold(true).set_small(false)    // 字号
     .set_text_role(TextRole::Secondary);   // 跟随主题的次要文字色
label.set_align(TextAlignH::Left, TextAlignV::Center);
// RichText lite(可选):一段文字内多样式,非嵌套标记
//   **加粗**  ~高亮(accent 色)~  [链接文字](action)
label2.set_rich_text(true).set_on_span_click([](const String& action){
    // 点击 [..](action) 链接 span 时触发
});
```

**按钮 / 图标按钮**

```cpp
auto& go = col->add<Button>("Go");
go.set_icon(IconId::PaperPlane).set_icon_size(18);   // 带图标
go.set_accent(false);                        // 非主色调样式
go.set_on_click([]{ /* ⚠️ 回调同步执行,必须 <1ms */ });
```

**输入:文本 / 数字切换**

```cpp
auto& box = col->add<TextBox>(String(), TextBoxConfig{});
box.set_placeholder("你的名字…")
   .set_text(String()).set_read_only(false);
box.set_on_commit([]{ /* Enter 提交(仅单行模式) */ });
// 编辑深度(已内置):undo()/redo()(Ctrl+Z / Ctrl+Y)、词边界导航
// (Ctrl+Left/Right/Backspace/Delete)、多行固定高度视口内部滚动
// TextBoxConfig 常用字段:enter_submits(多行 Enter 提交,Ctrl+Enter 换行)、
// transparent(不自绘表面/边框,由宿主画框)

auto& spin = col->add<SpinBox>(0.0, -100, 100, 1);
spin.set_decimals(2);                        // set_step()/set_range() 同理
spin.set_on_changed([](f64 v){ /* 值变化 */ });
```

**选择:复选 / 单选 / 开关 / 下拉**

```cpp
col->add<CheckBox>("记住我").set_on_toggled([](bool on){});

col->add<RadioButton>("A");            // 同父下自动互斥,天然单选组
col->add<RadioButton>("B").set_checked(true);

col->add<ToggleSwitch>().set_on_toggled([](bool on){});

auto& combo = col->add<ComboBox>(std::vector<String>{"x","y","z"});
combo.set_selected_index(0).set_on_changed([](i32 idx){});
// selected_text() 取选中文本;无选中可 set_placeholder("请选择")
```

**滑动 / 进度**

```cpp
auto& slider = col->add<Slider>();
slider.set_range(0.f, 100.f).set_value(50.f)
      .set_on_changed([](f32 v){});

auto& bar = col->add<ProgressBar>();
bar.set_value(0.6f);                        // 0..1
bar.set_indeterminate(true);                // 不确定模式,自动循环
```

**列表**

```cpp
auto& list = col->add<ListView>();
list.set_items(std::vector<String>{"a","b","c"});
list.set_on_selected([](i32 idx){});            // 单击选中
list.set_on_activate([](i32 idx){});            // 双击 / Enter

// 大列表(10k+):用数据源模式,按需渲染
list.set_data_source(new MySource);         // 实现 count()/text_at(i32)
// 自定义行:set_row_delegate(...) 实现 draw(ListView&, PaintContext&, i32, RectF)
```

**滚动**:`ScrollView` 包一个子控件即可:`sv->set_content(page)`;滚动位置 `set_scroll_y/set_scroll_by`。

**图片 / 图标**

```cpp
auto& img = col->add<Image>();
img.load_from_file(win, "photo.jpg");       // PNG/JPEG(WIC)
img.set_scale_mode(ImageScaleMode::Contain);

col->add<Icon>(IconId::Gear, 18).set_color(Color{0xFF,0x0,0x0});  // 不设色=跟随主题
```

**多页 — TabControl**(页头 + 活动页填充,页由控件托管):

```cpp
auto& tabs = col->add<TabControl>();
tabs.add_tab("常规", make_general_page())
    .add_tab("高级", make_advanced_page())
    .set_selected_index(0)
    .set_on_changed([](i32 idx){ /* 页切换 */ });
// 聚焦后 Left/Right 方向键切换;remove_tab(i32)/clear_tabs() 管理
```

## 4. 交互

**回调(推荐)** — 控件自带、轻量:

| 控件 | 回调(canonical) | 参数 | 时机 |
|------|------|------|------|
| Button | `set_on_click` | 无 | 点击 |
| TextBox | `set_on_commit` | 无 | Enter(单行) |
| ComboBox | `set_on_changed` | `i32` 选中下标 | 选项变化 |
| Slider / SpinBox | `set_on_changed` | `f32` / `f64` | 值变化(拖动/输入) |
| CheckBox / RadioButton / ToggleSwitch | `set_on_toggled` | `bool` | 状态变化 |
| ListView | `set_on_selected` `set_on_activate` | `i32` 行号 | 单击选中 / 激活 |

> 回调也有短别名 `on_click` / `on_changed` / `on_toggled`(与 `set_on_*` 等价),但本参考统一用 `set_on_*`。

**事件(进阶)** — 覆写 `virtual void on_event(Event&)`。事件类型详见 `EventType`:
`MouseMove/Enter/Leave/Down/Up/Click/DoubleClick/KeyDown/KeyUp/Character/Wheel/Resize/FocusGained/FocusLost/Timer/DragStart/Move/End/ImeCompose/ImeCommit/DropFiles`。

> ⚠️ 回调/事件处理**必须同步、<1ms 返回**(在消息泵内执行),长任务要推迟到帧边界。

**键盘焦点**:`request_focus()` 请求焦点;Window 自动化 `Tab` 循环 `focus_next()`、Enter 触发 `activate_focused()`。用 `set_focusable(bool)` 控制可聚焦性。

**定时器**:`win.start_timer(this, 500)`(每控件单 timer,重设即重启旧 → `EventType::Timer` 里处理;`stop_timer(this)` 停)。

**快捷键(窗口级)**:`win.add_accelerator({VK_S, KeyModifier_Control, []{ save(); }})` 在按键到达焦点控件**之前**触发;输入框聚焦时不劫持编辑键(Ctrl+Z/X/C/V/A、词导航、方向键等),应用组合(如 Ctrl+S、Ctrl+Shift+T)始终生效。`remove_accelerator(vk, mods)` 移除;F1/F2 预留给调试(见 §8)。

## 5. 外观:主题与设计系统

不用一个个设颜色——用 `Theme` 语义 token,深浅色切换自动生效。

```cpp
// 全局主题(对整个 app)
Theme::set(Theme::make_dark());   // 或 make_light()
// 读当前主题(自绘时也用它)
const Theme& t = Theme::get();
```

常用 token(`Theme::get().xxx`):

| 类别 | token |
|------|-------|
| 颜色 | `background` `surface` `surface_container` `accent` `text` `text_secondary` `text_disabled` `border` `track` |
| 排版 | `type_display(34)` `type_headline(28)` `type_title(20)` `type_body(14)` `type_label(13)` `type_caption(12)`;`font_family` `font_size` |
| 几何 | `control_height(32)` `control_height_compact(28)` `control_radius(4)` `radius_sm/md/pill` `border_width(1)` `border_width_strong(1.5)` |
| 滚动条 | `scrollbar_width(6)` `scrollbar_margin(2)` `wheel_step(40)` `scrollbar_thumb_min(24)` |
| 阴影 | `shadow_blur_floating(14)` `shadow_offset_floating(4)` `shadow_alpha_floating(70)`;`*_notice(12/4/40)` |

> token 分两类:**联动 token**(改主题即重绘:全部颜色/`control_height`/`control_radius`/`scrollbar_*`/`track`)和**常量 token**(`type_*`/`radius_*`/`border_width_*` 是给应用编码用的推荐值与放大系数,控件内部不强制读取,改它们不会自动重排你手写的布局)。验证:改 `Theme::make_dark()` 的 `accent` → 所有按钮/选中态一起变。

**给控件挂主题**:

```cpp
auto& card = col->add<Box>();
card.set_bg_role(ThemeRole::SurfaceContainerLow)   // 语义背景,跟随主题
    .set_radius(t.control_radius)
    .set_padding(16.f);

// 字体跟随主题
label->set_bold(true);
```

**卡片/容器外观** — `Box`:

```cpp
auto& b = col->add<Box>();
b.set_border(1.0f, t.border)                     // (width, color)
 .set_shadow(t.shadow_blur_floating, t.shadow_offset_floating)
 .set_radius(t.control_radius);                   // content_area() = 去掉 padding 后的区域
```

## 6. 动画

**隐式过渡(CSS transition 式)**:设一次 `set_transition(ms)`,之后视觉 setter 自动补间。

```cpp
w->set_transition(200.0f);
w->set_opacity(0.3f);          // 200ms 淡出
w->set_scale(1.2f, 1.2f);      // 200ms 放大
// 同样支持 set_translate / set_rotate_deg
```

**显式补间(任意数值)**:

```cpp
auto tok = AnimationSystem::instance().tween(
    0.0f, 1.0f, 280.0f, Easing::OutCubic,
    [w](f32 v){ w->set_progress(v); invalidate(); });
AnimationSystem::instance().finish_tween(tok);   // 立即终止到终值
```

缓动:Easing 枚举 `Linear/InQuad/OutQuad/InOutQuad/InCubic/OutCubic/…/OutBack`。每帧回调(跟随渲染节奏):`AnimationSystem::instance().on_frame(fn)` / `stop_frame(token)`。

## 7. 浮层:对话框 / 菜单 / 通知 / 提示

**消息框 —— `MessageBox`**(框架渲染的非阻塞模态:遮罩 + 居中面板 + 动画进出):

```cpp
MessageBox mb;
mb.show(win, "删除确认", "确定要删除这一项吗?", MessageBoxKind::YesNo,
        [](MessageBoxResult r){ /* Yes / No / None(遮罩关闭) */ });
// show() 立即返回;Esc=Cancel(单按钮时=该按钮),遮罩点击=None
// Info / Confirm / YesNo / YesNoCancel 四种形态;set_animated(false) 用于测试
```

**文件对话框 —— 原生 IFileDialog**(阻塞直至选择或取消,UTF-8 路径):

```cpp
auto files = open_file_dialog(&win, {.filters = {{"图片", "*.jpg;*.png"}},
                                     .multi_select = true});
String path = save_file_dialog(&win, {.default_name = "out.txt"});
// 空结果 = 用户取消
```

**模态对话框 —— `Overlay`**(全屏遮罩 + 居中面板,可加阴影/模糊,动画进出):

```cpp
class MyDialog : public Overlay {
public:
    MyDialog() {
        set_panel_rect(RectF::make(300, 120, 320, 180));
        // 在面板里放内容:自定义 paint 或往自身 append_child
    }
};
auto* dlg = new MyDialog;
dlg->show(win);      // win 传入窗口;Esc 或点遮罩外关闭
```

**右键菜单 —— `ContextMenu`**:

```cpp
auto* menu = new ContextMenu;
menu->add_item("复制", []{ /* ... */ });
menu->add_item("粘贴", []{ /* ... */ });
menu->add_separator();
menu->add_item("删除", []{ /* ... */ });
win.set_context_menu(menu);   // 窗口级全局右键
// 或指定位置弹出:menu->open(win, x, y);点击外部/Esc 自动关
```

**通知 —— `NotificationManager`**(右下角自动堆叠,滑入,超时消失):

```cpp
NotificationManager::instance().show(win, "标题", "内容",
                                     NotificationType::Info, 3200.0f);
```

**工具提示 —— `TooltipManager`**(hover 延迟 → 跟随鼠标 → 离开隐藏,全托管):

```cpp
TooltipManager::instance().set_tooltip(btn, "点击保存");
// TooltipManager::instance().remove_tooltip(btn) 解除
```

## 8. 调试工具(通用能力,F1/F2)

```cpp
auto* overlay = new DebugOverlay;
win.set_debug_overlay(overlay);        // F1:性能统计(VRS 帧/布局/录制/回放/内存)

auto* inspector = new WidgetInspector;
win.set_widget_inspector(inspector);   // F2:控件树 + 点选看属性/描边
```

无论什么窗口、什么场景,绑定即用。数据源:`win.frame_stats()`(累计值,采样区间做差值)。

## 9. 自绘控件(进阶)

覆写 `Widget::paint_impl(PaintContext&)`,用上下文里的绘图 API:

```cpp
class Gauge : public Widget {
public:
    void paint_impl(PaintContext& ctx) override {
        const Theme& t = ctx.theme();
        ctx.fill_rounded(bounds(), t.track, t.control_radius);
        ctx.fill_rounded(RectF::make(bounds().left, bounds().top,
                                     bounds().width() * value_, bounds().height()),
                         t.accent, t.control_radius);
        ctx.draw_text(sprintf_value(), bounds(), t.text);
    }
    f32 value_ = 0.6f;
};
```

常用:`fill_rect/rounded/circle/gradient`、`draw_border`、`draw_line`、`draw_shadow`、`draw_backdrop_blur`、`draw_text(_small)`、`draw_icon`、`draw_bitmap`;`push_clip/pop_clip` 裁剪;文本测量 `measure_text`。

> 事件:覆写 `on_event`。自绘控件的状态查询:`hovered()/pressed()/enabled()/visible()/focusable()`(框架维护,无需自管 hover flag)。

## 10. 其它实用

- **剪贴板**:`clipboard::set_text("...")` / `clipboard::get_text()` / `clipboard::has_text()`(UTF-8,全框架统一入口,TextBox 复制粘贴同源)。
- **二级窗口**:`child.set_owner(&win)` —— 随 owner 最小化/恢复并保持在其上方;`child.set_modal(true)` 禁用 owner,配合 `child.loop_until_closed()` 阻塞式对话框流;`set_topmost(true)` 常驻置顶。
- **文件拖放**:窗口接收 `EventType::DropFiles`,数据在 `e.data.drop.files`。
- **无边框/自定义标题栏**:`win.set_borderless(true)` 隐藏系统边框;`win.set_caption(widget)` 让指定区域可拖拽移动。
- **IME**:TextBox 非密码模式默认支持输入法组合;`refresh_ime_anchor()` 在 caret 移动后重锚定候选窗。
- **编码**:`utf::to_wide(str)` / `utf::to_utf8(wstr)`。
- **版本**:`YUZUKI_VERSION`(0.3.0)。