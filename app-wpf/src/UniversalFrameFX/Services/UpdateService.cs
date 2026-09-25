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

        /// <summary>
        /// URL of the <c>.sha256</c> sidecar the release workflow publishes next
        /// to the exe. Empty when the release has none, which is treated as a
        /// refusal to auto-update rather than as permission to skip the check.
        /// </summary>
        public string AssetSha256Url { get; init; } = "";
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

            string assetUrl = "", assetName = "", shaUrl = "";
            if (root.TryGetProperty("assets", out var assets) && assets.ValueKind == JsonValueKind.Array)
            {
                // Two passes: find the exe, then find the sidecar named exactly
                // "<exe>.sha256". Matching by name rather than by extension stops
                // a checksum for some other asset being accepted.
                foreach (var a in assets.EnumerateArray())
                {
                    var name = a.TryGetProperty("name", out var n) ? n.GetString() ?? "" : "";
                    if (name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase)
                        && a.TryGetProperty("browser_download_url", out var u))
                    {
                        assetName = name;
                        assetUrl = u.GetString() ?? "";
                        break;
                    }
                }

                if (assetName.Length > 0)
                {
                    var wanted = assetName + ".sha256";
                    foreach (var a in assets.EnumerateArray())
                    {
                        var name = a.TryGetProperty("name", out var n) ? n.GetString() ?? "" : "";
                        if (string.Equals(name, wanted, StringComparison.OrdinalIgnoreCase)
                            && a.TryGetProperty("browser_download_url", out var u))
                        {
                            shaUrl = u.GetString() ?? "";
                            break;
                        }
                    }
                }
            }

            return new UpdateInfo
            {
                AssetSha256Url = shaUrl,
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
    /// <summary>Raised when a download completes but its checksum does not match.</summary>
    public sealed class IntegrityException : Exception
    {
        public IntegrityException(string message) : base(message) { }
    }

    /// <summary>
    /// Downloads the update and verifies it against its published checksum.
    /// </summary>
    /// <remarks>
    /// The previous version executed whatever it downloaded with no verification
    /// at all, despite the release workflow already publishing a
    /// <c>.sha256</c> next to the exe. A missing or mismatched digest now
    /// deletes the file and throws, because "could not verify" must fail closed:
    /// the whole point of this path is that it runs an executable.
    /// </remarks>
    public static async Task<string> DownloadAsync(string url, IProgress<double>? progress,
                                                  string? sha256Url = null,
                                                  CancellationToken ct = default)
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

        await dst.FlushAsync(ct).ConfigureAwait(false);
        await dst.DisposeAsync().ConfigureAwait(false);

        await VerifyOrDeleteAsync(dest, sha256Url, client, ct).ConfigureAwait(false);
        return dest;
    }

    private static async Task VerifyOrDeleteAsync(string file, string? sha256Url,
                                                  HttpClient client, CancellationToken ct)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(sha256Url))
                throw new IntegrityException(
                    "This release does not publish a SHA-256 checksum, so the download "
                    + "cannot be verified. Install it manually from the releases page if "
                    + "you trust it.");

            var text = await client.GetStringAsync(sha256Url, ct).ConfigureAwait(false);
            var expected = Checksum.Parse(text);
            if (expected is null)
                throw new IntegrityException("The published checksum could not be parsed.");

            var actual = Checksum.OfFile(file);
            if (!Checksum.Match(expected, actual))
                throw new IntegrityException(
                    $"Checksum mismatch. Expected {expected}, got {actual}. "
                    + "The download was discarded and has NOT been run.");
        }
        catch
        {
            TryDelete(file);
            throw;
        }
    }

    private static void TryDelete(string path)
    {
        try { if (File.Exists(path)) File.Delete(path); }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
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
