namespace Csr.Core;

/// <summary>
/// Conversions between packed BGRA32 (what Windows imaging and D3D hand back) and
/// <see cref="ImageBuffer"/>.
/// </summary>
/// <remarks>
/// This lives here, tested, rather than inline in a view, because a swapped red
/// and blue channel is invisible on greyscale test content and on anything
/// symmetric — exactly the kind of bug that ships. See <c>Bgra32Tests</c>.
/// </remarks>
public static class Bgra32
{
    /// <summary>
    /// Unpacks BGRA32 into float RGB in [0,1].
    /// </summary>
    /// <remarks>
    /// The bytes are assumed sRGB-encoded, which is what CSR expects. They must
    /// not be linearised first; doing so makes edges halo.
    /// </remarks>
    public static ImageBuffer Decode(ReadOnlySpan<byte> bgra, int width, int height, int stride)
    {
        if (width <= 0 || height <= 0)
            throw new ArgumentOutOfRangeException(nameof(width), "dimensions must be positive");
        if (stride < width * 4)
            throw new ArgumentOutOfRangeException(nameof(stride),
                $"stride {stride} is too small for {width} BGRA pixels");
        if (bgra.Length < (long)stride * (height - 1) + width * 4)
            throw new ArgumentException("buffer is shorter than the stated dimensions", nameof(bgra));

        var img = new ImageBuffer(width, height);
        var data = img.Data;
        for (int y = 0; y < height; y++)
        {
            int row = y * stride;
            int dst = y * width * 3;
            for (int x = 0; x < width; x++)
            {
                int s = row + x * 4;
                int d = dst + x * 3;
                data[d] = bgra[s + 2] * (1f / 255f);     // R
                data[d + 1] = bgra[s + 1] * (1f / 255f); // G
                data[d + 2] = bgra[s] * (1f / 255f);     // B
            }
        }
        return img;
    }

    /// <summary>Packs float RGB back to BGRA32 with opaque alpha, round-to-nearest.</summary>
    public static byte[] Encode(ImageBuffer img, out int stride)
    {
        ArgumentNullException.ThrowIfNull(img);
        stride = img.Width * 4;
        var bytes = new byte[stride * img.Height];
        var data = img.Data;

        for (int y = 0; y < img.Height; y++)
        {
            int src = y * img.Width * 3;
            int row = y * stride;
            for (int x = 0; x < img.Width; x++)
            {
                int s = src + x * 3;
                int d = row + x * 4;
                bytes[d] = Quantise(data[s + 2]);     // B
                bytes[d + 1] = Quantise(data[s + 1]); // G
                bytes[d + 2] = Quantise(data[s]);     // R
                bytes[d + 3] = 255;                   // A
            }
        }
        return bytes;
    }

    private static byte Quantise(float v)
    {
        if (float.IsNaN(v)) return 0;
        float scaled = v * 255f + 0.5f;
        if (scaled <= 0f) return 0;
        if (scaled >= 255f) return 255;
        return (byte)scaled;
    }
}
