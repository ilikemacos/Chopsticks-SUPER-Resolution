using Csr.Core;
using Xunit;

namespace Csr.Core.Tests;

/// <summary>
/// A swapped red/blue channel is invisible on greyscale and on symmetric content,
/// so these use deliberately asymmetric colours where a swap cannot hide.
/// </summary>
public sealed class Bgra32Tests
{
    [Fact]
    public void DecodeMapsChannelsInBgraOrder()
    {
        // One pixel: B=10, G=20, R=30, A=255.
        byte[] bgra = { 10, 20, 30, 255 };
        var img = Bgra32.Decode(bgra, 1, 1, 4);

        Assert.Equal(30 / 255f, img.Data[0], 6); // R
        Assert.Equal(20 / 255f, img.Data[1], 6); // G
        Assert.Equal(10 / 255f, img.Data[2], 6); // B
    }

    [Fact]
    public void EncodeMapsChannelsInBgraOrder()
    {
        var img = new ImageBuffer(1, 1);
        img.Data[0] = 30 / 255f; // R
        img.Data[1] = 20 / 255f; // G
        img.Data[2] = 10 / 255f; // B

        var bytes = Bgra32.Encode(img, out var stride);
        Assert.Equal(4, stride);
        Assert.Equal(10, bytes[0]); // B
        Assert.Equal(20, bytes[1]); // G
        Assert.Equal(30, bytes[2]); // R
        Assert.Equal(255, bytes[3]);
    }

    [Fact]
    public void RoundTripIsLossless()
    {
        // Every byte value must survive decode -> encode exactly, which only
        // holds if the quantiser rounds rather than truncates.
        var rng = new Random(5);
        int w = 17, h = 9, stride = w * 4;
        var original = new byte[stride * h];
        for (int i = 0; i < original.Length; i += 4)
        {
            original[i] = (byte)rng.Next(256);
            original[i + 1] = (byte)rng.Next(256);
            original[i + 2] = (byte)rng.Next(256);
            original[i + 3] = 255;
        }

        var round = Bgra32.Encode(Bgra32.Decode(original, w, h, stride), out var outStride);
        Assert.Equal(stride, outStride);
        Assert.Equal(original, round);
    }

    [Fact]
    public void RoundTripCoversEveryByteValue()
    {
        var img = new ImageBuffer(256, 1);
        for (int v = 0; v < 256; v++)
        {
            img.Data[v * 3] = v / 255f;
            img.Data[v * 3 + 1] = v / 255f;
            img.Data[v * 3 + 2] = v / 255f;
        }
        var bytes = Bgra32.Encode(img, out _);
        for (int v = 0; v < 256; v++)
            Assert.Equal((byte)v, bytes[v * 4]);
    }

    [Fact]
    public void DecodeHonoursStridePadding()
    {
        // Windows imaging rows are padded, so a stride wider than width*4 must be
        // respected or every row after the first is skewed.
        int w = 2, h = 2, stride = 12; // 8 bytes of pixels + 4 bytes padding
        var bgra = new byte[stride * h];
        bgra[0] = 1; bgra[1] = 2; bgra[2] = 3;        // row 0, px 0
        bgra[4] = 4; bgra[5] = 5; bgra[6] = 6;        // row 0, px 1
        bgra[stride] = 7; bgra[stride + 1] = 8; bgra[stride + 2] = 9; // row 1, px 0

        var img = Bgra32.Decode(bgra, w, h, stride);
        Assert.Equal(3 / 255f, img.Data[0], 6);  // px(0,0) R
        Assert.Equal(6 / 255f, img.Data[3], 6);  // px(1,0) R
        Assert.Equal(9 / 255f, img.Data[6], 6);  // px(0,1) R — wrong if stride ignored
    }

    [Fact]
    public void EncodeClampsOutOfRangeAndNaN()
    {
        var img = new ImageBuffer(3, 1);
        img.Data[0] = -5f; img.Data[1] = 2f; img.Data[2] = float.NaN;
        var bytes = Bgra32.Encode(img, out _);
        Assert.Equal(0, bytes[2]);    // R clamped from -5
        Assert.Equal(255, bytes[1]);  // G clamped from 2
        Assert.Equal(0, bytes[0]);    // B from NaN
    }

    [Fact]
    public void DecodeRejectsTooSmallStride()
        => Assert.Throws<ArgumentOutOfRangeException>(
            () => Bgra32.Decode(new byte[16], 4, 1, 8));

    [Fact]
    public void DecodeRejectsShortBuffer()
        => Assert.Throws<ArgumentException>(
            () => Bgra32.Decode(new byte[8], 4, 4, 16));
}
