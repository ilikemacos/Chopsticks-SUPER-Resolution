// CSR resolve pass — edge-directed spatial upscale.
//
// Port of csr/ref/csr.py (the specification) and csr/src/Csr.Core
// (the verified CPU port). Verify against csr/ref/golden/*.json to within
// 1/255 before trusting it; exact reciprocals on the CPU versus rcp()/rsqrt()
// here mean bit-exactness is not the target.
//
// Compile: dxc -T cs_5_0 -E main CsrResolve.hlsl
// cs_5_0 deliberately, not cs_6_0: Kepler-era hardware (GeForce 700 series)
// is D3D11 feature level 11_0 and has no DXIL path.

Texture2D<float4>   Src : register(t0);
RWTexture2D<float4> Dst : register(u0);

cbuffer CsrConstants : register(b0)
{
    uint2  SrcSize;
    uint2  DstSize;
    float2 Scale;        // SrcSize / DstSize
    float  SharpnessLin; // exp2(-stops); unused in this pass
    float  Pad0;
};

static const float3 REC709 = float3(0.2126f, 0.7152f, 0.0722f);

float SafeDiv(float n, float d) { return d != 0.0f ? n / d : 0.0f; }

float3 Tap(int2 p)
{
    return Src[clamp(p, int2(0, 0), int2(SrcSize) - 1)].rgb;
}

float LumaAt(int2 p) { return dot(Tap(p), REC709); }

// One quadrant of the '+' pattern around a centre tap.
void Quadrant(float lA, float lB, float lC, float lD, float lE, float w,
              inout float2 dir, inout float len)
{
    float dc = lD - lC, cb = lC - lB;
    float mx = max(abs(dc), abs(cb));
    float gx = lD - lB;
    dir.x += gx * w;
    float lx = saturate(SafeDiv(abs(gx), mx));
    len += lx * lx * w;

    float ec = lE - lC, ca = lC - lA;
    float my = max(abs(ec), abs(ca));
    float gy = lE - lA;
    dir.y += gy * w;
    float ly = saturate(SafeDiv(abs(gy), my));
    len += ly * ly * w;
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= DstSize.x || tid.y >= DstSize.y) return;

    float2 pp = (float2(tid.xy) + 0.5f) * Scale - 0.5f;
    int2   ip = int2(floor(pp));
    float2 f  = pp - float2(ip);

    // 12 luma taps for edge estimation.
    float lb = LumaAt(ip + int2(0, -1)), lc = LumaAt(ip + int2(1, -1));
    float le = LumaAt(ip + int2(-1, 0)), lf = LumaAt(ip + int2(0, 0));
    float lg = LumaAt(ip + int2(1, 0)),  lh = LumaAt(ip + int2(2, 0));
    float li = LumaAt(ip + int2(-1, 1)), lj = LumaAt(ip + int2(0, 1));
    float lk = LumaAt(ip + int2(1, 1)),  ll = LumaAt(ip + int2(2, 1));
    float ln = LumaAt(ip + int2(0, 2)),  lo = LumaAt(ip + int2(1, 2));

    float wF = (1.0f - f.x) * (1.0f - f.y);
    float wG = f.x * (1.0f - f.y);
    float wJ = (1.0f - f.x) * f.y;
    float wK = f.x * f.y;

    float2 dir = float2(0.0f, 0.0f);
    float  len = 0.0f;
    Quadrant(lb, le, lf, lg, lj, wF, dir, len);
    Quadrant(lc, lf, lg, lh, lk, wG, dir, len);
    Quadrant(lf, li, lj, lk, ln, wJ, dir, len);
    Quadrant(lg, lj, lk, ll, lo, wK, dir, len);

    float dir2 = dot(dir, dir);
    bool  zero = dir2 < (1.0f / 32768.0f);
    float inv  = zero ? 1.0f : rsqrt(dir2);
    dir = float2(zero ? 1.0f : dir.x, zero ? 0.0f : dir.y) * inv;

    len *= 0.5f;
    len *= len;

    // Narrow across the edge, widen along it.
    float m       = max(abs(dir.x), abs(dir.y));
    float stretch = SafeDiv(dot(dir, dir), m);
    float lenAcross = 1.0f + (stretch - 1.0f) * len;
    float lenAlong  = 1.0f - 0.5f * len;

    float lob = 0.5f + ((1.0f / 4.0f - 0.04f) - 0.5f) * len;
    float clp = SafeDiv(1.0f, lob);

    float3 acc = float3(0.0f, 0.0f, 0.0f);
    float  accW = 0.0f;

    // Full 4x4 support. FSR 1 uses 12 taps and drops the corners; including
    // them is worth +0.53 dB because they carry the diagonal-edge information.
    [unroll]
    for (int dy = -1; dy <= 2; dy++)
    {
        [unroll]
        for (int dx = -1; dx <= 2; dx++)
        {
            float2 off = float2(dx, dy) - f;
            float2 v = float2(( off.x * dir.x + off.y * dir.y) * lenAcross,
                              (-off.x * dir.y + off.y * dir.x) * lenAlong);
            float d2 = min(dot(v, v), clp);

            // FSR 1's polynomial lobe. Its radius is coupled to edge strength,
            // which a fixed-support cubic cannot reproduce — the Keys cubic
            // alternative measured 2.13 dB worse.
            float wB = (2.0f / 5.0f) * d2 - 1.0f;
            float wA = lob * d2 - 1.0f;
            wB *= wB;
            wA *= wA;
            wB = (25.0f / 16.0f) * wB - (25.0f / 16.0f - 1.0f);
            float w = wB * wA;

            acc  += Tap(ip + int2(dx, dy)) * w;
            accW += w;
        }
    }

    float3 col = acc / max(accW, 1e-8f);

    // Deringing: clamp to the four nearest taps. Mandatory — without it hard
    // edges overshoot.
    float3 t00 = Tap(ip), t10 = Tap(ip + int2(1, 0));
    float3 t01 = Tap(ip + int2(0, 1)), t11 = Tap(ip + int2(1, 1));
    float3 lo4 = min(min(t00, t10), min(t01, t11));
    float3 hi4 = max(max(t00, t10), max(t01, t11));
    col = clamp(col, lo4, hi4);

    Dst[tid.xy] = float4(saturate(col), 1.0f);
}
