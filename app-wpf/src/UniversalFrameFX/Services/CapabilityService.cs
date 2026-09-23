using UniversalFrameFX.Models;

namespace UniversalFrameFX.Services;

/// <summary>
/// The honest capability matrix. It states exactly what can be offered on the detected
/// hardware and why something cannot — it never claims support that is not real.
/// </summary>
public static class CapabilityService
{
    public static List<CapabilityRow> Upscalers(GpuInfo? gpu, PlatformInfo plat)
    {
        var rows = new List<CapabilityRow>
        {
            new() { Id = "None", Name = "Native (no upscaling)", Available = true,
                    Requirements = "The game renders at its native output resolution." },
            new() { Id = "FSR1", Name = "FSR 1 (spatial)", Available = true,
                    Requirements = "Spatial upscaler; runs on any GPU. Needs the game to expose it." },
            new() { Id = "FSR2", Name = "FSR 2", Available = true,
                    Requirements = "Temporal: needs motion vectors, depth and jitter from the game." },
        };

        bool fsr3ok = plat.DX12;
        rows.Add(new CapabilityRow
        {
            Id = "FSR3",
            Name = "FSR 3 (upscaling)",
            Available = fsr3ok,
            Reason = fsr3ok ? "" : "Requires a DirectX 12 capable system.",
            Requirements = "Temporal; game integration required. Frame generation is separate.",
        });

        bool fsr4ok = gpu is { Vendor: "AMD", Arch: "RDNA 4" };
        rows.Add(new CapabilityRow
        {
            Id = "FSR4",
            Name = "FSR 4",
            Available = fsr4ok,
            Reason = fsr4ok ? "" :
                gpu == null ? "No GPU detected." :
                gpu.Vendor != "AMD" ? $"FSR 4 requires an AMD RDNA 4 GPU; detected {gpu.Vendor}." :
                $"FSR 4 requires RDNA 4 hardware; detected {gpu.Arch}.",
            Requirements = "ML upscaler; requires AMD RDNA 4 hardware and a game that ships FSR 4.",
        });

        rows.Add(new CapabilityRow
        {
            Id = "XeSS",
            Name = "Intel XeSS",
            Available = true,
            Requirements = "Temporal; XMX on Intel Arc, DP4a fallback elsewhere. Game integration required.",
        });

        return rows;
    }

    public static List<FrameGenRow> FrameGenerators(GpuInfo? gpu, PlatformInfo plat)
    {
        string state = plat.DX12 ? "Requires game integration" : "Requires compatible implementation (DX12)";
        return new List<FrameGenRow>
        {
            new()
            {
                Name = "FSR 3 Frame Generation",
                State = state,
                Note = "Cannot be added to a game that did not ship it (needs the swapchain + engine motion vectors).",
            },
            new()
            {
                Name = "XeSS Frame Generation",
                State = state,
                Note = "Cannot be added to a game that did not ship it. Best on Intel Arc where the game integrates it.",
            },
        };
    }

    public static double ScaleRatio(string mode) => mode switch
    {
        "Native" => 1.0,
        "Ultra Quality" => 1.3,
        "Quality" => 1.5,
        "Balanced" => 1.7,
        "Performance" => 2.0,
        "Ultra Performance" => 3.0,
        _ => 1.0,
    };

    public static readonly string[] QualityModes =
    {
        "Native", "Ultra Quality", "Quality", "Balanced", "Performance", "Ultra Performance",
    };
}
