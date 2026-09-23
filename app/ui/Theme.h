#pragma once

#ifdef _WIN32
#include <windows.h>

namespace ufx {

// Fluent-inspired dark palette.
struct Theme {
    COLORREF background   = RGB(0x1E, 0x1E, 0x1E);
    COLORREF surface      = RGB(0x2B, 0x2B, 0x2B);
    COLORREF surfaceAlt   = RGB(0x33, 0x33, 0x33);
    COLORREF sidebar      = RGB(0x25, 0x25, 0x25);
    COLORREF accent       = RGB(0x3B, 0x82, 0xF6);
    COLORREF accentHover  = RGB(0x60, 0xA5, 0xFA);
    COLORREF text         = RGB(0xF2, 0xF2, 0xF2);
    COLORREF textMuted    = RGB(0xA0, 0xA0, 0xA0);
    COLORREF ok           = RGB(0x4A, 0xDE, 0x80);
    COLORREF warn         = RGB(0xFB, 0xBF, 0x24);
    COLORREF bad          = RGB(0xF8, 0x71, 0x71);
    COLORREF border       = RGB(0x3A, 0x3A, 0x3A);
};

const Theme& GetTheme();

// Enable the Windows 11 dark title bar / mica where available.
void ApplyDarkTitleBar(HWND hwnd);

}
#endif
