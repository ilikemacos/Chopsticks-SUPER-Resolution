#ifdef _WIN32
#include "MainWindow.h"
#include "cli/UpscaleCommand.h"

#include <windows.h>
#include <commctrl.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Runs the CSR upscaler from the command line and prints to the parent console.
// A GUI-subsystem process has no console of its own, so attach to the one that
// launched us (falling back to allocating one) before writing.
int RunConsoleUpscale() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) AllocConsole();
    FILE* fo = nullptr; FILE* fe = nullptr;
    freopen_s(&fo, "CONOUT$", "w", stdout);
    freopen_s(&fe, "CONOUT$", "w", stderr);
    std::cout.clear();
    std::cerr.clear();

    std::vector<std::string> args;
    for (int i = 1; i < __argc; ++i) {
        // __argv is ANSI; adequate for file paths passed on the command line.
        args.emplace_back(__argv[i]);
    }

    ufx::UpscaleArgs parsed;
    std::string err;
    bool help = false;
    if (!ufx::ParseUpscaleArgs(args, parsed, err, help)) {
        std::cerr << "error: " << err << "\n\n";
        ufx::PrintUpscaleUsage(std::cerr);
        return 2;
    }
    if (help) { ufx::PrintUpscaleUsage(std::cout); return 0; }
    return ufx::RunUpscale(parsed, std::cout, std::cerr);
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    // Command-line arguments mean "upscale a file and exit"; no arguments launch
    // the GUI. This is what makes the installed .exe an actual upscaler, not only
    // a configurator.
    if (__argc > 1) return RunConsoleUpscale();

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    ufx::MainWindow window;
    if (!window.Create(hInstance, nCmdShow)) return 1;

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}
#endif
