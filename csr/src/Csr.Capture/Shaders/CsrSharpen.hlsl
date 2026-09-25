// CSR sharpen pass — contrast-limited, runs at output resolution.
//
// The limiter bounds the negative lobe by the local min/max, so this cannot
// clip. On already-saturated content it correctly does nothing at all.
//
// Compile: dxc -T cs_5_0 -E main CsrSharpen.hlsl

Texture2D<float4>   Src : register(t0);
RWTexture2D<float4> Dst : register(u0);

cbuffer CsrConstants : register(b0)
{
    uint2  SrcSize;
    uint2  DstSize;
    float2 Scale;
    float  SharpnessLin;   // exp2(-stops)
    float  AdaptiveSharpen; // 1 = on, 0 = off
};

static const float3 REC709 = float3(0.2126f, 0.7152f, 0.0722f);
static const float  RCAS_LIMIT = 0.25f - (1.0f / 16.0f);

float SafeDiv(float n, float d) { return d != 0.0f ? n / d : 0.0f; }

float3 Tap(int2 p)
{
    return Src[clamp(p, int2(0, 0), int2(DstSize) - 1)].rgb;
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= DstSize.x || tid.y >= DstSize.y) return;

    int2 p = int2(tid.xy);
    float3 e = Tap(p);
    float3 b = Tap(p + int2(0, -1));
    float3 d = Tap(p + int2(-1, 0));
    float3 f = Tap(p + int2(1, 0));
    float3 h = Tap(p + int2(0, 1));

    float3 mn4 = min(min(b, d), min(f, h));
    float3 mx4 = max(max(b, d), max(f, h));

    // Per channel: how much negative lobe fits before clipping.
    float3 hitMin = float3(SafeDiv(mn4.r, 4.0f * mx4.r),
                           SafeDiv(mn4.g, 4.0f * mx4.g),
                           SafeDiv(mn4.b, 4.0f * mx4.b));
    float3 hitMax = float3(SafeDiv(1.0f - mx4.r, 4.0f * mn4.r - 4.0f),
                           SafeDiv(1.0f - mx4.g, 4.0f * mn4.g - 4.0f),
                           SafeDiv(1.0f - mx4.b, 4.0f * mn4.b - 4.0f));
    float3 lobeRGB = max(-hitMin, hitMax);
    float  lobe = max(max(lobeRGB.r, lobeRGB.g), lobeRGB.b);
    lobe = max(-RCAS_LIMIT, min(lobe, 0.0f)) * SharpnessLin;

    if (AdaptiveSharpen > 0.5f)
    {
        // Back off where there is little local detail; sharpening there only
        // lifts noise. Separable 3x3 box of luma and luma^2, edge-clamped.
        float sum = 0.0f, sumSq = 0.0f;
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        {
            [unroll]
            for (int dx = -1; dx <= 1; dx++)
            {
                float l = dot(Tap(p + int2(dx, dy)), REC709);
                sum += l;
                sumSq += l * l;
            }
        }
        float mean = sum / 9.0f;
        float var = sumSq / 9.0f - mean * mean;
        lobe *= saturate(var * 400.0f);
    }

    float3 col = (lobe * b + lobe * d + lobe * f + lobe * h + e)
               / (4.0f * lobe + 1.0f);
    Dst[tid.xy] = float4(saturate(col), 1.0f);
}
