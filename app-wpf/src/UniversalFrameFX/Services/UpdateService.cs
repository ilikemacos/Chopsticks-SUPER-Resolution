using System.IO;
using System.Net.Http;
using System.Text.Json;

namespace UniversalFrameFX.Services;

/// <summary>Checks GitHub Releases for a newer build and can download + launch it.</summary>
public static class UpdateService
{
    private const string LatestApi =
        "https://api.github.com/repos/ilikemacos/Chopsticks-SUPER-Resolution/releases/latest";

    public sealed class UpdateInfo
    {
        public string Version { get; init; } = "";
        public string TagName { get; init; } = "";
        public string AssetUrl { get; init; } = "";
        public string HtmlUrl { get; init; } = "";
    }

    private static HttpClient CreateClient()
    {
        var c = new HttpClient { Timeout = TimeSpan.FromSeconds(30) };
        c.DefaultRequestHeaders.UserAgent.ParseAdd("UniversalFrameFX/" + AppState.Version);
        c.DefaultRequestHeaders.Accept.ParseAdd("application/vnd.github+json");
        return c;
    }

    /// <summary>Returns update info when a newer release exists, otherwise null.</summary>
    public static async Task<UpdateInfo?> CheckAsync(CancellationToken ct = default)
    {
        try
        {
            using var client = CreateClient();
            var json = await client.GetStringAsync(LatestApi, ct).ConfigureAwait(false);
            using var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;

            var tag = root.TryGetProperty("tag_name", out var t) ? t.GetString() ?? "" : "";
            if (string.IsNullOrWhiteSpace(tag)) return null;
            if (Compare(Normalize(tag), Normalize(AppState.Version)) <= 0) return null;

            string assetUrl = "";
            if (root.TryGetProperty("assets", out var assets) && assets.ValueKind == JsonValueKind.Array)
            {
                foreach (var a in assets.EnumerateArray())
                {
                    var name = a.TryGetProperty("name", out var n) ? n.GetString() ?? "" : "";
                    if (name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase)
                        && a.TryGetProperty("browser_download_url", out var u))
                    {
                        assetUrl = u.GetString() ?? "";
                        break;
                    }
                }
            }

            return new UpdateInfo
            {
                Version = Normalize(tag),
                TagName = tag,
                AssetUrl = assetUrl,
                HtmlUrl = root.TryGetProperty("html_url", out var h) ? h.GetString() ?? "" : "",
            };
        }
        catch
        {
            return null; // Offline or API hiccup — never bother the user.
        }
    }

    /// <summary>Downloads the new exe to a temp file and returns its path.</summary>
    public static async Task<string> DownloadAsync(string url, IProgress<double>? progress, CancellationToken ct = default)
    {
        var dest = Path.Combine(Path.GetTempPath(), $"UniversalFrameFX-update-{Guid.NewGuid():N}.exe");
        using var client = CreateClient();
        using var resp = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, ct).ConfigureAwait(false);
        resp.EnsureSuccessStatusCode();

        var total = resp.Content.Headers.ContentLength ?? -1L;
        await using var src = await resp.Content.ReadAsStreamAsync(ct).ConfigureAwait(false);
        await using var dst = new FileStream(dest, FileMode.Create, FileAccess.Write, FileShare.None);

        var buffer = new byte[81920];
        long read = 0;
        int n;
        while ((n = await src.ReadAsync(buffer, ct).ConfigureAwait(false)) > 0)
        {
            await dst.WriteAsync(buffer.AsMemory(0, n), ct).ConfigureAwait(false);
            read += n;
            if (total > 0) progress?.Report((double)read / total);
        }
        return dest;
    }

    private static string Normalize(string v) => v.TrimStart('v', 'V').Trim();

    /// <summary>Compares dotted numeric versions; returns >0 if a is newer than b.</summary>
    private static int Compare(string a, string b)
    {
        var pa = a.Split('.');
        var pb = b.Split('.');
        int len = Math.Max(pa.Length, pb.Length);
        for (int i = 0; i < len; i++)
        {
            int x = i < pa.Length && int.TryParse(pa[i], out var xi) ? xi : 0;
            int y = i < pb.Length && int.TryParse(pb[i], out var yi) ? yi : 0;
            if (x != y) return x.CompareTo(y);
        }
        return 0;
    }
}
