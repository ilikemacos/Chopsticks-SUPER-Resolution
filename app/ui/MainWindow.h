#pragma once

#ifdef _WIN32
#include <windows.h>

#include "core/AppPaths.h"
#include "gpu/GpuEnumerator.h"
#include "upscaling/UpscalerRegistry.h"
#include "framegen/FrameGenRegistry.h"
#include "profiles/ProfileService.h"

#include <memory>
#include <vector>

namespace ufx {

enum class Page {
    Dashboard, Games, Upscaling, FrameGeneration, Gpu, Profiles, Settings, Logs, About, Count
};

class MainWindow {
public:
    MainWindow();
    bool Create(HINSTANCE hInstance, int nCmdShow);
    HWND Handle() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(UINT msg, WPARAM w, LPARAM l);

    void OnCreate();
    void OnPaint();
    void OnSize();
    void OnNavClick(int index);
    void PaintSidebar(HDC dc, const RECT& rc);
    void PaintContent(HDC dc, RECT rc);

    void PaintDashboard(HDC dc, RECT rc);
    void PaintGpu(HDC dc, RECT rc);
    void PaintUpscaling(HDC dc, RECT rc);
    void PaintFrameGen(HDC dc, RECT rc);
    void PaintProfiles(HDC dc, RECT rc);
    void PaintGames(HDC dc, RECT rc);
    void PaintSettings(HDC dc, RECT rc);
    void PaintLogs(HDC dc, RECT rc);
    void PaintAbout(HDC dc, RECT rc);

    HWND hwnd_ = nullptr;
    HINSTANCE hInstance_ = nullptr;
    Page current_ = Page::Dashboard;
    RECT navRects_[static_cast<int>(Page::Count)]{};

    AppPaths paths_;
    std::unique_ptr<IGpuEnumerator> gpuEnum_;
    std::vector<GpuInfo> gpus_;
    UpscalerRegistry upscalers_;
    FrameGenRegistry frameGens_;
    std::unique_ptr<ProfileService> profiles_;
    std::vector<GameProfile> profileList_;

    HFONT fontBody_ = nullptr;
    HFONT fontHeading_ = nullptr;
    HFONT fontTitle_ = nullptr;
    HFONT fontSmall_ = nullptr;
};

}
#endif
