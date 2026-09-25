using System.Reflection;
using Vortice.Direct3D;
using Vortice.Direct3D11;
using Vortice.DXGI;

namespace Csr.Capture;

/// <summary>
/// The GPU side of CSR: two compute dispatches over a texture that is already
/// in VRAM.
/// </summary>
/// <remarks>
/// <para>
/// The frame never touches the CPU. A WGC frame arrives as an
/// <c>ID3D11Texture2D</c>, is bound as an SRV, and the result is written to a
/// UAV. There is no staging texture, no <c>Map</c> and no <c>memcpy</c> — that
/// is what makes this cheap, and it is the whole reason to prefer WGC's frame
/// pool over a readback-based capture.
/// </para>
/// <para>
/// Shaders are compiled for cs_5_0, not cs_6_0, so feature-level 11_0 hardware
/// (Kepler-era GeForce 700 series and similar) can run them.
/// </para>
/// </remarks>
public sealed class CsrGpuPipeline : IDisposable
{
    private readonly ID3D11Device _device;
    private readonly ID3D11DeviceContext _context;
    private readonly ID3D11ComputeShader _resolve;
    private readonly ID3D11ComputeShader _sharpen;
    private readonly ID3D11Buffer _constants;
    private readonly ID3D11Query? _disjoint;
    private readonly ID3D11Query? _tsBegin;
    private readonly ID3D11Query? _tsEnd;

    private ID3D11Texture2D? _intermediate;
    private ID3D11UnorderedAccessView? _intermediateUav;
    private ID3D11ShaderResourceView? _intermediateSrv;
    private int _interW, _interH;

    /// <summary>Last measured GPU time for both passes, in milliseconds.</summary>
    /// <remarks>
    /// Measured, never estimated. On a GPU-bound title this cost is subtracted
    /// from the game's own frame budget, so it is the number that decides
    /// whether CSR is worth enabling at a given resolution.
    /// </remarks>
    public double LastGpuMilliseconds { get; private set; }

    public CsrGpuPipeline(ID3D11Device device, bool enableTiming = true)
    {
        ArgumentNullException.ThrowIfNull(device);
        _device = device;
        _context = device.ImmediateContext;

        _resolve = CompileEmbedded("CsrResolve.hlsl");
        _sharpen = CompileEmbedded("CsrSharpen.hlsl");

        _constants = _device.CreateBuffer(new BufferDescription
        {
            ByteWidth = 48, // 2x uint2 + float2 + 2 floats, padded to 16
            Usage = ResourceUsage.Dynamic,
            BindFlags = BindFlags.ConstantBuffer,
            CPUAccessFlags = CpuAccessFlags.Write,
        });

        if (enableTiming)
        {
            _disjoint = _device.CreateQuery(new QueryDescription(QueryType.TimestampDisjoint));
            _tsBegin = _device.CreateQuery(new QueryDescription(QueryType.Timestamp));
            _tsEnd = _device.CreateQuery(new QueryDescription(QueryType.Timestamp));
        }
    }

    private ID3D11ComputeShader CompileEmbedded(string name)
    {
        var asm = Assembly.GetExecutingAssembly();
        var resource = asm.GetManifestResourceNames()
                          .FirstOrDefault(n => n.EndsWith(name, StringComparison.Ordinal))
            ?? throw new InvalidOperationException($"embedded shader {name} not found");

        using var stream = asm.GetManifestResourceStream(resource)!;
        using var reader = new StreamReader(stream);
        var source = reader.ReadToEnd();

        // cs_5_0: D3D11 feature level 11_0 has no DXIL path, so shader model 6
        // would exclude exactly the old hardware this is meant to run on.
        var result = Vortice.D3DCompiler.Compiler.Compile(
            source, "main", name, "cs_5_0", out var blob, out var errors);
        if (result.Failure || blob is null)
            throw new InvalidOperationException(
                $"{name} failed to compile: {errors?.AsString() ?? "unknown error"}");

        using (blob)
            return _device.CreateComputeShader(blob.AsSpan());
    }

    private void EnsureIntermediate(int w, int h)
    {
        if (_intermediate is not null && _interW == w && _interH == h) return;

        _intermediateSrv?.Dispose();
        _intermediateUav?.Dispose();
        _intermediate?.Dispose();

        _intermediate = _device.CreateTexture2D(new Texture2DDescription
        {
            Width = (uint)w,
            Height = (uint)h,
            MipLevels = 1,
            ArraySize = 1,
            Format = Format.B8G8R8A8_UNorm,
            SampleDescription = new SampleDescription(1, 0),
            Usage = ResourceUsage.Default,
            BindFlags = BindFlags.ShaderResource | BindFlags.UnorderedAccess,
        });
        _intermediateUav = _device.CreateUnorderedAccessView(_intermediate);
        _intermediateSrv = _device.CreateShaderResourceView(_intermediate);
        _interW = w;
        _interH = h;
    }

