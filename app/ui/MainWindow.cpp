#include "MainWindow.h"

#ifdef _WIN32
#include "Theme.h"
#include "core/Version.h"
#include "logging/Logger.h"

#include <string>
#include <vector>

namespace ufx {

namespace {
constexpr int kSidebarWidth = 200;
constexpr int kNavItemHeight = 44;
constexpr int kNavTop = 72;

const wchar_t* kNavLabels[] = {
    L"Dashboard", L"Games", L"Upscaling", L"Frame Generation",
    L"GPU", L"Profiles", L"Settings", L"Logs", L"About",
};

std::wstring Widen(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(len > 0 ? len - 1 : 0, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

std::wstring FormatVram(uint64_t bytes) {
    double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    wchar_t buf[32];
    swprintf(buf, 32, L"%.1f GB", gb);
    return buf;
}

std::wstring FeatureLevelStr(uint32_t fl) {
    switch (fl) {
        case 0xC200: return L"12_2";
        case 0xC100: return L"12_1";
        case 0xC000: return L"12_0";
        case 0xB100: return L"11_1";
        case 0xB000: return L"11_0";
        default:     return fl ? L"present" : L"-";
    }
}

void DrawText(HDC dc, const std::wstring& s, RECT rc, HFONT font, COLORREF color, UINT flags = DT_LEFT | DT_TOP) {
    HFONT old = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, s.c_str(), -1, &rc, flags);
    SelectObject(dc, old);
}

void FillRoundRect(HDC dc, RECT rc, COLORREF color, int radius = 8) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH ob = (HBRUSH)SelectObject(dc, brush);
    HPEN op = (HPEN)SelectObject(dc, pen);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(brush); DeleteObject(pen);
}
}

MainWindow::MainWindow() {
    paths_ = AppPaths::resolve();
    paths_.ensure();
    Logger::Instance().Init(paths_.logs);
    gpuEnum_ = CreateDefaultGpuEnumerator();
    gpus_ = gpuEnum_->Enumerate();
    profiles_ = std::make_unique<ProfileService>(paths_);
    if (auto res = profiles_->LoadAll()) profileList_ = res.value();
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    hInstance_ = hInstance;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &MainWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(GetTheme().background);
    wc.lpszClassName = L"UniversalFrameFXMainWindow";
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        0, wc.lpszClassName, L"Universal FrameFX",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        nullptr, nullptr, hInstance, this);
    if (!hwnd_) return false;

    ApplyDarkTitleBar(hwnd_);
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
    return true;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(l);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self) return self->Handle(msg, w, l);
    return DefWindowProc(hwnd, msg, w, l);
}

LRESULT MainWindow::Handle(UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
        case WM_CREATE: OnCreate(); return 0;
        case WM_PAINT: OnPaint(); return 0;
        case WM_ERASEBKGND: return 1;
        case WM_SIZE: OnSize(); return 0;
        case WM_LBUTTONDOWN: {
            POINT pt{ LOWORD(l), HIWORD(l) };
            for (int i = 0; i < static_cast<int>(Page::Count); ++i)
                if (PtInRect(&navRects_[i], pt)) { OnNavClick(i); break; }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd_, msg, w, l);
}

void MainWindow::OnCreate() {
    fontTitle_   = CreateFontW(-28, 0,0,0, FW_SEMIBOLD, 0,0,0, DEFAULT_CHARSET, 0,0,0,0, L"Segoe UI Variable Display");
    fontHeading_ = CreateFontW(-20, 0,0,0, FW_SEMIBOLD, 0,0,0, DEFAULT_CHARSET, 0,0,0,0, L"Segoe UI Variable Display");
    fontBody_    = CreateFontW(-15, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, 0,0,0,0, L"Segoe UI");
    fontSmall_   = CreateFontW(-13, 0,0,0, FW_NORMAL,   0,0,0, DEFAULT_CHARSET, 0,0,0,0, L"Segoe UI");
}

void MainWindow::OnSize() { InvalidateRect(hwnd_, nullptr, TRUE); }

void MainWindow::OnNavClick(int index) {
    current_ = static_cast<Page>(index);
    if (current_ == Page::Profiles) {
        if (auto res = profiles_->LoadAll()) profileList_ = res.value();
    }
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void MainWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd_, &ps);
    RECT client; GetClientRect(hwnd_, &client);

    // Double buffer.
    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bmp = CreateCompatibleBitmap(dc, client.right, client.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(mem, bmp);

    HBRUSH bg = CreateSolidBrush(GetTheme().background);
    FillRect(mem, &client, bg);
    DeleteObject(bg);

    RECT side = { 0, 0, kSidebarWidth, client.bottom };
    PaintSidebar(mem, side);

    RECT content = { kSidebarWidth, 0, client.right, client.bottom };
    PaintContent(mem, content);

    BitBlt(dc, 0, 0, client.right, client.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd_, &ps);
}

