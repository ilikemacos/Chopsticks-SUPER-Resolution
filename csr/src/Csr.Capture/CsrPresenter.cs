using Vortice.DXGI;
using Vortice.Direct3D11;

namespace Csr.Capture;

/// <summary>
/// Presents CSR's upscaled texture to a window via a DXGI flip-model swapchain.
/// </summary>
/// <remarks>
/// <para>
/// This is the last mile of the live path: capture -> CSR (<see cref="CsrGpuPipeline"/>)
/// -> present. The upscaled frame is already in VRAM at the target resolution, so
/// presenting it is a single GPU-to-GPU <c>CopyResource</c> into the backbuffer
/// followed by <c>Present</c>. There is no CPU copy on this path either.
/// </para>
/// <para>
/// <b>Not thread-safe by design.</b> It shares the device's immediate context, so
/// every call must come from one thread. In the live pipeline that is the WGC
/// frame-arrived thread, the same thread that ran the compute dispatch, so
/// capture, upscale and present are naturally serialised without a lock.
/// </para>
/// <para>
/// The backbuffer is always kept exactly the size of the incoming upscaled
/// texture, so the <c>CopyResource</c> is always dimension-valid and the host
/// window shows CSR's pixels 1:1 rather than letting the swapchain stretch them
/// (which would layer a bilinear scale on top of CSR and defeat the point).
/// </para>
/// </remarks>
public sealed class CsrPresenter : IDisposable
{
    private const Format SwapFormat = Format.B8G8R8A8_UNorm;

    private readonly ID3D11Device _device;
    private readonly ID3D11DeviceContext _context;
    private readonly IDXGISwapChain1 _swapChain;

    private ID3D11Texture2D? _backBuffer;
    private int _width, _height;
    private bool _disposed;

    /// <summary>Frames presented so far.</summary>
    public long FramesPresented { get; private set; }

    public CsrPresenter(ID3D11Device device, IntPtr hwnd, int width, int height)
    {
        ArgumentNullException.ThrowIfNull(device);
        if (hwnd == IntPtr.Zero) throw new ArgumentException("null HWND", nameof(hwnd));
        _device = device;
        _context = device.ImmediateContext;
        _width = Math.Max(1, width);
        _height = Math.Max(1, height);

        using var dxgiDevice = device.QueryInterface<IDXGIDevice>();
        using var adapter = dxgiDevice.GetAdapter();
        using var factory = adapter.GetParent<IDXGIFactory2>();

        var desc = new SwapChainDescription1
        {
            Width = (uint)_width,
            Height = (uint)_height,
            Format = SwapFormat,
            Stereo = false,
            SampleDescription = new SampleDescription(1, 0),
            BufferUsage = Usage.RenderTargetOutput,
            BufferCount = 2,
            Scaling = Scaling.Stretch,
            SwapEffect = SwapEffect.FlipDiscard,
            AlphaMode = AlphaMode.Ignore,
            Flags = SwapChainFlags.None,
        };

        _swapChain = factory.CreateSwapChainForHwnd(device, hwnd, desc);
        // We handle Alt+Enter ourselves (we don't want exclusive fullscreen).
        factory.MakeWindowAssociation(hwnd, WindowAssociationFlags.IgnoreAltEnter);
        AcquireBackBuffer();
    }

    private void AcquireBackBuffer()
    {
        _backBuffer?.Dispose();
        _backBuffer = _swapChain.GetBuffer<ID3D11Texture2D>(0);
    }

    private void Resize(int width, int height)
    {
        _width = Math.Max(1, width);
        _height = Math.Max(1, height);
        _backBuffer?.Dispose();
        _backBuffer = null;
        // All outstanding references to the backbuffer must be released first.
        _swapChain.ResizeBuffers(2, (uint)_width, (uint)_height, SwapFormat, SwapChainFlags.None);
        AcquireBackBuffer();
    }

    /// <summary>
    /// Copies the upscaled texture into the backbuffer and presents it. If the
    /// texture's size changed (the source window resized), the swapchain is
    /// resized to match first, so the copy is always valid and the display stays
    /// 1:1 with CSR's output.
    /// </summary>
    /// <returns>The size presented, so the host can match its client area.</returns>
    public (int Width, int Height) Present(ID3D11Texture2D upscaled, bool vsync = true)
    {
        ArgumentNullException.ThrowIfNull(upscaled);
        if (_disposed) return (_width, _height);

        var d = upscaled.Description;
        int uw = (int)d.Width, uh = (int)d.Height;
        if (uw != _width || uh != _height) Resize(uw, uh);

        _context.CopyResource(_backBuffer!, upscaled);
        _swapChain.Present(vsync ? 1u : 0u, PresentFlags.None);
        FramesPresented++;
        return (_width, _height);
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _backBuffer?.Dispose();
        _swapChain.Dispose();
    }
}
