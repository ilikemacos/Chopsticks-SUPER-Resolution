using System.IO;
using System.Management;
using UniversalFrameFX.Models;

namespace UniversalFrameFX.Services;

/// <summary>Detects the OS build and which graphics runtimes are actually present.</summary>
public static class PlatformDetector
{
    public static PlatformInfo Detect()
    {
        int build = 0;
        try
        {
            using var searcher = new ManagementObjectSearcher(
                "SELECT BuildNumber FROM Win32_OperatingSystem");
            foreach (ManagementObject mo in searcher.Get())
            {
                if (int.TryParse(mo["BuildNumber"] as string, out var b)) build = b;
                break;
            }
        }
        catch { /* leave build at 0. */ }

        bool win11 = build >= 22000;
        string sys = Environment.GetFolderPath(Environment.SpecialFolder.System);

        return new PlatformInfo
        {
            Build = build,
            Win11 = win11,
            DX11 = File.Exists(Path.Combine(sys, "d3d11.dll")),
            // DX12 runtime ships with Windows; treat it as usable on Windows 11.
            DX12 = File.Exists(Path.Combine(sys, "d3d12.dll")) && win11,
            Vulkan = File.Exists(Path.Combine(sys, "vulkan-1.dll")),
        };
    }
}
