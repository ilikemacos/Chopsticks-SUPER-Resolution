using System.Runtime.InteropServices;
using Windows.Graphics.Capture;
using WinRT;

namespace Csr.Capture;

/// <summary>
/// COM interop needed to reach Windows Graphics Capture from a plain desktop
/// process. These interfaces are not surfaced by the WinRT projections.
/// </summary>
internal static class CaptureInterop
{
    /// <summary>
    /// Creates a capture item for a specific window without showing a picker.
    /// </summary>
    /// <remarks>
    /// <see cref="GraphicsCapturePicker"/> is the right choice for a first-run
    /// flow, because the system UI is what grants consent. It is the wrong
    /// choice for a saved per-game profile: it requires a user click every
    /// time. This path is what a profile uses once the user has chosen a target.
    /// </remarks>
    [ComImport]
    [Guid("3628E81B-3CAC-4C60-B7F4-23CE0E0C3356")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IGraphicsCaptureItemInterop
    {
        IntPtr CreateForWindow([In] IntPtr window, [In] ref Guid iid);
        IntPtr CreateForMonitor([In] IntPtr monitor, [In] ref Guid iid);
    }

    /// <summary>Unwraps an <c>IDirect3DSurface</c> to the underlying DXGI resource.</summary>
    [ComImport]
    [Guid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IDirect3DDxgiInterfaceAccess
    {
        IntPtr GetInterface([In] ref Guid iid);
    }

    [DllImport("d3d11.dll", EntryPoint = "CreateDirect3D11DeviceFromDXGIDevice",
               SetLastError = true, CharSet = CharSet.Unicode,
               ExactSpelling = true, PreserveSig = false)]
    internal static extern IntPtr CreateDirect3D11DeviceFromDXGIDevice(IntPtr dxgiDevice);

    private static readonly Guid CaptureItemIid =
        new("79C3F95B-31F7-4EC2-A464-632EF5D30760");

    /// <summary>Targets a window by handle. Throws if WGC is unavailable.</summary>
    public static GraphicsCaptureItem CreateForWindow(IntPtr hwnd)
    {
        if (hwnd == IntPtr.Zero) throw new ArgumentException("null HWND", nameof(hwnd));

        var factory = GraphicsCaptureItem.As<IGraphicsCaptureItemInterop>();
        var iid = CaptureItemIid;
        var ptr = factory.CreateForWindow(hwnd, ref iid);
        if (ptr == IntPtr.Zero)
            throw new InvalidOperationException(
                "CreateForWindow returned null. The window may have been destroyed, " +
                "or it may be a protected/DRM surface that refuses capture.");

        return GraphicsCaptureItem.FromAbi(ptr);
    }

    /// <summary>
    /// True when this OS build supports Windows Graphics Capture at all.
    /// </summary>
    public static bool IsSupported()
    {
        // EntryPointNotFoundException derives from TypeLoadException, so one
        // catch covers both "projection missing" and "API absent on this build".
        try { return GraphicsCaptureSession.IsSupported(); }
        catch (TypeLoadException) { return false; }
    }
}
