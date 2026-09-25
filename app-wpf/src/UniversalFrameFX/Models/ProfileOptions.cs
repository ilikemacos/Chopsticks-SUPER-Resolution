namespace UniversalFrameFX.Models;

/// <summary>
/// The upscaler values a profile's <c>upscaler</c> field may hold.
/// </summary>
/// <remarks>
/// One list, so the profile editor's combo and the web console's interop mapping
/// cannot drift apart. They did: the editor offered no CSR entry at all, so a
/// profile arriving with CSR could not be displayed and was rewritten on save.
/// These are stored in JSON that other tools read, so the strings are a contract
/// — add to them rather than renaming them.
/// </remarks>
public static class ProfileUpscalers
{
    /// <summary>CSR first: it is the only one that needs no game support.</summary>
    public static readonly string[] All =
    {
        "CSR", "None", "FSR1", "FSR2", "FSR3", "FSR4", "XeSS",
    };
}
