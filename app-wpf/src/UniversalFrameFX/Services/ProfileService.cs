using System.IO;
using System.Text.Json;
using System.Text.RegularExpressions;
using UniversalFrameFX.Models;

namespace UniversalFrameFX.Services;

/// <summary>Loads and saves per-game profiles as JSON under %APPDATA%\UniversalFrameFX\profiles.</summary>
public static class ProfileService
{
    private static readonly JsonSerializerOptions Options = new()
    {
        WriteIndented = true,
        PropertyNameCaseInsensitive = true,
    };

    public static string Slug(string name)
    {
        var s = Regex.Replace(name.ToLowerInvariant(), "[^a-z0-9]+", "-").Trim('-');
        return string.IsNullOrWhiteSpace(s) ? "profile" : s;
    }

    public static List<GameProfile> Load()
    {
        var list = new List<GameProfile>();
        try
        {
            if (!Directory.Exists(AppPaths.Profiles)) return list;
            foreach (var file in Directory.GetFiles(AppPaths.Profiles, "*.json"))
            {
                try
                {
                    var p = JsonSerializer.Deserialize<GameProfile>(File.ReadAllText(file), Options);
                    if (p != null && !string.IsNullOrWhiteSpace(p.Name)) list.Add(p);
                }
                catch { /* skip malformed profile */ }
            }
        }
        catch { /* directory unreadable */ }
        list.Sort((a, b) => string.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase));
        return list;
    }

    public static void Save(GameProfile p)
    {
        AppPaths.EnsureCreated();
        var file = Path.Combine(AppPaths.Profiles, Slug(p.Name) + ".json");
        File.WriteAllText(file, JsonSerializer.Serialize(p, Options));
    }

    public static void Delete(string name)
    {
        var file = Path.Combine(AppPaths.Profiles, Slug(name) + ".json");
        if (File.Exists(file)) File.Delete(file);
    }

    public static GameProfile? Read(string path)
        => JsonSerializer.Deserialize<GameProfile>(File.ReadAllText(path), Options);

    public static void Export(GameProfile p, string path)
        => File.WriteAllText(path, JsonSerializer.Serialize(p, Options));
}
