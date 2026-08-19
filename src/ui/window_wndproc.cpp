// Window: wnd_proc (Win32 messages -> internal events). Handoff to input and
// rendering lives in window_input.cpp / window_frame.cpp / window_borderless.cpp
#include <yuzuki/ui/window.hpp>

#include <yuzuki/ui/application.hpp>
#include "window_internal.hpp"

#include <windowsx.h>

namespace yzk {

LRESULT CALLBACK Window::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Window* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            if (self) self->invalidate_all();
            return 0;
        }

        case WM_SIZE:
            if (self) self->on_resize(LOWORD(lparam), HIWORD(lparam));
            return 0;

        case WM_DPICHANGED: {
            if (self) {
                const u32 dpi = HIWORD(wparam);
                self->backend().set_dpi(dpi);
                const RECT* suggested = reinterpret_cast<const RECT*>(lparam);
                SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left, suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }

        case WM_CAPTURECHANGED:
            if (self) self->resize_zone_ = 0;
            return 0;

        // Borderless: intercept non-client presses — WS_POPUP has no SC_SIZE modal loop
        // (move works, resize doesn't), so both edge and caption presses become
        // manual start_gesture drags.
        case WM_NCLBUTTONDOWN:
            if (self && self->borderless() && !self->resize_zone_ && !self->maximized()) {
                const int zone = static_cast<int>(wparam);
                if (zone == HTCAPTION || zone == HTLEFT || zone == HTRIGHT || zone == HTTOP ||
                    zone == HTBOTTOM || zone == HTTOPLEFT || zone == HTTOPRIGHT ||
                    zone == HTBOTTOMLEFT || zone == HTBOTTOMRIGHT) {
                    self->start_gesture(zone);
                    return 0;
                }
            }
            break;

        // Caption double-click -> maximize/restore (standard system behavior)
        case WM_NCLBUTTONDBLCLK:
            if (self && self->borderless() && static_cast<int>(wparam) == HTCAPTION) {
                self->maximize_toggle();
                return 0;
            }
            break;

        case WM_NCLBUTTONUP:
            if (self && self->resize_zone_) {
                self->resize_zone_ = 0;
                ReleaseCapture();
                return 0;
            }
            break;

        // Borderless hit test: edge resize zones + caption drag area. Non-borderless
        // windows defer to DefWindowProc, which returns HT* zones from the frame/caption.
        case WM_NCHITTEST: {
            if (!self || !self->borderless()) break;
            POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
            ScreenToClient(hwnd, &pt);
            const f32 scale = self->backend().dpi_scale();
            const f32 x = static_cast<f32>(pt.x) / scale;
            const f32 y = static_cast<f32>(pt.y) / scale;
            LRESULT hit_result = self->hit_zone(x, y);
            if (hit_result == HTCLIENT && self->caption_hit(x, y)) hit_result = HTCAPTION;
            return hit_result;
        }

        // Borderless maximized avoids the taskbar (overrides the default fullscreen maximize)
        case WM_GETMINMAXINFO: {
            if (self && self->borderless()) {
                MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
                RECT wa{};
                if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0)) {
                    mmi->ptMaxSize.x = wa.right - wa.left;
                    mmi->ptMaxSize.y = wa.bottom - wa.top;
                    mmi->ptMaxPosition.x = wa.left;
                    mmi->ptMaxPosition.y = wa.top;
                }
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (self) {
                TRACKMOUSEEVENT tme{};
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                self->on_mouse_input(msg, static_cast<f32>(GET_X_LPARAM(lparam)),
                                     static_cast<f32>(GET_Y_LPARAM(lparam)),
                                     map_mouse_buttons(), map_mods());
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            if (self) self->update_hover(-1.0f, -1.0f);
            return 0;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if (self) self->on_mouse_input(msg, static_cast<f32>(GET_X_LPARAM(lparam)),
                                           static_cast<f32>(GET_Y_LPARAM(lparam)),
                                           map_mouse_buttons(), map_mods());
            return 0;

        case WM_MOUSEWHEEL: {
            if (self) {
                POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ScreenToClient(hwnd, &pt);
                const f32 scale = self->backend().dpi_scale();
                self->on_wheel(static_cast<f32>(pt.x) / scale, static_cast<f32>(pt.y) / scale,
                               static_cast<i16>(GET_WHEEL_DELTA_WPARAM(wparam)), map_mods());
            }
            return 0;
        }

        case WM_CHAR:
            if (self) self->on_key(msg, 0, static_cast<u16>(wparam), map_mods(), false);
            return 0;

        case WM_KEYDOWN:
        case WM_KEYUP:
            if (self) self->on_key(msg, static_cast<u32>(wparam), static_cast<u16>(0),
                                   map_mods(), (lparam & 0x40000000) != 0);
            return 0;

        case WM_TIMER: {
            if (self) {
                auto it = self->timers_.find(static_cast<UINT_PTR>(wparam));
                if (it != self->timers_.end() && it->second) {
                    Widget* w = it->second;
                    Event tick;
                    tick.type = EventType::Timer;
                    w->on_event(tick);
                    // Handlers may re-arm/stop timers (start_timer erases the entry), invalidating
                    // this iterator — re-find before invalidating; never touch a freed node.
                    auto it2 = self->timers_.find(static_cast<UINT_PTR>(wparam));
                    // The animation driver timer targets the window itself: it only
                    // wakes the message loop (avoids WaitMessage sleeping); real work is
                    // set by tick_frames' invalidations. Invalidating the whole window
                    // here would full-repaint every animation frame.
                    if (it2 != self->timers_.end() && it2->second && it2->second != self)
                        it2->second->invalidate();
                }
            }
            return 0;
        }

        case WM_SETCURSOR:
            if (self && self->borderless()) {
                // Borderless windows fully manage the cursor: size arrows on the edge zones,
                // widget cursor state everywhere else
                POINT pt{};
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                const f32 scale = self->backend().dpi_scale();
                switch (self->hit_zone(static_cast<f32>(pt.x) / scale,
                                       static_cast<f32>(pt.y) / scale)) {
                    case HTLEFT:
                    case HTRIGHT:
                        SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
                        return TRUE;
                    case HTTOP:
                    case HTBOTTOM:
                        SetCursor(LoadCursorW(nullptr, IDC_SIZENS));
                        return TRUE;
                    case HTTOPLEFT:
                    case HTBOTTOMRIGHT:
                        SetCursor(LoadCursorW(nullptr, IDC_SIZENWSE));
                        return TRUE;
                    case HTTOPRIGHT:
                    case HTBOTTOMLEFT:
                        SetCursor(LoadCursorW(nullptr, IDC_SIZENESW));
                        return TRUE;
                    default:
                        self->update_cursor();
                        return TRUE;
                }
            }
            if (LOWORD(lparam) == HTCLIENT) {
                if (self) self->update_cursor();
                return TRUE;
            }
            break;

        case WM_CLOSE:
            // Full teardown (resets hwnd_/state/render target); the Window can be re-created
            if (self) self->destroy();
            return 0;

        case WM_DESTROY:
            if (self) Application::instance().remove_window(self);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace yzk