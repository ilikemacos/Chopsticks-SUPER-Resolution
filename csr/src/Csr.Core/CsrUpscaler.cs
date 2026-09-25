using System.Runtime.CompilerServices;

namespace Csr.Core;

/// <summary>
/// CSR — Chopsticks Super Resolution. Spatial upscaler derived from FSR 1.
/// </summary>
/// <remarks>
/// <para>
/// Port of <c>csr/ref/csr.py</c>, which is the specification. Verified against
/// <c>csr/ref/golden/*.json</c> to within one 8-bit step.
/// </para>
/// <para>
/// This resolves a single finished frame. It is not FSR 2, DLSS or XeSS and
/// cannot be made equivalent to them — those reconstruct from motion vectors,
/// depth and jitter supplied by the renderer, none of which exist here.
/// </para>
/// </remarks>
public static class CsrUpscaler
{
    // Rec.709 luma. Measured +0.22 dB over FSR 1's cheap 0.5B+0.5R+G proxy on
    // content where the channels disagree.
    private const float LumaR = 0.2126f;
    private const float LumaG = 0.7152f;
    private const float LumaB = 0.0722f;

    // Window of the sharpening negative lobe, from FSR 1.
    private const float RcasLimit = 0.25f - (1.0f / 16.0f);

    /// <summary>
    /// The full pipeline: resolve then sharpen.
    /// </summary>
    /// <remarks>
    /// At 1:1 this returns a copy without touching the pixels. The resolve is a
    /// filter with a negative lobe, not a pass-through, so running it at 1.0x
    /// would visibly alter a frame the user asked not to be upscaled.
    /// </remarks>
    public static ImageBuffer Upscale(ImageBuffer src, int outWidth, int outHeight,
                                      CsrOptions? options = null)
    {
        ArgumentNullException.ThrowIfNull(src);
        options ??= CsrOptions.Default;

        if (outWidth == src.Width && outHeight == src.Height)
            return new ImageBuffer(src.Width, src.Height, src.Data);

        var resolved = Resolve(src, outWidth, outHeight, options);
        return Sharpen(resolved, options);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float SafeDiv(float n, float d) => d != 0f ? n / d : 0f;

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float Luma(ReadOnlySpan<float> p, int i)
        => LumaR * p[i] + LumaG * p[i + 1] + LumaB * p[i + 2];

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float Sat(float v) => v < 0f ? 0f : (v > 1f ? 1f : v);

    /// <summary>
    /// Accumulates edge direction and strength for one quadrant of the '+'
    /// pattern around a centre tap. FSR 1's estimator, kept because the
    /// structure-tensor alternative measured 0.38 dB worse.
    /// </summary>
    private static void Quadrant(float lA, float lB, float lC, float lD, float lE,
                                 float w, ref float dirX, ref float dirY, ref float len)
    {
        float dc = lD - lC, cb = lC - lB;
        float mx = MathF.Max(MathF.Abs(dc), MathF.Abs(cb));
        float gx = lD - lB;
        dirX += gx * w;
        float lx = Sat(SafeDiv(MathF.Abs(gx), mx));
        len += lx * lx * w;

        float ec = lE - lC, ca = lC - lA;
        float my = MathF.Max(MathF.Abs(ec), MathF.Abs(ca));
        float gy = lE - lA;
        dirY += gy * w;
        float ly = Sat(SafeDiv(MathF.Abs(gy), my));
        len += ly * ly * w;
    }

    /// <summary>CSR's edge-directed resolve pass.</summary>
    public static ImageBuffer Resolve(ImageBuffer src, int outWidth, int outHeight,
                                      CsrOptions? options = null)
    {
        ArgumentNullException.ThrowIfNull(src);
        options ??= CsrOptions.Default;
        var dst = new ImageBuffer(outWidth, outHeight);

        ReadOnlySpan<float> s = src.Data;
        Span<float> d = dst.Data;

        float scaleX = (float)src.Width / outWidth;
        float scaleY = (float)src.Height / outHeight;

        // Full 4x4 support. FSR 1 drops the four corners; including them is
        // worth +0.53 dB because they carry the diagonal-edge information.
        Span<int> offX = stackalloc int[16];
        Span<int> offY = stackalloc int[16];
        int n = 0;
        for (int dy = -1; dy <= 2; dy++)
            for (int dx = -1; dx <= 2; dx++) { offX[n] = dx; offY[n] = dy; n++; }

        for (int oy = 0; oy < outHeight; oy++)
        {
            float ppY = (oy + 0.5f) * scaleY - 0.5f;
            int ipY = (int)MathF.Floor(ppY);
            float fy = ppY - ipY;

            for (int ox = 0; ox < outWidth; ox++)
            {
                float ppX = (ox + 0.5f) * scaleX - 0.5f;
                int ipX = (int)MathF.Floor(ppX);
                float fx = ppX - ipX;

                // Luma of the 12 taps the estimator needs.
                float lb = Luma(s, src.ClampedIndex(ipX, ipY - 1));
                float lc = Luma(s, src.ClampedIndex(ipX + 1, ipY - 1));
                float le = Luma(s, src.ClampedIndex(ipX - 1, ipY));
                float lf = Luma(s, src.ClampedIndex(ipX, ipY));
                float lg = Luma(s, src.ClampedIndex(ipX + 1, ipY));
                float lh = Luma(s, src.ClampedIndex(ipX + 2, ipY));
                float li = Luma(s, src.ClampedIndex(ipX - 1, ipY + 1));
                float lj = Luma(s, src.ClampedIndex(ipX, ipY + 1));
                float lk = Luma(s, src.ClampedIndex(ipX + 1, ipY + 1));
                float ll = Luma(s, src.ClampedIndex(ipX + 2, ipY + 1));
                float ln_ = Luma(s, src.ClampedIndex(ipX, ipY + 2));
                float lo = Luma(s, src.ClampedIndex(ipX + 1, ipY + 2));

                float wF = (1f - fx) * (1f - fy);
                float wG = fx * (1f - fy);
                float wJ = (1f - fx) * fy;
                float wK = fx * fy;

                float dirX = 0f, dirY = 0f, len = 0f;
                Quadrant(lb, le, lf, lg, lj, wF, ref dirX, ref dirY, ref len);
                Quadrant(lc, lf, lg, lh, lk, wG, ref dirX, ref dirY, ref len);
                Quadrant(lf, li, lj, lk, ln_, wJ, ref dirX, ref dirY, ref len);
                Quadrant(lg, lj, lk, ll, lo, wK, ref dirX, ref dirY, ref len);

                // Normalise direction; a near-zero gradient means "no edge".
                float dir2 = dirX * dirX + dirY * dirY;
                bool zero = dir2 < (1.0f / 32768.0f);
                float inv = zero ? 1f : SafeDiv(1f, MathF.Sqrt(dir2));
                dirX = (zero ? 1f : dirX) * inv;
                dirY = (zero ? 0f : dirY) * inv;

                len *= 0.5f;
                len *= len;

                // Anisotropy: narrow across the edge, widen along it.
                float m = MathF.Max(MathF.Abs(dirX), MathF.Abs(dirY));
                float stretch = SafeDiv(dirX * dirX + dirY * dirY, m);
                float lenAcross = 1f + (stretch - 1f) * len;
                float lenAlong = 1f - 0.5f * len;

                float lob = 0.5f + ((1.0f / 4.0f - 0.04f) - 0.5f) * len;
                float clp = SafeDiv(1f, lob);

                float accR = 0f, accG = 0f, accB = 0f, accW = 0f;
                for (int t = 0; t < 16; t++)
                {
                    int idx = src.ClampedIndex(ipX + offX[t], ipY + offY[t]);
                    float offx = offX[t] - fx;
                    float offy = offY[t] - fy;

                    float vx = (offx * dirX + offy * dirY) * lenAcross;
                    float vy = (offx * -dirY + offy * dirX) * lenAlong;
                    float d2 = MathF.Min(vx * vx + vy * vy, clp);

                    // FSR 1's polynomial lobe. Its radius is coupled to edge
                    // strength, which a fixed-support cubic cannot reproduce —
                    // the Keys alternative measured 2.13 dB worse.
                    float wB = (2.0f / 5.0f) * d2 - 1f;
                    float wA = lob * d2 - 1f;
                    wB *= wB;
                    wA *= wA;
                    wB = (25.0f / 16.0f) * wB - (25.0f / 16.0f - 1.0f);
                    float w = wB * wA;

                    accR += s[idx] * w;
                    accG += s[idx + 1] * w;
                    accB += s[idx + 2] * w;
                    accW += w;
                }

                float rr = SafeDiv(accR, accW);
                float gg = SafeDiv(accG, accW);
                float bb = SafeDiv(accB, accW);

                if (options.Dering)
                {
                    int i0 = src.ClampedIndex(ipX, ipY);
                    int i1 = src.ClampedIndex(ipX + 1, ipY);
                    int i2 = src.ClampedIndex(ipX, ipY + 1);
                    int i3 = src.ClampedIndex(ipX + 1, ipY + 1);
                    for (int c = 0; c < 3; c++)
                    {
                        float a = s[i0 + c], b2 = s[i1 + c], c2 = s[i2 + c], e2 = s[i3 + c];
                        float lo2 = MathF.Min(MathF.Min(a, b2), MathF.Min(c2, e2));
                        float hi2 = MathF.Max(MathF.Max(a, b2), MathF.Max(c2, e2));
                        float v = c == 0 ? rr : (c == 1 ? gg : bb);
                        v = MathF.Min(hi2, MathF.Max(lo2, v));
                        if (c == 0) rr = v; else if (c == 1) gg = v; else bb = v;
                    }
                }

                int o = (oy * outWidth + ox) * 3;
                d[o] = Sat(rr);
                d[o + 1] = Sat(gg);
                d[o + 2] = Sat(bb);
            }
        }

        return dst;
    }

    /// <summary>
    /// Contrast-limited sharpening at output resolution. Cannot clip, by
    /// construction: the negative lobe is bounded by the local min/max so it
    /// never pushes a channel past 0 or 1.
    /// </summary>
    public static ImageBuffer Sharpen(ImageBuffer img, CsrOptions? options = null)
    {
        ArgumentNullException.ThrowIfNull(img);
        options ??= CsrOptions.Default;

        int w = img.Width, h = img.Height;
        var dst = new ImageBuffer(w, h);
        ReadOnlySpan<float> s = img.Data;
        Span<float> d = dst.Data;

        float sharp = MathF.Pow(2f, -options.SharpnessStops);

        // Local mean and mean-of-squares of luma, for the adaptive term. A 3x3
        // box, separable, with edge clamping to match the reference.
        float[]? mean = null;
        float[]? meanSq = null;
        if (options.AdaptiveSharpen)
        {
            var lum = new float[w * h];
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    lum[y * w + x] = Luma(s, img.ClampedIndex(x, y));
            mean = Box3(lum, w, h);
            var sq = new float[w * h];
            for (int i = 0; i < sq.Length; i++) sq[i] = lum[i] * lum[i];
            meanSq = Box3(sq, w, h);
        }

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                int ie = img.ClampedIndex(x, y);
                int ib = img.ClampedIndex(x, y - 1);
                int id = img.ClampedIndex(x - 1, y);
                int iRight = img.ClampedIndex(x + 1, y);
                int ih = img.ClampedIndex(x, y + 1);

                // Per-channel limiter: how much negative lobe fits before clipping.
                float lobe = float.NegativeInfinity;
                for (int c = 0; c < 3; c++)
                {
                    float b = s[ib + c], dd = s[id + c], ff = s[iRight + c], hh = s[ih + c];
                    float mn = MathF.Min(MathF.Min(b, dd), MathF.Min(ff, hh));
                    float mx = MathF.Max(MathF.Max(b, dd), MathF.Max(ff, hh));
                    float hitMin = SafeDiv(mn, 4f * mx);
                    float hitMax = SafeDiv(1f - mx, 4f * mn - 4f);
                    float l = MathF.Max(-hitMin, hitMax);
                    if (l > lobe) lobe = l;
                }
                lobe = MathF.Max(-RcasLimit, MathF.Min(lobe, 0f)) * sharp;

                if (options.AdaptiveSharpen)
                {
                    // Back off where there is little local detail; sharpening
                    // there only lifts noise. Worth +2.57 dB on mid-contrast
                    // content, and exactly 0 where the limiter already refuses.
                    int p = y * w + x;
                    float var = meanSq![p] - mean![p] * mean[p];
                    lobe *= Sat(var * 400f);
                }

                int o = (y * w + x) * 3;
                float denom = 4f * lobe + 1f;
                for (int c = 0; c < 3; c++)
                {
                    float acc = lobe * s[ib + c] + lobe * s[id + c]
                              + lobe * s[iRight + c] + lobe * s[ih + c] + s[ie + c];
                    d[o + c] = Sat(SafeDiv(acc, denom));
                }
            }
        }

        return dst;
    }

    /// <summary>Separable 3x3 box blur with edge clamping.</summary>
    private static float[] Box3(float[] src, int w, int h)
    {
        var tmp = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                int xm = x > 0 ? x - 1 : 0;
                int xp = x < w - 1 ? x + 1 : w - 1;
                tmp[y * w + x] = (src[y * w + xm] + src[y * w + x] + src[y * w + xp]) / 3f;
            }

        var outp = new float[w * h];
        for (int y = 0; y < h; y++)
        {
            int ym = y > 0 ? y - 1 : 0;
            int yp = y < h - 1 ? y + 1 : h - 1;
            for (int x = 0; x < w; x++)
                outp[y * w + x] = (tmp[ym * w + x] + tmp[y * w + x] + tmp[yp * w + x]) / 3f;
        }
        return outp;
    }
}