void MainWindow::PaintSidebar(HDC dc, const RECT& rc) {
    HBRUSH sb = CreateSolidBrush(GetTheme().sidebar);
    FillRect(dc, &rc, sb);
    DeleteObject(sb);

    RECT logo = { 20, 20, kSidebarWidth - 10, 60 };
    DrawText(dc, L"FrameFX", logo, fontHeading_, GetTheme().accent);

    POINT cursor; GetCursorPos(&cursor); ScreenToClient(hwnd_, &cursor);

    for (int i = 0; i < static_cast<int>(Page::Count); ++i) {
        RECT item = { 8, kNavTop + i * kNavItemHeight, kSidebarWidth - 8,
                      kNavTop + i * kNavItemHeight + kNavItemHeight - 4 };
        navRects_[i] = item;
        bool active = (static_cast<int>(current_) == i);
        bool hover = PtInRect(&item, cursor);
        if (active) FillRoundRect(dc, item, GetTheme().accent, 8);
        else if (hover) FillRoundRect(dc, item, GetTheme().surfaceAlt, 8);
        RECT textRect = item; textRect.left += 16;
        DrawText(dc, kNavLabels[i], textRect, fontBody_,
                 active ? RGB(255,255,255) : GetTheme().text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    RECT ver = { 16, rc.bottom - 34, kSidebarWidth - 8, rc.bottom - 10 };
    DrawText(dc, L"v" + Widen(kAppVersion), ver, fontSmall_, GetTheme().textMuted);
}

void MainWindow::PaintContent(HDC dc, RECT rc) {
    rc.left += 32; rc.top += 28; rc.right -= 32; rc.bottom -= 24;
    switch (current_) {
        case Page::Dashboard:       PaintDashboard(dc, rc); break;
        case Page::Games:           PaintGames(dc, rc); break;
        case Page::Upscaling:       PaintUpscaling(dc, rc); break;
        case Page::FrameGeneration: PaintFrameGen(dc, rc); break;
        case Page::Gpu:             PaintGpu(dc, rc); break;
        case Page::Profiles:        PaintProfiles(dc, rc); break;
        case Page::Settings:        PaintSettings(dc, rc); break;
        case Page::Logs:            PaintLogs(dc, rc); break;
        case Page::About:           PaintAbout(dc, rc); break;
        default: break;
    }
}

void MainWindow::PaintDashboard(HDC dc, RECT rc) {
    DrawText(dc, L"UNIVERSAL FRAMEFX", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    int y = rc.top + 60;

    // GPU card
    const GpuInfo* gpu = gpus_.empty() ? nullptr : &gpus_[0];
    RECT card = { rc.left, y, rc.left + 320, y + 130 };
    FillRoundRect(dc, card, GetTheme().surface, 10);
    DrawText(dc, L"GPU", { card.left + 16, card.top + 12, card.right - 12, card.top + 34 }, fontSmall_, GetTheme().textMuted);
    if (gpu) {
        DrawText(dc, Widen(gpu->name), { card.left + 16, card.top + 34, card.right - 12, card.top + 62 }, fontHeading_, GetTheme().text);
        DrawText(dc, FormatVram(gpu->dedicatedVramBytes) + L" VRAM",
                 { card.left + 16, card.top + 66, card.right - 12, card.top + 88 }, fontBody_, GetTheme().textMuted);
        DrawText(dc, L"Driver: " + Widen(gpu->driverVersion.empty() ? "unknown" : gpu->driverVersion),
                 { card.left + 16, card.top + 90, card.right - 12, card.top + 112 }, fontSmall_, GetTheme().textMuted);
    } else {
        DrawText(dc, L"No GPU detected", { card.left + 16, card.top + 34, card.right - 12, card.top + 62 }, fontBody_, GetTheme().bad);
    }

    // Active profile card
    RECT card2 = { rc.left + 340, y, rc.left + 660, y + 130 };
    FillRoundRect(dc, card2, GetTheme().surface, 10);
    DrawText(dc, L"ACTIVE PROFILE", { card2.left + 16, card2.top + 12, card2.right - 12, card2.top + 34 }, fontSmall_, GetTheme().textMuted);
    if (!profileList_.empty()) {
        const auto& p = profileList_.front();
        DrawText(dc, Widen(p.name), { card2.left + 16, card2.top + 34, card2.right - 12, card2.top + 62 }, fontHeading_, GetTheme().text);
        std::wstring up = Widen(UpscalerIdName(p.upscaler)) + L" - " + Widen(QualityModeName(p.quality));
        DrawText(dc, up, { card2.left + 16, card2.top + 66, card2.right - 12, card2.top + 88 }, fontBody_, GetTheme().textMuted);
        DrawText(dc, p.frameGenEnabled ? L"Frame Generation: Enabled" : L"Frame Generation: Disabled",
                 { card2.left + 16, card2.top + 90, card2.right - 12, card2.top + 112 }, fontSmall_, GetTheme().textMuted);
    } else {
        DrawText(dc, L"No profiles yet", { card2.left + 16, card2.top + 34, card2.right - 12, card2.top + 62 }, fontBody_, GetTheme().textMuted);
    }

    y += 150;
    RECT note = { rc.left, y, rc.right, y + 120 };
    FillRoundRect(dc, note, GetTheme().surface, 10);
    DrawText(dc,
        L"Universal FrameFX configures the upscalers a game already supports and manages\n"
        L"profiles and backups. It does not add FSR/XeSS or frame generation to games that\n"
        L"were not built for them. FPS numbers are never fabricated; measured values are\n"
        L"labelled as such on the GPU page.",
        { note.left + 16, note.top + 14, note.right - 16, note.bottom - 12 }, fontBody_, GetTheme().textMuted);
}

void MainWindow::PaintGpu(HDC dc, RECT rc) {
    DrawText(dc, L"GPU", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    int y = rc.top + 56;
    if (gpus_.empty()) {
        DrawText(dc, L"No GPU detected via DXGI.", { rc.left, y, rc.right, y + 30 }, fontBody_, GetTheme().bad);
        return;
    }
    for (const auto& g : gpus_) {
        RECT card = { rc.left, y, rc.right, y + 176 };
        FillRoundRect(dc, card, GetTheme().surface, 10);
        int cx = card.left + 20, cy = card.top + 16;
        DrawText(dc, Widen(g.name), { cx, cy, card.right - 20, cy + 28 }, fontHeading_, GetTheme().text);
        cy += 34;
        auto row = [&](const std::wstring& k, const std::wstring& v, COLORREF vc) {
            DrawText(dc, k, { cx, cy, cx + 220, cy + 22 }, fontBody_, GetTheme().textMuted);
            DrawText(dc, v, { cx + 230, cy, card.right - 20, cy + 22 }, fontBody_, vc);
            cy += 24;
        };
        row(L"Vendor", Widen(VendorName(g.vendor)), GetTheme().text);
        row(L"Architecture", Widen(ArchName(g.arch)), GetTheme().text);
        row(L"VRAM", FormatVram(g.dedicatedVramBytes), GetTheme().text);
        row(L"Driver", Widen(g.driverVersion.empty() ? "unknown" : g.driverVersion), GetTheme().text);
        row(L"DirectX 11", g.supportsD3D11 ? L"Supported (FL " + FeatureLevelStr(g.d3d11FeatureLevel) + L")" : L"No",
            g.supportsD3D11 ? GetTheme().ok : GetTheme().bad);
        row(L"DirectX 12", g.supportsD3D12 ? L"Supported (FL " + FeatureLevelStr(g.d3d12FeatureLevel) + L")" : L"No",
            g.supportsD3D12 ? GetTheme().ok : GetTheme().bad);
        row(L"Vulkan", g.supportsVulkan ? (L"Supported " + Widen(g.vulkanApiVersion)) : L"Not detected",
            g.supportsVulkan ? GetTheme().ok : GetTheme().textMuted);
        y += 192;
    }
}

void MainWindow::PaintUpscaling(HDC dc, RECT rc) {
    DrawText(dc, L"Upscaling", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    int y = rc.top + 56;
    const GpuInfo* gpu = gpus_.empty() ? nullptr : &gpus_[0];
    GpuInfo fallback{};
    const GpuInfo& g = gpu ? *gpu : fallback;

    for (const auto& entry : upscalers_.Evaluate(g, GraphicsApi::D3D12)) {
        auto caps = entry.upscaler->Caps();
        RECT card = { rc.left, y, rc.right, y + 96 };
        FillRoundRect(dc, card, GetTheme().surface, 10);
        DrawText(dc, Widen(caps.displayName), { card.left + 18, card.top + 12, card.right - 200, card.top + 38 }, fontHeading_, GetTheme().text);
        std::wstring status = entry.availability.available ? L"Available" : L"Unavailable";
        DrawText(dc, status, { card.right - 200, card.top + 12, card.right - 18, card.top + 38 },
                 fontBody_, entry.availability.available ? GetTheme().ok : GetTheme().bad, DT_RIGHT);
        std::wstring detail = entry.availability.available
            ? Widen(caps.requirements)
            : Widen(entry.availability.reason);
        DrawText(dc, detail, { card.left + 18, card.top + 40, card.right - 18, card.top + 88 }, fontSmall_, GetTheme().textMuted);
        y += 108;
    }
}

void MainWindow::PaintFrameGen(HDC dc, RECT rc) {
    DrawText(dc, L"Frame Generation", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    int y = rc.top + 56;
    const GpuInfo* gpu = gpus_.empty() ? nullptr : &gpus_[0];
    GpuInfo fallback{};
    const GpuInfo& g = gpu ? *gpu : fallback;

    for (const auto& gen : frameGens_.All()) {
        auto caps = gen->Caps();
        // We do not assume the game supports FG; show the honest default state.
        FrameGenState state = gen->Evaluate(g, GraphicsApi::D3D12, /*gameDeclaresSupport=*/false);
        RECT card = { rc.left, y, rc.right, y + 120 };
        FillRoundRect(dc, card, GetTheme().surface, 10);
        DrawText(dc, Widen(caps.displayName), { card.left + 18, card.top + 12, card.right - 260, card.top + 38 }, fontHeading_, GetTheme().text);
        COLORREF sc = state == FrameGenState::Supported ? GetTheme().ok : GetTheme().warn;
        DrawText(dc, Widen(FrameGenStateLabel(state)), { card.right - 300, card.top + 12, card.right - 18, card.top + 38 }, fontBody_, sc, DT_RIGHT);
        DrawText(dc, Widen(caps.externalLimitation), { card.left + 18, card.top + 42, card.right - 18, card.bottom - 10 }, fontSmall_, GetTheme().textMuted);
        y += 132;
    }
}

void MainWindow::PaintProfiles(HDC dc, RECT rc) {
    DrawText(dc, L"Profiles", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    int y = rc.top + 56;
    if (profileList_.empty()) {
        DrawText(dc, L"No profiles found. Profiles are stored as JSON under\n" + Widen(paths_.profiles.string()),
                 { rc.left, y, rc.right, y + 60 }, fontBody_, GetTheme().textMuted);
        return;
    }
    for (const auto& p : profileList_) {
        RECT card = { rc.left, y, rc.right, y + 92 };
        FillRoundRect(dc, card, GetTheme().surface, 10);
        DrawText(dc, Widen(p.name), { card.left + 18, card.top + 12, card.right - 18, card.top + 38 }, fontHeading_, GetTheme().text);
        wchar_t line[256];
        swprintf(line, 256, L"%s  -  %s / %s  -  %u x %u  -  FG %s",
                 Widen(p.executablePath).c_str(),
                 Widen(GraphicsApiName(p.api)).c_str(),
                 Widen(UpscalerIdName(p.upscaler)).c_str(),
                 p.outputWidth, p.outputHeight,
                 p.frameGenEnabled ? L"on" : L"off");
        DrawText(dc, line, { card.left + 18, card.top + 42, card.right - 18, card.bottom - 10 }, fontSmall_, GetTheme().textMuted);
        y += 104;
    }
}

void MainWindow::PaintGames(HDC dc, RECT rc) {
    DrawText(dc, L"Games", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    DrawText(dc,
        L"Add a game by selecting its executable, or scan Steam / Epic / GOG.\n"
        L"Everything stays local; your library is never transmitted.\n\n"
        L"Before Universal FrameFX writes to a game folder it creates a backup and\n"
        L"refuses to touch folders protected by anti-cheat (EAC / BattlEye / VAC).",
        { rc.left, rc.top + 56, rc.right, rc.top + 200 }, fontBody_, GetTheme().textMuted);
}

void MainWindow::PaintSettings(HDC dc, RECT rc) {
    DrawText(dc, L"Settings", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    DrawText(dc, L"Data directory:\n" + Widen(paths_.root.string()),
             { rc.left, rc.top + 56, rc.right, rc.top + 120 }, fontBody_, GetTheme().textMuted);
}

void MainWindow::PaintLogs(HDC dc, RECT rc) {
    DrawText(dc, L"Logs", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    DrawText(dc, L"Log file:\n" + Widen((paths_.logs / "universalframefx.log").string()),
             { rc.left, rc.top + 56, rc.right, rc.top + 120 }, fontBody_, GetTheme().textMuted);
}

void MainWindow::PaintAbout(HDC dc, RECT rc) {
    DrawText(dc, L"About", { rc.left, rc.top, rc.right, rc.top + 40 }, fontTitle_, GetTheme().text);
    DrawText(dc,
        L"Universal FrameFX v" + Widen(kAppVersion) + L"\n\n"
        L"Open-source (MIT). A unified interface for FSR, XeSS and frame generation.\n"
        L"No proprietary vendor binaries are bundled. This tool never fakes support for\n"
        L"a graphics technology.",
        { rc.left, rc.top + 56, rc.right, rc.top + 200 }, fontBody_, GetTheme().textMuted);
}

}
#endif
