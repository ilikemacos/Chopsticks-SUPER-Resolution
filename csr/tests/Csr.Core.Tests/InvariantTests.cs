using Csr.Core;
using Xunit;

namespace Csr.Core.Tests;

/// <summary>
/// Invariants that must hold for any correct implementation, independent of the
/// golden vectors. These are the contracts that catch a structurally wrong port
/// even if it happens to agree on the sample images.
/// </summary>
public sealed class InvariantTests
{
    private static ImageBuffer Fill(int w, int h, float r, float g, float b)
    {
        var img = new ImageBuffer(w, h);
        for (int i = 0; i < img.Data.Length; i += 3)
        {
            img.Data[i] = r; img.Data[i + 1] = g; img.Data[i + 2] = b;
        }
        return img;
    }

    [Theory]
    [InlineData(0f, 0f, 0f)]
    [InlineData(1f, 1f, 1f)]
    [InlineData(0.2f, 0.6f, 0.9f)]
    public void SolidColourIsPreserved(float r, float g, float b)
    {
        // Catches weight-normalisation errors: if the kernel weights do not sum
        // correctly after normalisation, a flat field drifts off its value.
        var outp = CsrUpscaler.Resolve(Fill(48, 48, r, g, b), 72, 72);
        for (int i = 0; i < outp.Data.Length; i += 3)
        {
            Assert.Equal(r, outp.Data[i], 5);
            Assert.Equal(g, outp.Data[i + 1], 5);
            Assert.Equal(b, outp.Data[i + 2], 5);
        }
    }

    [Fact]
    public void OneToOneIsExactPassthrough()
    {
        // The resolve is a filter, not a pass-through, so the pipeline must
        // bypass it entirely at 1:1 rather than rely on it being neutral.
        var src = Fill(16, 16, 0.3f, 0.4f, 0.5f);
        src.Data[0] = 0.9f;
        var outp = CsrUpscaler.Upscale(src, 16, 16);
        Assert.Equal(src.Data, outp.Data);
    }

    [Fact]
    public void HardEdgeDoesNotOvershoot()
    {
        // The deringing clamp is what guarantees this. Without it a hard edge
        // rings past the source range.
        var src = new ImageBuffer(32, 32);
        for (int y = 0; y < 32; y++)
            for (int x = 16; x < 32; x++)
            {
                int i = (y * 32 + x) * 3;
                src.Data[i] = src.Data[i + 1] = src.Data[i + 2] = 1f;
            }

        var outp = CsrUpscaler.Upscale(src, 64, 64);
        foreach (var v in outp.Data)
        {
            Assert.True(v >= -1e-6f, $"undershoot: {v}");
            Assert.True(v <= 1f + 1e-6f, $"overshoot: {v}");
        }
    }

    [Fact]
    public void SharpenIsNoOpOnFlatRegion()
    {
        var src = Fill(32, 32, 0.45f, 0.45f, 0.45f);
        var outp = CsrUpscaler.Sharpen(src);
        foreach (var v in outp.Data) Assert.Equal(0.45f, v, 5);
    }

    [Fact]
    public void SharpenCannotClip()
    {
        var rng = new Random(7);
        var src = new ImageBuffer(64, 64);
        for (int i = 0; i < src.Data.Length; i++) src.Data[i] = (float)rng.NextDouble();
        var outp = CsrUpscaler.Sharpen(src, new CsrOptions { SharpnessStops = 0f });
        foreach (var v in outp.Data)
        {
            Assert.True(v >= 0f, $"below range: {v}");
            Assert.True(v <= 1f, $"above range: {v}");
        }
    }

    [Fact]
    public void MonotonicRampStaysMonotonic()
    {
        var src = new ImageBuffer(64, 16);
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 64; x++)
            {
                int i = (y * 64 + x) * 3;
                float v = x / 63f;
                src.Data[i] = src.Data[i + 1] = src.Data[i + 2] = v;
            }

        var outp = CsrUpscaler.Resolve(src, 128, 16);
        for (int y = 0; y < 16; y++)
            for (int x = 1; x < 128; x++)
            {
                float prev = outp.Data[(y * 128 + x - 1) * 3];
                float cur = outp.Data[(y * 128 + x) * 3];
                Assert.True(cur >= prev - 1e-5f,
                    $"reversal at ({x},{y}): {prev} -> {cur}");
            }
    }

    [Fact]
    public void IsDeterministic()
    {
        var rng = new Random(11);
        var src = new ImageBuffer(32, 32);
        for (int i = 0; i < src.Data.Length; i++) src.Data[i] = (float)rng.NextDouble();
        Assert.Equal(CsrUpscaler.Upscale(src, 48, 48).Data,
                     CsrUpscaler.Upscale(src, 48, 48).Data);
    }

    [Fact]
    public void RejectsMismatchedBufferLength()
        => Assert.Throws<ArgumentException>(() => new ImageBuffer(4, 4, new float[10]));
}
