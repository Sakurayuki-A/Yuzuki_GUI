#pragma once
#include <yuzuki/core/types.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace yzk {

namespace utf {

inline WString to_wide(const String& text) {
    if (text.empty()) return WString();
    const int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (len <= 0) return WString();
    WString out(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), len);
    return out;
}

inline String to_utf8(const WString& text) {
    if (text.empty()) return String();
    const int len = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return String();
    String out(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), len, nullptr, nullptr);
    return out;
}

}  // namespace utf

}  // namespace yzk