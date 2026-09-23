set(UFX_CORE_SOURCES
    core/AppPaths.cpp
    core/AppPaths.h
    core/Result.h
    core/Version.h
    gpu/GpuInfo.h
    gpu/GpuEnumerator.cpp
    gpu/GpuEnumerator.h
    gpu/MockGpuEnumerator.cpp
    gpu/MockGpuEnumerator.h
    upscaling/IUpscaler.h
    upscaling/QualityMode.h
    upscaling/FsrUpscaler.cpp
    upscaling/FsrUpscaler.h
    upscaling/XessUpscaler.cpp
    upscaling/XessUpscaler.h
    upscaling/NullUpscaler.cpp
    upscaling/NullUpscaler.h
    upscaling/UpscalerRegistry.cpp
    upscaling/UpscalerRegistry.h
    framegen/IFrameGenerator.h
    framegen/FsrFrameGenerator.cpp
    framegen/FsrFrameGenerator.h
    framegen/FrameGenRegistry.cpp
    framegen/FrameGenRegistry.h
    profiles/GameProfile.h
    profiles/ProfileService.cpp
    profiles/ProfileService.h
    profiles/GameScanner.cpp
    profiles/GameScanner.h
    profiles/BackupManager.cpp
    profiles/BackupManager.h
    logging/Logger.cpp
    logging/Logger.h)

add_library(ufx_core STATIC ${UFX_CORE_SOURCES})
target_include_directories(ufx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ufx_core PUBLIC nlohmann_json::nlohmann_json)

if(WIN32)
    target_sources(ufx_core PRIVATE
        gpu/DxgiGpuEnumerator.cpp
        gpu/DxgiGpuEnumerator.h
        dx11/Dx11Probe.cpp
        dx11/Dx11Probe.h
        dx12/Dx12Probe.cpp
        dx12/Dx12Probe.h
        vulkan/VulkanProbe.cpp
        vulkan/VulkanProbe.h)
    target_link_libraries(ufx_core PUBLIC dxgi d3d11 d3d12 version)
endif()

if(UFX_BUILD_APP AND WIN32)
    add_executable(UniversalFrameFX WIN32
        ui/WinMain.cpp
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/Theme.cpp
        ui/Theme.h)
    target_link_libraries(UniversalFrameFX PRIVATE ufx_core comctl32 dwmapi uxtheme)
    set_target_properties(UniversalFrameFX PROPERTIES
        VS_DPI_AWARE "PerMonitor"
        WIN32_EXECUTABLE TRUE)
endif()
