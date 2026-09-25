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

        /// <summary>UTC time the snapshot was taken, parsed back from <see cref="Id"/>.</summary>
        public DateTime TakenUtc =>
            DateTime.TryParseExact(Id, "yyyyMMddTHHmmssZ",
                System.Globalization.CultureInfo.InvariantCulture,
                System.Globalization.DateTimeStyles.AssumeUniversal
                | System.Globalization.DateTimeStyles.AdjustToUniversal,
                out var t) ? t : DateTime.MinValue;

        /// <summary>One-line description for a picker, naming the target directory.</summary>
        public string Describe =>
            $"{(string.IsNullOrWhiteSpace(GameName) ? "(unnamed)" : GameName)}  ·  "
            + $"{(TakenUtc == DateTime.MinValue ? Id : TakenUtc.ToLocalTime().ToString("yyyy-MM-dd HH:mm"))}"
            + $"  ·  {Files.Count} file(s)  ·  {GameDir}";
    }

    public static BackupMeta Backup(string gameName, string dir)
    {
        AppPaths.EnsureCreated();
        var id = DateTime.UtcNow.ToString("yyyyMMddTHHmmssZ");
        var snapRoot = Path.Combine(AppPaths.Backups, id);
        // Second granularity means two backups in the same second would land in
        // the same directory and merge. Disambiguate rather than silently mix
        // two games' files into one snapshot.
        if (Directory.Exists(snapRoot))
        {
            for (int n = 2; n < 1000; n++)
            {
                var candidate = $"{id}-{n}";
                var root = Path.Combine(AppPaths.Backups, candidate);
                if (Directory.Exists(root)) continue;
                id = candidate;
                snapRoot = root;
                break;
            }
        }
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

    /// <summary>
    /// Every snapshot, newest first, with its metadata.
    /// </summary>
    /// <remarks>
    /// Restore needs this. Previously the only accessor was
    /// <see cref="LatestSnapshotId"/>, and the UI restored that blindly — which
    /// writes into whichever <c>GameDir</c> the newest snapshot happens to
    /// record, not the game the user is looking at. Callers must be able to see
    /// and choose the target.
    /// </remarks>
    public static List<BackupMeta> ListSnapshots()
    {
        var list = new List<BackupMeta>();
        if (!Directory.Exists(AppPaths.Backups)) return list;

        foreach (var dir in Directory.GetDirectories(AppPaths.Backups))
        {
            var metaFile = Path.Combine(dir, "backup.json");
            if (!File.Exists(metaFile)) continue;
            try
            {
                var meta = JsonSerializer.Deserialize<BackupMeta>(File.ReadAllText(metaFile));
                if (meta is not null && !string.IsNullOrEmpty(meta.Id)) list.Add(meta);
            }
            catch (JsonException) { /* skip a corrupt snapshot rather than fail the list */ }
            catch (IOException) { }
        }

        list.Sort((a, b) => string.CompareOrdinal(b.Id, a.Id));
        return list;
    }

    /// <summary>Reads one snapshot's metadata, or null if it is missing or unreadable.</summary>
    public static BackupMeta? ReadSnapshot(string id)
    {
        var metaFile = Path.Combine(AppPaths.Backups, id, "backup.json");
        if (!File.Exists(metaFile)) return null;
        try
        {
            return JsonSerializer.Deserialize<BackupMeta>(File.ReadAllText(metaFile));
        }
        catch (JsonException) { return null; }
        catch (IOException) { return null; }
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
        if (string.IsNullOrWhiteSpace(meta.GameDir))
            throw new InvalidDataException($"Backup {id} records no target directory.");

        // Refuse to write outside the recorded game directory. The relative paths
        // come from a file on disk, so a hand-edited or corrupted manifest
        // containing "..\.." must not be able to overwrite arbitrary files.
        var root = Path.GetFullPath(meta.GameDir);
        var rootPrefix = root.EndsWith(Path.DirectorySeparatorChar)
            ? root : root + Path.DirectorySeparatorChar;

        foreach (var rel in meta.Files)
        {
            var src = Path.Combine(snap, rel);
            if (!File.Exists(src)) continue;

            var dst = Path.GetFullPath(Path.Combine(root, rel));
            if (!dst.StartsWith(rootPrefix, StringComparison.OrdinalIgnoreCase)
                && !string.Equals(dst, root, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(
                    $"Backup {id} contains a path that escapes the game folder: {rel}");
            }

            Directory.CreateDirectory(Path.GetDirectoryName(dst)!);
            File.Copy(src, dst, overwrite: true);
        }
    }
}
