using System.IO;
using System.Text.Json;

namespace UniversalFrameFX.Services;

/// <summary>Backs up a game folder's config files before any change, and can restore the latest snapshot.</summary>
public static class BackupService
{
    private static readonly string[] ConfigExtensions = { ".ini", ".cfg", ".json", ".xml" };

    public sealed class BackupMeta
    {
        public string Id { get; set; } = "";
        public string GameName { get; set; } = "";
        public string GameDir { get; set; } = "";
        public List<string> Files { get; set; } = new();
    }

    public static BackupMeta Backup(string gameName, string dir)
    {
        AppPaths.EnsureCreated();
        var id = DateTime.UtcNow.ToString("yyyyMMddTHHmmssZ");
        var snapRoot = Path.Combine(AppPaths.Backups, id);
        var snap = Path.Combine(snapRoot, "files");
        Directory.CreateDirectory(snap);

        var copied = new List<string>();
        foreach (var file in Directory.EnumerateFiles(dir, "*", SearchOption.AllDirectories))
        {
            if (Array.IndexOf(ConfigExtensions, Path.GetExtension(file).ToLowerInvariant()) < 0) continue;
            var rel = Path.GetRelativePath(dir, file);
            var dst = Path.Combine(snap, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(dst)!);
            File.Copy(file, dst, overwrite: true);
            copied.Add(rel);
        }

        var meta = new BackupMeta { Id = id, GameName = gameName, GameDir = dir, Files = copied };
        File.WriteAllText(Path.Combine(snapRoot, "backup.json"),
            JsonSerializer.Serialize(meta, new JsonSerializerOptions { WriteIndented = true }));
        return meta;
    }

    public static string? LatestSnapshotId()
    {
        if (!Directory.Exists(AppPaths.Backups)) return null;
        var dirs = Directory.GetDirectories(AppPaths.Backups);
        if (dirs.Length == 0) return null;
        Array.Sort(dirs, StringComparer.Ordinal);
        return Path.GetFileName(dirs[^1]);
    }

    public static void Restore(string id)
    {
        var snapRoot = Path.Combine(AppPaths.Backups, id);
        var metaFile = Path.Combine(snapRoot, "backup.json");
        if (!File.Exists(metaFile)) throw new FileNotFoundException($"Backup {id} not found.");
        var meta = JsonSerializer.Deserialize<BackupMeta>(File.ReadAllText(metaFile))
                   ?? throw new InvalidDataException("Backup metadata is unreadable.");
        var snap = Path.Combine(snapRoot, "files");
        foreach (var rel in meta.Files)
        {
            var src = Path.Combine(snap, rel);
            var dst = Path.Combine(meta.GameDir, rel);
            if (!File.Exists(src)) continue;
            Directory.CreateDirectory(Path.GetDirectoryName(dst)!);
            File.Copy(src, dst, overwrite: true);
        }
    }
}
