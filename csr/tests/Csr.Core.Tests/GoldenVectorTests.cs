using System.Text.Json;
using Csr.Core;
using Xunit;

namespace Csr.Core.Tests;

/// <summary>
/// Verifies the C# port against the Python reference in <c>csr/ref</c>, which is
/// the specification. Any failure here means the port diverged — historically
/// the likely culprits are the luma weights, the deringing clamp, or edge
/// handling at frame borders.
/// </summary>
public sealed class GoldenVectorTests
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

    private sealed record IndexEntry(string name);
    private sealed record GoldenIndex(IndexEntry[] cases);

    /// <summary>
    /// Reads the case list from <c>index.json</c> rather than globbing the folder.
    /// </summary>
    /// <remarks>
    /// Globbing silently swallowed any other JSON written into the folder and
    /// tried to parse it as a CSR case — a missing case would have read as a pass,
    /// and an unrelated one (the bilinear baseline, say) as a spurious failure.
    /// The index is the manifest the exporter writes, so a case that is exported
    /// is a case that runs.
    /// </remarks>
    public static TheoryData<string> Cases()
    {
        var json = File.ReadAllText(Path.Combine(GoldenDir(), "index.json"));
        var index = JsonSerializer.Deserialize<GoldenIndex>(json,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
            ?? throw new InvalidDataException("could not parse golden/index.json");
        Assert.NotEmpty(index.cases);

        var data = new TheoryData<string>();
        foreach (var c in index.cases.OrderBy(c => c.name, StringComparer.Ordinal))
        {
            var file = Path.Combine(GoldenDir(), c.name + ".json");
            if (!File.Exists(file))
                throw new FileNotFoundException(
                    $"golden/index.json lists '{c.name}' but {c.name}.json is missing. "
                    + "Run 'python3 export_golden.py' in csr/ref.");
            data.Add(c.name);
        }
        return data;
    }

    private sealed record Plane(int width, int height, float[] rgb);
    private sealed record Case(string name, float tolerance, Plane input,
                               Plane resolve, Plane full);

    private static Case Load(string name)
    {
        var json = File.ReadAllText(Path.Combine(GoldenDir(), name + ".json"));
        return JsonSerializer.Deserialize<Case>(json,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
            ?? throw new InvalidDataException($"could not parse {name}.json");
    }

    private static void AssertMatches(string label, Plane expected, ImageBuffer actual,
                                      float tolerance)
    {
        Assert.Equal(expected.width, actual.Width);
        Assert.Equal(expected.height, actual.Height);

        float worst = 0f;
        int worstIdx = -1;
        for (int i = 0; i < expected.rgb.Length; i++)
        {
            float diff = MathF.Abs(expected.rgb[i] - actual.Data[i]);
            if (diff > worst) { worst = diff; worstIdx = i; }
        }

        if (worst > tolerance)
        {
            int px = worstIdx / 3, c = worstIdx % 3;
            Assert.Fail($"{label}: worst deviation {worst:F6} exceeds tolerance " +
                        $"{tolerance:F6} at pixel ({px % actual.Width},{px / actual.Width}) " +
                        $"channel {c}: expected {expected.rgb[worstIdx]:F6}, " +
                        $"got {actual.Data[worstIdx]:F6}");
        }
    }

    [Theory]
    [MemberData(nameof(Cases))]
    public void ResolveMatchesReference(string name)
    {
        var c = Load(name);
        var src = new ImageBuffer(c.input.width, c.input.height, c.input.rgb);
        var actual = CsrUpscaler.Resolve(src, c.resolve.width, c.resolve.height);
        AssertMatches($"{name} resolve", c.resolve, actual, c.tolerance);
    }

    [Theory]
    [MemberData(nameof(Cases))]
    public void FullPipelineMatchesReference(string name)
    {
        var c = Load(name);
        var src = new ImageBuffer(c.input.width, c.input.height, c.input.rgb);
        var actual = CsrUpscaler.Upscale(src, c.full.width, c.full.height);
        AssertMatches($"{name} full", c.full, actual, c.tolerance);
    }
}
