using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Windows.Forms;
using Csr.Capture;
using Vortice.Direct3D;
using Vortice.Direct3D11;

namespace Csr.LiveUpscale;

// Real-time CSR upscaling of a live window: Windows Graphics Capture -> CSR
// compute pipeline -> DXGI present, all in VRAM.
//
// This is the "works on any app" path. It runs only on Windows with a GPU; it is
// compiled on CI (EnableWindowsTargeting) but cannot be executed there.
//
// Usage:
//   ufx-live-upscale                     pick a window/monitor with the system UI
//   ufx-live-upscale --window "Notepad"  target the first window whose title matches
//   ufx-live-upscale --quality Quality   preset ratio (default Quality = 1.5x)
[SupportedOSPlatform("windows10.0.17763.0")]
internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        string? windowMatch = null;
        float ratio = 1.5f;
        for (int i = 0; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "--window" when i + 1 < args.Length: windowMatch = args[++i]; break;
                case "--quality" when i + 1 < args.Length: ratio = RatioOf(args[++i]); break;
                case "-h" or "--help": PrintUsage(); return 0;
                default: Console.Error.WriteLine($"unknown argument: {args[i]}"); PrintUsage(); return 2;
            }
        }

        if (!CaptureSupported())
        {
            MessageBox.Show(
                "Windows Graphics Capture is not available. This needs Windows 10 " +
                "1809 (build 17763) or later.", "CSR live upscale",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }

        ApplicationConfiguration.Initialize();

        ID3D11Device device = CreateDevice();
        try
        {
            using var form = new PresentForm(device, windowMatch, ratio);
            Application.Run(form);
        }
        finally
        {
            device.Dispose();
        }
        return 0;
    }

    private static bool CaptureSupported()
    {
        try { return Windows.Graphics.Capture.GraphicsCaptureSession.IsSupported(); }
        catch (TypeLoadException) { return false; }
    }

    private static ID3D11Device CreateDevice()
    {
        // BgraSupport is required to interop with WGC's B8G8R8A8 surfaces.
        var result = D3D11.D3D11CreateDevice(
            null, DriverType.Hardware, DeviceCreationFlags.BgraSupport,
            new[] { FeatureLevel.Level_11_1, FeatureLevel.Level_11_0 },
            out ID3D11Device? device);
        if (result.Failure || device is null)
            throw new InvalidOperationException("could not create a Direct3D 11 device");
        return device;
    }

    private static float RatioOf(string mode) => mode.ToLowerInvariant() switch
    {
        "ultra quality" or "ultra-quality" => 1.3f,
        "quality" => 1.5f,
        "balanced" => 1.7f,
        "performance" => 2.0f,
        "ultra performance" or "ultra-performance" => 3.0f,
        _ => 1.5f,
    };

    private static void PrintUsage() => Console.WriteLine(
        "ufx-live-upscale [--window <title-substring>] [--quality <mode>]\n" +
        "  No --window: the system picker chooses the target.\n" +
        "  Modes: Ultra Quality | Quality | Balanced | Performance | Ultra Performance");

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr FindWindow(string? cls, string? title);

    // Finds the first top-level window whose title contains the given text.
    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc proc, IntPtr lParam);
    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowText(IntPtr hwnd, char[] text, int count);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowTextLength(IntPtr hwnd);
    private delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);

    internal static IntPtr FindWindowByTitle(string match)
    {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hwnd, _) =>
        {
            if (!IsWindowVisible(hwnd)) return true;
            int len = GetWindowTextLength(hwnd);
            if (len <= 0) return true;
            var buf = new char[len + 1];
            int n = GetWindowText(hwnd, buf, buf.Length);
            var title = new string(buf, 0, n);
            if (title.Contains(match, StringComparison.OrdinalIgnoreCase))
            {
                found = hwnd;
                return false; // stop
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
