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
        var probe = Interop.D3DProbe.Probe();

        return new PlatformInfo
        {
            Build = build,
            Win11 = win11,
            // Probed, not guessed. d3d11.dll and d3d12.dll exist on every
            // Windows 10/11 install regardless of GPU capability, so a
            // File.Exists check answered a different question than the one asked.
            DX11 = probe.Dx11,
            DX12 = probe.Dx12,
            Dx11FeatureLevel = Interop.D3DProbe.FeatureLevelText(probe.Dx11FeatureLevel),
            // Still a file check, and still labelled as one: the loader being
            // present says nothing about a usable Vulkan device, so this must
            // never gate a capability.
            VulkanLoaderPresent = File.Exists(Path.Combine(sys, "vulkan-1.dll")),
        };
    }
}
