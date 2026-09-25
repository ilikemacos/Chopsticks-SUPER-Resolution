namespace Csr.Core;

/// <summary>
/// Centre-aligned bilinear resample — the baseline CSR has to beat.
/// </summary>
/// <remarks>
/// This exists so a comparison against CSR is against a named, pinned filter
/// rather than whatever the host's image stack happens to do. WPF's
/// <c>TransformedBitmap</c>, for instance, resamples with Fant, not bilinear;
/// showing that alongside CSR and labelling it "bilinear" would be inaccurate.
/// The sampling convention matches <c>framefx_spatial.bilinear</c> in the Python
/// reference exactly: output pixel centres map to <c>(i + 0.5) * in/out - 0.5</c>
/// in input space, with edge clamping.
/// </remarks>
public static class Bilinear
{
    public static ImageBuffer Upscale(ImageBuffer src, int outWidth, int outHeight)
    {
        ArgumentNullException.ThrowIfNull(src);
        if (outWidth <= 0 || outHeight <= 0)
            throw new ArgumentOutOfRangeException(nameof(outWidth), "dimensions must be positive");

        var dst = new ImageBuffer(outWidth, outHeight);
        float sx = (float)src.Width / outWidth;
        float sy = (float)src.Height / outHeight;

        for (int y = 0; y < outHeight; y++)
        {
            float py = (y + 0.5f) * sy - 0.5f;
            int y0 = (int)MathF.Floor(py);
            float ty = py - y0;

            for (int x = 0; x < outWidth; x++)
            {
                float px = (x + 0.5f) * sx - 0.5f;
                int x0 = (int)MathF.Floor(px);
                float tx = px - x0;

                int i00 = src.ClampedIndex(x0, y0);
                int i10 = src.ClampedIndex(x0 + 1, y0);
                int i01 = src.ClampedIndex(x0, y0 + 1);
                int i11 = src.ClampedIndex(x0 + 1, y0 + 1);

                int o = (y * outWidth + x) * 3;
                for (int c = 0; c < 3; c++)
                {
                    float top = src.Data[i00 + c] * (1f - tx) + src.Data[i10 + c] * tx;
                    float bot = src.Data[i01 + c] * (1f - tx) + src.Data[i11 + c] * tx;
                    dst.Data[o + c] = top * (1f - ty) + bot * ty;
                }
            }
        }

        return dst;
    }
}
