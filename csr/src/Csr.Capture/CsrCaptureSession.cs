using System.Runtime.InteropServices;
using Vortice.Direct3D11;
using Windows.Graphics;
using Windows.Graphics.Capture;
using Windows.Graphics.DirectX;
using Windows.Graphics.DirectX.Direct3D11;
using WinRT;

namespace Csr.Capture;

/// <summary>
/// Windows Graphics Capture source feeding the CSR compute pipeline.
/// </summary>
/// <remarks>
/// <para>
/// <b>Why WGC and not DXGI Desktop Duplication.</b> Desktop Duplication captures
/// a whole display output, so a window target needs cropping, it cannot capture
/// an occluded window, and it has to handle mode changes and rotation. It exists
/// for remote desktop. WGC targets a window or monitor directly, survives resize,
/// and hands back a GPU texture. For per-application upscaling WGC is the correct
/// choice.
/// </para>
/// <para>
/// <b>Latency, honestly.</b> WGC delivers frames <i>after</i> DWM composition,
/// and the upscaled result is then presented through DWM again. That is a
/// structural one-to-two frame addition (roughly 16–33 ms at 60 Hz) and no
/// tuning removes it. "Zero compositor lag" is not achievable on this path — the
/// only way to avoid the round trip is to intercept inside the target process
/// before <c>Present</c>, which is a different mechanism with different
/// trade-offs.
/// </para>
/// <para>
/// What this design <i>does</i> minimise is everything else: the frame stays in
/// VRAM from capture to present, so there is no readback, no staging texture and
/// no CPU copy of pixel data.
/// </para>
/// </remarks>
public sealed class CsrCaptureSession : IDisposable
{
    private readonly ID3D11Device _device;
    private readonly CsrGpuPipeline _pipeline;
    private readonly GraphicsCaptureItem _item;
    private readonly IDirect3DDevice _winrtDevice;
    private readonly Direct3D11CaptureFramePool _framePool;
    private readonly GraphicsCaptureSession _session;
    private readonly float _ratio;
    private readonly object _gate = new();

    private ID3D11Texture2D? _output;
    private SizeInt32 _lastSize;
    private bool _disposed;

    /// <summary>Raised with the upscaled texture. The handler must not retain it.</summary>
    public event Action<ID3D11Texture2D>? FrameUpscaled;

    /// <summary>Frames the GPU pipeline has completed.</summary>
    public long FramesProcessed { get; private set; }

    /// <summary>Frames dropped because the previous one was still in flight.</summary>
    /// <remarks>
    /// Non-zero means CSR cannot keep up with the capture rate on this hardware,
    /// which is the signal to lower the output resolution or disable CSR. It is
    /// reported rather than hidden.
    /// </remarks>
    public long FramesDropped { get; private set; }

    /// <summary>Last measured GPU cost of both CSR passes, in milliseconds.</summary>
    public double LastGpuMilliseconds => _pipeline.LastGpuMilliseconds;

    private CsrCaptureSession(ID3D11Device device, GraphicsCaptureItem item, float ratio)
    {
        _device = device;
        _item = item;
        _ratio = ratio;
        _pipeline = new CsrGpuPipeline(device);

        using var dxgi = device.QueryInterface<Vortice.DXGI.IDXGIDevice>();
        var abi = CaptureInterop.CreateDirect3D11DeviceFromDXGIDevice(dxgi.NativePointer);
        _winrtDevice = MarshalInterface<IDirect3DDevice>.FromAbi(abi);
        Marshal.Release(abi);

        _lastSize = item.Size;

        // Free-threaded: FrameArrived is delivered on a pool thread rather than
        // requiring a DispatcherQueue, which keeps the capture path off the UI
        // thread entirely.
        _framePool = Direct3D11CaptureFramePool.CreateFreeThreaded(
            _winrtDevice,
            DirectXPixelFormat.B8G8R8A8UIntNormalized,
            2,                 // two buffers: enough to pipeline, minimal latency
            _lastSize);

        _session = _framePool.CreateCaptureSession(item);
        TryConfigureSession(_session);

        _framePool.FrameArrived += OnFrameArrived;
        _item.Closed += (_, _) => Dispose();
    }

    /// <summary>
    /// Applies the optional session settings that only exist on newer builds.
    /// </summary>
    private static void TryConfigureSession(GraphicsCaptureSession session)
    {
        // The cursor is composited into the captured frame, so upscaling it
        // would produce a soft, doubled pointer. Added in build 19041.
        if (OperatingSystem.IsWindowsVersionAtLeast(10, 0, 19041))
        {
            try { session.IsCursorCaptureEnabled = false; }
            catch (Exception ex) when (ex is NotSupportedException
                                      or UnauthorizedAccessException) { }
        }

        // Windows 11 draws a yellow capture border by default; it would be
        // upscaled along with the content. The property does not exist before
        // build 22000, hence the version guard rather than a bare try/catch.
        if (OperatingSystem.IsWindowsVersionAtLeast(10, 0, 22000))
        {
            try { session.IsBorderRequired = false; }
            catch (Exception ex) when (ex is NotSupportedException
                                      or UnauthorizedAccessException) { }
        }
    }

