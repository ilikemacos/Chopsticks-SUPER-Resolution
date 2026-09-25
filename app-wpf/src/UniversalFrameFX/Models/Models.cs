using System.Text.Json.Serialization;

namespace UniversalFrameFX.Models;

/// <summary>How confident we are in an inferred value.</summary>
public enum Certainty
{
    /// <summary>Read directly from the driver or the OS.</summary>
    Known,
    /// <summary>Inferred from a lookup table; may be wrong for unlisted parts.</summary>
    Inferred,
    /// <summary>Could not be determined. Must never imply a capability either way.</summary>
    Unknown,
}

/// <summary>Whether a technology can be applied from outside the game.</summary>
public enum Integration
{
    /// <summary>The game must ship it; we can only configure what is already there.</summary>
    Native,
    /// <summary>We can apply it ourselves to any window. Spatial methods only.</summary>
    External,
    /// <summary>Not possible by any route available to this application.</summary>
    Unavailable,
}

/// <summary>
/// How far along a capability is in THIS build, independent of whether the
/// technology allows it at all.
/// </summary>
/// <remarks>
/// These are different questions and conflating them is how a UI ends up
/// claiming support that does not exist. <see cref="Integration"/> says what the
/// technology permits; Stage says what this application can currently do about
/// it. A row may be perfectly possible in principle and still be
/// <see cref="Preview"/> here.
/// </remarks>
public enum Stage
{
    /// <summary>Implemented and usable now, on the path the row describes.</summary>
    Shipping,

    /// <summary>
    /// The algorithm is implemented and tested, but not yet on the path the row
    /// describes — so the row must not be presented as ready to use.
    /// </summary>
    Preview,

    /// <summary>Not implemented in this build.</summary>
    NotImplemented,
}

/// <summary>A physical display adapter. Vendor, IDs, VRAM and driver come from WMI
/// and the driver registry key. Architecture is inferred — see <see cref="ArchCertainty"/>.</summary>
public sealed class GpuInfo
{
    public string Name { get; set; } = "Unknown GPU";
    public string Vendor { get; set; } = "Unknown";
    public string Arch { get; set; } = "Unknown";
    public int VendorId { get; set; }
    public int DeviceId { get; set; }
    public long VramBytes { get; set; }
    public string DriverVersion { get; set; } = "";

    /// <summary>
    /// How much to trust <see cref="Arch"/>. It is derived from a device-ID
    /// lookup, not read from the driver, so an unlisted part reads
    /// <see cref="Certainty.Unknown"/> and must not be used to rule a
    /// capability either in or out.
    /// </summary>
    public Certainty ArchCertainty { get; set; } = Certainty.Unknown;

    public string ArchDisplay => ArchCertainty switch
    {
        Certainty.Known => Arch,
        Certainty.Inferred => $"{Arch} (inferred)",
        _ => "Unknown",
    };

    public string VramDisplay =>
        VramBytes <= 0 ? "unknown" : $"{VramBytes / 1024d / 1024d / 1024d:0.0} GB";
}

/// <summary>Best-effort, honest view of the OS/graphics runtimes present.</summary>
public sealed class PlatformInfo
{
    public int Build { get; set; }
    public bool Win11 { get; set; }
    public bool DX11 { get; set; }
    public bool DX12 { get; set; }

    /// <summary>Highest D3D11 feature level accepted, e.g. "11_0". Empty if unprobed.</summary>
    public string Dx11FeatureLevel { get; set; } = "";

    /// <summary>
    /// The Vulkan loader DLL is present. This is NOT "Vulkan works" — it says
    /// nothing about whether a usable device exists, so it must never gate a
    /// capability. Named to stop that mistake being made again.
    /// </summary>
    public bool VulkanLoaderPresent { get; set; }
}

/// <summary>One upscaler and whether it can honestly be offered on this machine.</summary>
public sealed class CapabilityRow
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";

    /// <summary>
    /// True when this machine's hardware and OS do not block the technology.
    /// For <see cref="Integration.Native"/> rows this is NOT "you can use it" —
    /// the game still has to ship it. The UI must not render this as "Available"
    /// on its own.
    /// </summary>
    public bool HardwareOk { get; set; }

    /// <summary>
    /// Null when the hardware verdict could not be determined. Distinct from
    /// false: asserting absence from a guess is the same error as asserting
    /// presence.
    /// </summary>
    public bool? Determined { get; set; } = true;

    public Integration Integration { get; set; } = Integration.Native;

    /// <summary>How much of this row is actually implemented in this build.</summary>
    public Stage Stage { get; set; } = Stage.Shipping;

    public string Reason { get; set; } = "";
    public string Requirements { get; set; } = "";

    /// <summary>Short, honest pill text for this row.</summary>
    /// <remarks>
    /// <see cref="Stage"/> is checked before <see cref="Integration"/> on purpose.
    /// "Works on any app" is a statement about the technology; whether this build
    /// can do it is a separate fact, and the pill must report the second.
    /// </remarks>
    public string StatusText => Determined is null
        ? "Undetermined"
        : !HardwareOk ? "Not supported"
        : Stage == Stage.NotImplemented ? "Not implemented"
        : Stage == Stage.Preview ? "Preview only"
        : Integration == Integration.External ? "Ready to use"
        : "Hardware OK";

    /// <summary>Theme brush key for the pill.</summary>
    public string StatusBrush => Determined is null
        ? "Warn"
        : !HardwareOk ? "Bad"
        : Stage == Stage.NotImplemented ? "Bad"
        : Stage == Stage.Preview ? "Warn"
        : Integration == Integration.External ? "Ok"
        : "Accent";
}

/// <summary>One frame-generation technology and its honest state.</summary>
public sealed class FrameGenRow
{
    public string Name { get; set; } = "";
    public string State { get; set; } = "";
    public string Note { get; set; } = "";
}

/// <summary>A saved per-game configuration.</summary>
public sealed class GameProfile
{
    [JsonPropertyName("schema")] public int Schema { get; set; } = 1;
    [JsonPropertyName("name")] public string Name { get; set; } = "";
    [JsonPropertyName("executablePath")] public string ExecutablePath { get; set; } = "";
    [JsonPropertyName("api")] public string Api { get; set; } = "DirectX 12";
    [JsonPropertyName("upscaler")] public string Upscaler { get; set; } = "FSR3";
    [JsonPropertyName("quality")] public string Quality { get; set; } = "Quality";
    [JsonPropertyName("frameGen")] public string FrameGen { get; set; } = "None";
    [JsonPropertyName("frameGenEnabled")] public bool FrameGenEnabled { get; set; }
    [JsonPropertyName("sharpness")] public double Sharpness { get; set; } = 0.5;
    [JsonPropertyName("outputWidth")] public int OutputWidth { get; set; } = 2560;
    [JsonPropertyName("outputHeight")] public int OutputHeight { get; set; } = 1440;
}
