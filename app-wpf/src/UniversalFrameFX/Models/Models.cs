using System.Text.Json.Serialization;

namespace UniversalFrameFX.Models;

/// <summary>A physical display adapter, detected from WMI + the driver, never guessed from the brand.</summary>
public sealed class GpuInfo
{
    public string Name { get; set; } = "Unknown GPU";
    public string Vendor { get; set; } = "Unknown";
    public string Arch { get; set; } = "Unknown";
    public int VendorId { get; set; }
    public int DeviceId { get; set; }
    public long VramBytes { get; set; }
    public string DriverVersion { get; set; } = "";

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
    public bool Vulkan { get; set; }
}

/// <summary>One upscaler and whether it can honestly be offered on this machine.</summary>
public sealed class CapabilityRow
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public bool Available { get; set; }
    public string Reason { get; set; } = "";
    public string Requirements { get; set; } = "";
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
