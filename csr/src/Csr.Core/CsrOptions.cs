namespace Csr.Core;

/// <summary>
/// CSR tunables. Defaults are the configuration validated against FSR 1 in
/// <c>csr/ref</c> — do not change them without re-running <c>validate.py</c>.
/// </summary>
/// <remarks>
/// The options rejected by ablation (structure-tensor edge estimation, Keys
/// cubic kernel, linear anisotropy) are intentionally absent rather than
/// present-but-disabled. They measured worse than FSR 1; keeping switches for
/// them would only invite someone to turn them back on.
/// </remarks>
public sealed record CsrOptions
{
    /// <summary>Sharpening strength in stops. 0 is strongest; higher is gentler.</summary>
    public float SharpnessStops { get; init; } = 0.25f;

    /// <summary>Back sharpening off in low-variance regions. Worth +2.57 dB.</summary>
    public bool AdaptiveSharpen { get; init; } = true;

    /// <summary>
    /// Clamp the resolve to the range of the four nearest taps. This is what
    /// prevents overshoot on hard edges; never disable it in shipping code.
    /// </summary>
    public bool Dering { get; init; } = true;

    public static CsrOptions Default { get; } = new();
}