    /// <summary>
    /// Upscales <paramref name="source"/> into <paramref name="destination"/>.
    /// Both must already live on this device; nothing is copied through the CPU.
    /// </summary>
    public void Dispatch(ID3D11Texture2D source, ID3D11Texture2D destination,
                        float sharpnessStops = 0.25f, bool adaptiveSharpen = true)
    {
        ArgumentNullException.ThrowIfNull(source);
        ArgumentNullException.ThrowIfNull(destination);

        var srcDesc = source.Description;
        var dstDesc = destination.Description;
        int sw = (int)srcDesc.Width, sh = (int)srcDesc.Height;
        int dw = (int)dstDesc.Width, dh = (int)dstDesc.Height;

        EnsureIntermediate(dw, dh);

        using var srcSrv = _device.CreateShaderResourceView(source);
        using var dstUav = _device.CreateUnorderedAccessView(destination);

        if (_disjoint is not null)
        {
            _context.Begin(_disjoint);
            _context.End(_tsBegin!);
        }

        WriteConstants(sw, sh, dw, dh, sharpnessStops, adaptiveSharpen);

        // Pass 1: resolve into the intermediate.
        _context.CSSetShader(_resolve);
        _context.CSSetConstantBuffer(0, _constants);
        _context.CSSetShaderResource(0, srcSrv);
        _context.CSSetUnorderedAccessView(0, _intermediateUav);
        _context.Dispatch(Groups(dw), Groups(dh), 1);

        // Unbind before rebinding the same resource with the other view type.
        _context.CSSetShaderResource(0, null);
        _context.CSSetUnorderedAccessView(0, null);

        // Pass 2: sharpen into the destination.
        _context.CSSetShader(_sharpen);
        _context.CSSetShaderResource(0, _intermediateSrv);
        _context.CSSetUnorderedAccessView(0, dstUav);
        _context.Dispatch(Groups(dw), Groups(dh), 1);

        _context.CSSetShaderResource(0, null);
        _context.CSSetUnorderedAccessView(0, null);
        _context.CSSetShader(null);

        if (_disjoint is not null)
        {
            _context.End(_tsEnd!);
            _context.End(_disjoint);
            ReadTiming();
        }
    }

    private static uint Groups(int n) => (uint)((n + 7) / 8); // numthreads(8,8,1)

    private void WriteConstants(int sw, int sh, int dw, int dh,
                                float sharpnessStops, bool adaptive)
    {
        var mapped = _context.Map(_constants, 0, MapMode.WriteDiscard);
        try
        {
            unsafe
            {
                var p = (byte*)mapped.DataPointer;
                var u = (uint*)p;
                u[0] = (uint)sw; u[1] = (uint)sh;
                u[2] = (uint)dw; u[3] = (uint)dh;
                var f = (float*)(p + 16);
                f[0] = (float)sw / dw;
                f[1] = (float)sh / dh;
                f[2] = MathF.Pow(2f, -sharpnessStops);
                f[3] = adaptive ? 1f : 0f;
            }
        }
        finally
        {
            _context.Unmap(_constants, 0);
        }
    }

    private void ReadTiming()
    {
        // Non-blocking: if the results are not ready this frame, keep the
        // previous value rather than stalling the pipeline to read a counter.
        if (!_context.GetData(_disjoint!, out QueryDataTimestampDisjoint dj)) return;
        if (dj.Disjoint) return;
        if (!_context.GetData(_tsBegin!, out ulong begin)) return;
        if (!_context.GetData(_tsEnd!, out ulong end)) return;
        if (dj.Frequency == 0) return;

        LastGpuMilliseconds = (end - begin) * 1000.0 / dj.Frequency;
    }

    public void Dispose()
    {
        _intermediateSrv?.Dispose();
        _intermediateUav?.Dispose();
        _intermediate?.Dispose();
        _tsEnd?.Dispose();
        _tsBegin?.Dispose();
        _disjoint?.Dispose();
        _constants.Dispose();
        _sharpen.Dispose();
        _resolve.Dispose();
    }
}