    /// <summary>Starts capturing a window by handle, with no picker UI.</summary>
    public static CsrCaptureSession StartForWindow(ID3D11Device device, IntPtr hwnd,
                                                  float ratio = 1.3333333f)
    {
        ArgumentNullException.ThrowIfNull(device);
        if (!CaptureInterop.IsSupported())
            throw new PlatformNotSupportedException(
                "Windows Graphics Capture is unavailable. CSR requires Windows 10 " +
                "1809 (build 17763) or later for the free-threaded frame pool.");

        var item = CaptureInterop.CreateForWindow(hwnd);
        var s = new CsrCaptureSession(device, item, ratio);
        s._session.StartCapture();
        return s;
    }

    /// <summary>
    /// Starts capturing whatever the user selects in the system picker.
    /// </summary>
    /// <remarks>
    /// The picker is the consent mechanism, so this is the right entry point the
    /// first time a target is chosen. Saved profiles should use
    /// <see cref="StartForWindow"/> instead, which needs no per-launch click.
    /// </remarks>
    public static async Task<CsrCaptureSession> StartWithPickerAsync(
        ID3D11Device device, float ratio = 1.3333333f)
    {
        ArgumentNullException.ThrowIfNull(device);
        if (!CaptureInterop.IsSupported())
            throw new PlatformNotSupportedException("Windows Graphics Capture is unavailable.");

        var picker = new GraphicsCapturePicker();
        var item = await picker.PickSingleItemAsync()
            ?? throw new OperationCanceledException("no capture target was selected");

        var s = new CsrCaptureSession(device, item, ratio);
        s._session.StartCapture();
        return s;
    }

    private void OnFrameArrived(Direct3D11CaptureFramePool pool, object? _)
    {
        // Never block the capture callback. If CSR is still busy with the
        // previous frame, drop this one and count it — queueing would grow
        // latency without improving throughput.
        if (!Monitor.TryEnter(_gate)) { FramesDropped++; return; }
        try
        {
            if (_disposed) return;

            using var frame = pool.TryGetNextFrame();
            if (frame is null) return;

            if (frame.ContentSize.Width != _lastSize.Width ||
                frame.ContentSize.Height != _lastSize.Height)
            {
                // The target window resized. Recreate at the new size; the next
                // frame will arrive with matching dimensions.
                _lastSize = frame.ContentSize;
                pool.Recreate(_winrtDevice, DirectXPixelFormat.B8G8R8A8UIntNormalized,
                              2, _lastSize);
                DisposeOutput();
                return;
            }

            using var srcTexture = GetTexture(frame.Surface);
            if (srcTexture is null) return;

            var desc = srcTexture.Description;
            int outW = Math.Max(1, (int)MathF.Round((int)desc.Width * _ratio));
            int outH = Math.Max(1, (int)MathF.Round((int)desc.Height * _ratio));

            EnsureOutput(outW, outH);
            _pipeline.Dispatch(srcTexture, _output!);
            FramesProcessed++;
            FrameUpscaled?.Invoke(_output!);
        }
        catch (SharpGen.Runtime.SharpGenException)
        {
            // Device lost, or the target vanished mid-frame. The owner is
            // expected to observe FramesProcessed stalling and restart.
        }
        finally
        {
            Monitor.Exit(_gate);
        }
    }

    /// <summary>
    /// Unwraps the WinRT surface to the D3D11 texture backing it. No copy: this
    /// is the same VRAM allocation DWM already holds.
    /// </summary>
    private static ID3D11Texture2D? GetTexture(IDirect3DSurface surface)
    {
        var access = surface.As<CaptureInterop.IDirect3DDxgiInterfaceAccess>();
        var iid = typeof(ID3D11Texture2D).GUID;
        var ptr = access.GetInterface(ref iid);
        return ptr == IntPtr.Zero ? null : new ID3D11Texture2D(ptr);
    }

    private void EnsureOutput(int w, int h)
    {
        if (_output is not null &&
            _output.Description.Width == (uint)w &&
            _output.Description.Height == (uint)h) return;

        DisposeOutput();
        _output = _device.CreateTexture2D(new Texture2DDescription
        {
            Width = (uint)w,
            Height = (uint)h,
            MipLevels = 1,
            ArraySize = 1,
            Format = Vortice.DXGI.Format.B8G8R8A8_UNorm,
            SampleDescription = new Vortice.DXGI.SampleDescription(1, 0),
            Usage = ResourceUsage.Default,
            BindFlags = BindFlags.ShaderResource | BindFlags.UnorderedAccess,
        });
    }

    private void DisposeOutput()
    {
        _output?.Dispose();
        _output = null;
    }

    public void Dispose()
    {
        lock (_gate)
        {
            if (_disposed) return;
            _disposed = true;
        }

        _framePool.FrameArrived -= OnFrameArrived;
        _session.Dispose();
        _framePool.Dispose();
        _pipeline.Dispose();
        DisposeOutput();
    }
}
