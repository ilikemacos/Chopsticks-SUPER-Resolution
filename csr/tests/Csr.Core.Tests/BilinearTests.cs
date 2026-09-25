using System.Text.Json;
using Csr.Core;
using Xunit;

namespace Csr.Core.Tests;

/// <summary>
/// Verifies the bilinear baseline against the same Python reference, so that a
/// "CSR versus bilinear" comparison anywhere in the product is against the exact
/// filter the published quality numbers were measured with.
/// </summary>
public sealed class BilinearTests
{
    private static string GoldenDir()
    {
        var dir = AppContext.BaseDirectory;
        for (int i = 0; i < 8 && dir is not null; i++)
        {
            var candidate = Path.Combine(dir, "ref", "golden");
            if (Directory.Exists(candidate)) return candidate;
            dir = Path.GetDirectoryName(dir);
        }
        throw new DirectoryNotFoundException(
            "csr/ref/golden not found. Run 'python3 export_golden.py' in csr/ref.");
    }

    private sealed record Plane(int width, int height, float[] rgb);
    private sealed record Case(string name, Plane input, Plane bilinear);
    private sealed record Baseline(float tolerance, Case[] cases);

    private static Baseline Load()
    {
        var json = File.ReadAllText(Path.Combine(GoldenDir(), "bilinear_baseline.json"));
        return JsonSerializer.Deserialize<Baseline>(json,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
            ?? throw new InvalidDataException("could not parse bilinear_baseline.json");
    }

    [Fact]
    public void MatchesThePythonReferenceOnEveryCase()
    {
        var baseline = Load();
        Assert.NotEmpty(baseline.cases);

        foreach (var c in baseline.cases)
        {
            var src = new ImageBuffer(c.input.width, c.input.height, c.input.rgb);
            var actual = Bilinear.Upscale(src, c.bilinear.width, c.bilinear.height);

            Assert.Equal(c.bilinear.width, actual.Width);
            Assert.Equal(c.bilinear.height, actual.Height);

            float worst = 0f;
            for (int i = 0; i < c.bilinear.rgb.Length; i++)
                worst = MathF.Max(worst, MathF.Abs(c.bilinear.rgb[i] - actual.Data[i]));

            Assert.True(worst <= baseline.tolerance,
                $"{c.name}: worst channel difference {worst:F6} exceeds {baseline.tolerance:F6}");
        }
    }

    [Fact]
    public void SolidColourIsPreservedExactly()
    {
        var src = new ImageBuffer(5, 4);
        for (int i = 0; i < src.Data.Length; i += 3)
        {
            src.Data[i] = 0.25f; src.Data[i + 1] = 0.5f; src.Data[i + 2] = 0.75f;
        }

        var outp = Bilinear.Upscale(src, 11, 9);
        for (int i = 0; i < outp.Data.Length; i += 3)
        {
            Assert.Equal(0.25f, outp.Data[i], 6);
            Assert.Equal(0.5f, outp.Data[i + 1], 6);
            Assert.Equal(0.75f, outp.Data[i + 2], 6);
        }
    }

    [Fact]
    public void SameSizeIsIdentity()
    {
        var rng = new Random(7);
        var src = new ImageBuffer(9, 6);
        for (int i = 0; i < src.Data.Length; i++) src.Data[i] = (float)rng.NextDouble();

        var outp = Bilinear.Upscale(src, 9, 6);
        for (int i = 0; i < src.Data.Length; i++)
            Assert.Equal(src.Data[i], outp.Data[i], 6);
    }

    [Fact]
    public void StaysWithinTheInputRange()
    {
        // Bilinear is a convex combination, so it cannot overshoot. A failure here
        // would mean the weights do not sum to one.
        var rng = new Random(3);
        var src = new ImageBuffer(7, 7);
        for (int i = 0; i < src.Data.Length; i++) src.Data[i] = 0.2f + 0.6f * (float)rng.NextDouble();

        float lo = src.Data.Min(), hi = src.Data.Max();
        var outp = Bilinear.Upscale(src, 15, 13);
        foreach (var v in outp.Data)
        {
            Assert.True(v >= lo - 1e-6f, $"{v} below input minimum {lo}");
            Assert.True(v <= hi + 1e-6f, $"{v} above input maximum {hi}");
        }
    }

    [Fact]
    public void RejectsNonPositiveOutputDimensions()
    {
        var src = new ImageBuffer(4, 4);
        Assert.Throws<ArgumentOutOfRangeException>(() => Bilinear.Upscale(src, 0, 4));
        Assert.Throws<ArgumentOutOfRangeException>(() => Bilinear.Upscale(src, 4, -1));
    }
}
