using Microsoft.Win32;

namespace UniversalFrameFX.Services;

/// <summary>
/// Reads and writes the per-application GPU preference Windows itself uses
/// (Settings ▸ Display ▸ Graphics). It sets HKCU\...\DirectX\UserGpuPreferences
/// so a game launches on the chosen adapter — the same, supported mechanism
/// Windows exposes, not a driver hack.
/// </summary>
public static class GpuPreferenceService
{
    private const string KeyPath = @"Software\Microsoft\DirectX\UserGpuPreferences";

    public enum Preference
    {
        SystemDefault = 0, // Let Windows decide
        PowerSaving = 1,   // Integrated GPU
        HighPerformance = 2, // Discrete GPU
    }

    public static string Describe(Preference p) => p switch
    {
        Preference.PowerSaving => "Power saving (integrated GPU)",
        Preference.HighPerformance => "High performance (discrete GPU)",
        _ => "Let Windows decide",
    };

    /// <summary>Reads the stored preference for an executable path, or SystemDefault if none.</summary>
    public static Preference Get(string exePath)
    {
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(KeyPath);
            if (key?.GetValue(exePath) is string val)
            {
                var idx = val.IndexOf("GpuPreference=", StringComparison.OrdinalIgnoreCase);
                if (idx >= 0)
                {
                    var rest = val[(idx + "GpuPreference=".Length)..];
                    var digits = new string(rest.TakeWhile(char.IsDigit).ToArray());
                    if (int.TryParse(digits, out var n) && Enum.IsDefined(typeof(Preference), n))
                        return (Preference)n;
                }
            }
        }
        catch { /* not set / unreadable */ }
        return Preference.SystemDefault;
    }

    /// <summary>Sets (or clears) the GPU preference for an executable path.</summary>
    public static void Set(string exePath, Preference pref)
    {
        if (string.IsNullOrWhiteSpace(exePath))
            throw new ArgumentException("An executable path is required.", nameof(exePath));

        using var key = Registry.CurrentUser.CreateSubKey(KeyPath, writable: true)
                        ?? throw new InvalidOperationException("Could not open the GPU preferences key.");

        if (pref == Preference.SystemDefault)
        {
            // Removing the value restores Windows' automatic choice.
            if (key.GetValue(exePath) != null) key.DeleteValue(exePath, throwOnMissingValue: false);
            return;
        }

        key.SetValue(exePath, $"GpuPreference={(int)pref};", RegistryValueKind.String);
    }
}
