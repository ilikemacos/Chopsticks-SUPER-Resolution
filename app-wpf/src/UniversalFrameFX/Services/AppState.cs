using UniversalFrameFX.Models;

namespace UniversalFrameFX.Services;

/// <summary>Hardware/platform detected once at startup and shared across the UI.</summary>
public static class AppState
{
    public const string Version = "0.4.0";

    public static IReadOnlyList<GpuInfo> Gpus { get; private set; } = new List<GpuInfo>();
    public static PlatformInfo Platform { get; private set; } = new();

    /// <summary>The primary (preferred) GPU, or null if none was detected.</summary>
    public static GpuInfo? PrimaryGpu => Gpus.Count > 0 ? Gpus[0] : null;

    public static void Initialize()
    {
        AppPaths.EnsureCreated();
        Gpus = GpuDetector.Enumerate();
        Platform = PlatformDetector.Detect();
    }
}
