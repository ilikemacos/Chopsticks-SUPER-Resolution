using System.IO;

namespace UniversalFrameFX.Services;

/// <summary>Refuses to touch folders that carry anti-cheat, so we never risk a ban on an online game.</summary>
public static class AntiCheatScanner
{
    private static readonly string[] Markers =
    {
        "EasyAntiCheat", "BEService", "BattlEye", "beclient", "vgk.sys", "anticheat",
    };

    /// <summary>Returns the marker name if anti-cheat is detected, otherwise null.</summary>
    public static string? Detect(string dir)
    {
        try
        {
            foreach (var file in Directory.EnumerateFiles(dir, "*", SearchOption.AllDirectories))
            {
                var name = Path.GetFileName(file);
                foreach (var m in Markers)
                {
                    if (name.Contains(m, StringComparison.OrdinalIgnoreCase)) return m;
                }
            }
        }
        catch { /* unreadable subtrees are ignored */ }
        return null;
    }
}
