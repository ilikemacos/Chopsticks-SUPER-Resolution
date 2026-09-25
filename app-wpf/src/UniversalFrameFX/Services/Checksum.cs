using System.IO;
using System.Security.Cryptography;

namespace UniversalFrameFX.Services;

/// <summary>
/// SHA-256 helpers for verifying downloaded files.
/// </summary>
/// <remarks>
/// Deliberately free of any WPF, HTTP or app-state dependency so it can be
/// unit-tested on any platform — see <c>app-wpf/tests/UniversalFrameFX.Tests</c>,
/// which links this file directly rather than testing a copy of it.
/// </remarks>
internal static class Checksum
{
    /// <summary>
    /// Parses a <c>sha256sum</c>-style line into a bare lowercase hex digest, or
    /// null if it is not a valid digest.
    /// </summary>
    /// <remarks>
    /// The sidecar may be "&lt;hex&gt;", "&lt;hex&gt;  filename", or carry a BOM
    /// and a trailing newline. The PowerShell installer once shipped a bug in
    /// exactly this parse — it decoded raw bytes as text and split an ASCII code
    /// out of them, producing "102" as the expected hash — so this validates the
    /// shape rather than trusting it.
    /// </remarks>
    internal static string? Parse(string? text)
    {
        if (string.IsNullOrWhiteSpace(text)) return null;

        var token = text.TrimStart('﻿')
                        .Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries)
                        .FirstOrDefault();
        if (token is null) return null;

        token = token.Trim().ToLowerInvariant();
        if (token.Length != 64) return null;
        foreach (var ch in token)
        {
            bool hex = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
            if (!hex) return null;
        }
        return token;
    }

    /// <summary>Computes a file's SHA-256 as lowercase hex.</summary>
    internal static string OfFile(string path)
    {
        using var stream = File.OpenRead(path);
        using var sha = SHA256.Create();
        return Convert.ToHexString(sha.ComputeHash(stream)).ToLowerInvariant();
    }

    /// <summary>
    /// Compares two hex digests. Length-checked first so a truncated digest can
    /// never match a prefix.
    /// </summary>
    internal static bool Match(string? expected, string? actual)
        => expected is not null
           && actual is not null
           && expected.Length == actual.Length
           && string.Equals(expected, actual, StringComparison.OrdinalIgnoreCase);
}
