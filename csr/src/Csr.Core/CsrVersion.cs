namespace Csr.Core;

/// <summary>Identifies the CSR release. CSR 1.0 is proprietary; see csr/LICENSE.</summary>
public static class CsrVersion
{
    /// <summary>Marketing/display version, e.g. "1.0".</summary>
    public const string Display = "1.0";

    /// <summary>Full semantic version.</summary>
    public const string Full = "1.0.0";

    /// <summary>Product name as it must be shown in UI (never "FSR"/"FidelityFX").</summary>
    public const string Name = "CSR (Chopsticks Super Resolution)";
}
