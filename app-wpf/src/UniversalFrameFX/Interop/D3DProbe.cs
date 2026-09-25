using System.Runtime.InteropServices;

namespace UniversalFrameFX.Interop;

/// <summary>
/// Asks Direct3D what it actually supports, instead of guessing from whether a
/// DLL exists on disk.
/// </summary>
/// <remarks>
/// <para>
/// This replaces a <c>File.Exists</c> check that was wrong in two directions:
/// <c>d3d11.dll</c> and <c>d3d12.dll</c> are present on every Windows 10 and 11
/// install regardless of what the GPU can do, so the check was effectively a
/// constant; and DX12 was additionally gated on <c>build &gt;= 22000</c>, which
/// reported DX12 as unavailable on every Windows 10 machine and in turn falsely
/// marked FSR 3 unavailable there.
/// </para>
/// <para>
/// Creating a null-adapter device is the documented way to ask this question. No
/// swapchain, no window, and the device is released immediately.
/// </para>
/// </remarks>
internal static class D3DProbe
{
    private const uint D3D_DRIVER_TYPE_HARDWARE = 1;

    // Feature levels, highest first; the first one accepted is what we report.
    private static readonly uint[] Levels11 =
    {
        0xc100, // 12_1
        0xc000, // 12_0
        0xb100, // 11_1
        0xb000, // 11_0
    };

    [DllImport("d3d11.dll", ExactSpelling = true)]
    private static extern int D3D11CreateDevice(
        IntPtr adapter, uint driverType, IntPtr software, uint flags,
        uint[]? featureLevels, uint featureLevelCount, uint sdkVersion,
        out IntPtr device, out uint featureLevel, out IntPtr context);

    [DllImport("d3d12.dll", ExactSpelling = true)]
    private static extern int D3D12CreateDevice(
        IntPtr adapter, uint minimumFeatureLevel, in Guid riid, out IntPtr device);

    /// <summary>Result of probing the graphics runtimes.</summary>
    internal readonly record struct Result(
        bool Dx11, uint Dx11FeatureLevel, bool Dx12, bool Probed);

    private static string LevelName(uint fl) => fl switch
    {
        0xc100 => "12_1",
        0xc000 => "12_0",
        0xb100 => "11_1",
        0xb000 => "11_0",
        0xa100 => "10_1",
        0xa000 => "10_0",
        _ => $"0x{fl:x}",
    };

    /// <summary>Human-readable feature level, or empty when not probed.</summary>
    internal static string FeatureLevelText(uint fl) => fl == 0 ? "" : LevelName(fl);

    /// <summary>
    /// Probes D3D11 and D3D12. Never throws: a missing DLL or a failed device
    /// creation is reported as "not supported", and <c>Probed</c> is false only
    /// if the probe itself could not run.
    /// </summary>
    internal static Result Probe()
    {
        bool dx11 = false;
        uint level = 0;
        try
        {
            const uint D3D11_SDK_VERSION = 7;
            int hr = D3D11CreateDevice(IntPtr.Zero, D3D_DRIVER_TYPE_HARDWARE, IntPtr.Zero, 0,
                                       Levels11, (uint)Levels11.Length, D3D11_SDK_VERSION,
                                       out var dev, out level, out var ctx);
            if (hr >= 0)
            {
                dx11 = true;
                if (ctx != IntPtr.Zero) Marshal.Release(ctx);
                if (dev != IntPtr.Zero) Marshal.Release(dev);
            }
        }
        catch (DllNotFoundException) { /* no D3D11 runtime at all */ }
        catch (EntryPointNotFoundException) { }

        bool dx12 = false;
        try
        {
            // IID_ID3D12Device
            var iid = new Guid("189819f1-1db6-4b57-be54-1821339b85f7");
            const uint FL_11_0 = 0xb000;
            int hr = D3D12CreateDevice(IntPtr.Zero, FL_11_0, in iid, out var dev12);
            if (hr >= 0)
            {
                dx12 = true;
                if (dev12 != IntPtr.Zero) Marshal.Release(dev12);
            }
        }
        catch (DllNotFoundException) { /* pre-DX12 Windows, or no runtime */ }
        catch (EntryPointNotFoundException) { }

        return new Result(dx11, level, dx12, true);
    }
}
