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
            new()
            {
                Id = "None", Name = "Native (no upscaling)",
                HardwareOk = true, Integration = Integration.Native,
                Requirements = "The game renders at its native output resolution.",
            },

            // CSR is the only row here that this application can actually apply
            // itself. Everything else is a front-end for something the game must
            // already ship.
            new()
            {
                Id = "CSR", Name = "CSR — Chopsticks Super Resolution",
                HardwareOk = plat.DX11,
                Determined = plat.DX11 ? true : (bool?)false,
                Integration = Integration.External,
                Reason = plat.DX11 ? "" : "Requires a Direct3D 11 capable GPU.",
                Requirements =
                    "Spatial, applied by FrameFX to a captured window — works without " +
                    "game integration. Derived from FSR 1 and measured +0.74 dB PSNR " +
                    "over it on edge detail. Adds about one frame of latency, and costs " +
                    "GPU time, so it only pays off when the game renders fewer pixels " +
                    "than it presents.",
            },

            new()
            {
                Id = "FSR1", Name = "FSR 1 (spatial)",
                HardwareOk = true, Integration = Integration.Native,
                Requirements = "Spatial; runs on any GPU, but the game has to expose it.",
            },
            new()
            {
                Id = "FSR2", Name = "FSR 2",
                HardwareOk = true, Integration = Integration.Native,
                Requirements = "Temporal: needs motion vectors, depth and jitter from the game.",
            },
        };

        // FSR 3 needs a real DX12 device. Previously this was gated on
        // "is this Windows 11", which reported it unavailable on every Windows 10
        // machine that in fact supports it.
        rows.Add(new CapabilityRow
        {
            Id = "FSR3", Name = "FSR 3 (upscaling)",
            HardwareOk = plat.DX12,
            Determined = true,
            Integration = Integration.Native,
            Reason = plat.DX12 ? "" : "No Direct3D 12 device was found on this system.",
            Requirements = "Temporal; game integration required. Frame generation is separate.",
        });

        // FSR 4 needs RDNA 4. We infer architecture from a device-ID table, so
        // when that inference is not trustworthy the honest answer is
        // "undetermined" — not "unavailable". Ruling a capability out from a
        // guess is the same error as ruling it in.
        rows.Add(BuildFsr4Row(gpu));

        rows.Add(new CapabilityRow
        {
            Id = "XeSS", Name = "Intel XeSS",
            HardwareOk = true, Integration = Integration.Native,
            Requirements = "Temporal; XMX on Intel Arc, DP4a fallback elsewhere. " +
                           "Game integration required.",
        });

        return rows;
    }

    private static CapabilityRow BuildFsr4Row(GpuInfo? gpu)
    {
        const string requirements =
            "ML upscaler; requires AMD RDNA 4 hardware and a game that ships FSR 4.";

        if (gpu is null)
        {
            return new CapabilityRow
            {
                Id = "FSR4", Name = "FSR 4", HardwareOk = false, Determined = null,
                Integration = Integration.Native,
                Reason = "No GPU was detected, so this cannot be determined.",
                Requirements = requirements,
            };
        }

        // A non-AMD vendor is read straight from the PCI vendor ID, so ruling
        // FSR 4 out on that basis is a fact, not an inference.
        if (!string.Equals(gpu.Vendor, "AMD", StringComparison.OrdinalIgnoreCase))
        {
            return new CapabilityRow
            {
                Id = "FSR4", Name = "FSR 4", HardwareOk = false, Determined = true,
                Integration = Integration.Native,
                Reason = $"FSR 4 requires an AMD RDNA 4 GPU; this is {gpu.Vendor}.",
                Requirements = requirements,
            };
        }

        // AMD, but the architecture came from a device-ID lookup.
        bool isRdna4 = gpu.Arch.Equals("RDNA 4", StringComparison.OrdinalIgnoreCase);
        if (gpu.ArchCertainty == Certainty.Unknown)
        {
            return new CapabilityRow
            {
                Id = "FSR4", Name = "FSR 4", HardwareOk = false, Determined = null,
                Integration = Integration.Native,
                Reason = $"This AMD GPU (device 0x{gpu.DeviceId:X4}) is not in our " +
                         "architecture table, so we cannot confirm whether it is RDNA 4. " +
                         "Check AMD's own FSR 4 hardware list.",
                Requirements = requirements,
            };
        }

        return new CapabilityRow
        {
            Id = "FSR4", Name = "FSR 4",
            HardwareOk = isRdna4, Determined = true,
            Integration = Integration.Native,
            Reason = isRdna4 ? "" : $"FSR 4 requires RDNA 4; this is {gpu.ArchDisplay}.",
            Requirements = requirements,
        };
    }

    public static List<FrameGenRow> FrameGenerators(GpuInfo? gpu, PlatformInfo plat)
    {
        string state = plat.DX12
            ? "Requires game integration"
            : "Requires a Direct3D 12 device";
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
