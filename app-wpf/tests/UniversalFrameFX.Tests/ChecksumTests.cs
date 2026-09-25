using System.Security.Cryptography;
using UniversalFrameFX.Services;
using Xunit;

namespace UniversalFrameFX.Tests;

/// <summary>
/// The auto-updater executes what it downloads, so these are the tests that stand
/// between a corrupted or substituted release asset and running it.
/// </summary>
public sealed class ChecksumTests
{
    private const string Valid = "f50d19737936e08c6867108642ec6bf866c15ada50f254056361398bd9b858a1";

    [Fact]
    public void ParsesBareDigest() => Assert.Equal(Valid, Checksum.Parse(Valid));

    [Fact]
    public void ParsesSha256sumFormat()
        => Assert.Equal(Valid, Checksum.Parse($"{Valid}  UniversalFrameFX.exe"));

    [Fact]
    public void ParsesWithBomAndTrailingNewline()
        => Assert.Equal(Valid, Checksum.Parse($"﻿{Valid}\r\n"));

    [Fact]
    public void ParsesUppercaseToLowercase()
        => Assert.Equal(Valid, Checksum.Parse(Valid.ToUpperInvariant()));

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    [InlineData("   ")]
    [InlineData("102")]                  // the real installer bug: a stray ASCII code
    [InlineData("nothex_nothex_nothex_nothex_nothex_nothex_nothex_nothex_nothex_x")]
    [InlineData("f50d1973")]             // truncated
    public void RejectsMalformed(string? text) => Assert.Null(Checksum.Parse(text));

    [Fact]
    public void RejectsDigestOfWrongLength()
        => Assert.Null(Checksum.Parse(Valid + "ff"));

    [Fact]
    public void ComputesKnownDigest()
    {
        // SHA-256 of the empty input is a fixed, published value.
        var path = Path.Combine(Path.GetTempPath(), $"csum-{Guid.NewGuid():N}");
        try
        {
            File.WriteAllBytes(path, Array.Empty<byte>());
            Assert.Equal("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                         Checksum.OfFile(path));
        }
        finally { File.Delete(path); }
    }

    [Fact]
    public void ComputesDigestMatchingDotNetCrypto()
    {
        var path = Path.Combine(Path.GetTempPath(), $"csum-{Guid.NewGuid():N}");
        try
        {
            var bytes = new byte[100_000];
            new Random(9).NextBytes(bytes);
            File.WriteAllBytes(path, bytes);

            var expected = Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();
            Assert.Equal(expected, Checksum.OfFile(path));
        }
        finally { File.Delete(path); }
    }

    [Fact]
    public void MatchIsCaseInsensitive()
        => Assert.True(Checksum.Match(Valid, Valid.ToUpperInvariant()));

    [Fact]
    public void MatchRejectsDifferentDigest()
    {
        var other = Valid[..63] + (Valid[63] == 'a' ? 'b' : 'a');
        Assert.False(Checksum.Match(Valid, other));
    }

    [Fact]
    public void MatchRejectsPrefix()
    {
        // A truncated digest must never satisfy a prefix comparison.
        Assert.False(Checksum.Match(Valid, Valid[..32]));
        Assert.False(Checksum.Match(Valid[..32], Valid));
    }

    [Theory]
    [InlineData(null, null)]
    [InlineData(null, "abc")]
    [InlineData("abc", null)]
    public void MatchRejectsNulls(string? a, string? b) => Assert.False(Checksum.Match(a, b));
}
