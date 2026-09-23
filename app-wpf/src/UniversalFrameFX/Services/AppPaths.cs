using System.IO;

namespace UniversalFrameFX.Services;

/// <summary>Per-user data locations under %APPDATA%\UniversalFrameFX.</summary>
public static class AppPaths
{
    public static string Root { get; } = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "UniversalFrameFX");

    public static string Profiles { get; } = Path.Combine(Root, "profiles");
    public static string Backups { get; } = Path.Combine(Root, "backups");
    public static string Logs { get; } = Path.Combine(Root, "logs");

    public static void EnsureCreated()
    {
        foreach (var d in new[] { Root, Profiles, Backups, Logs })
        {
            try { Directory.CreateDirectory(d); } catch { /* best effort */ }
        }
    }
}
