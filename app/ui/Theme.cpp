#include "Theme.h"

#ifdef _WIN32
#include <dwmapi.h>

namespace ufx {

const Theme& GetTheme() {
    static Theme t;
    return t;
}

void ApplyDarkTitleBar(HWND hwnd) {
    // DWMWA_USE_IMMERSIVE_DARK_MODE == 20 on Windows 11.
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));

    // Optional: request Mica backdrop (DWMWA_SYSTEMBACKDROP_TYPE == 38, DWMSBT_MAINWINDOW == 2).
    int backdrop = 2;
    DwmSetWindowAttribute(hwnd, 38, &backdrop, sizeof(backdrop));
}

}
#endif
