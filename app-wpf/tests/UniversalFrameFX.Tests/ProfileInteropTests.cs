using System.Text.Json;
using UniversalFrameFX.Models;
using Xunit;

namespace UniversalFrameFX.Tests;

/// <summary>
/// Pins the profile JSON contract against the web console.
/// </summary>
/// <remarks>
/// The two apps use different names for the same settings: this app reads
/// <c>upscaler</c> (a display string) and a display-string <c>quality</c>, while
/// the website uses <c>method</c> and preset ids. The site claims a profile
/// exported there imports here unchanged, so the exact bytes it writes have to
/// be exercised — otherwise the claim is only an assumption, and the failure
/// mode is silent: missing fields deserialise to this class's defaults, so a
/// profile saved as CSR would quietly come back as FSR3.
///
/// The literal below is the output of <c>toInteropProfile</c> in
/// <c>website/lib/engine.ts</c>. If that function changes, this test must be
/// updated with it — that is the point of pinning it here.
/// </remarks>
public sealed class ProfileInteropTests
{
    private const string WebConsoleExport = """
    {
      "schema": 2,
      "name": "Test Game",
      "executablePath": "C:\\Games\\Test\\test.exe",
      "mode": "external",
      "api": "Auto",
      "method": "csr",
      "quality": "Balanced",
      "qualityId": "balanced",
      "upscaler": "CSR",
      "renderWidth": 1506,
      "renderHeight": 847,
      "outputWidth": 2560,
      "outputHeight": 1440,
      "sharpness": 0.5,
      "frameGen": "None",
      "frameGenEnabled": false,
      "hdr": false,
      "lowLatency": false,
      "launchArgs": "",
      "notes": "",
      "safeMode": true
    }
    """;

    private static GameProfile Parse(string json) =>
        JsonSerializer.Deserialize<GameProfile>(json)
        ?? throw new InvalidDataException("could not parse the profile");

    [Fact]
    public void AWebConsoleExportKeepsItsUpscalerAndQuality()
    {
        var p = Parse(WebConsoleExport);

        // These two are the ones that were silently lost before the site wrote
        // both spellings: without `upscaler` the default is FSR3, which is a
        // native-only method, and it would have replaced an external one.
        Assert.Equal("CSR", p.Upscaler);
        Assert.Equal("Balanced", p.Quality);

        Assert.Equal("Test Game", p.Name);
        Assert.Equal(@"C:\Games\Test\test.exe", p.ExecutablePath);
        Assert.Equal(2560, p.OutputWidth);
        Assert.Equal(1440, p.OutputHeight);
        Assert.Equal(0.5, p.Sharpness, 6);
        Assert.False(p.FrameGenEnabled);
    }

    [Fact]
    public void FieldsThisAppDoesNotKnowAreIgnoredRatherThanFatal()
    {
        // mode / hdr / lowLatency / safeMode / launchArgs / notes / qualityId are
        // schema-2 only. Deserialising must not throw on them.
        var p = Parse(WebConsoleExport);
        Assert.Equal(2, p.Schema);
    }

    [Fact]
    public void TheQualityStringIsOneThisAppOffers()
    {
        var p = Parse(WebConsoleExport);
        Assert.Contains(p.Quality, Services.CapabilityService.QualityModes);
    }

    [Fact]
    public void AMissingUpscalerStillFallsBackRatherThanThrowing()
    {
        // The old website behaviour, kept as a regression guard: this is what a
        // schema-2 file written without the desktop field names looks like, and
        // it must be read as the default rather than crashing the import.
        var p = Parse("""{"schema":2,"name":"No upscaler field","method":"csr"}""");
        Assert.Equal("FSR3", p.Upscaler);
        Assert.Equal("Quality", p.Quality);
    }

    [Fact]
    public void CsrIsSelectableInTheProfileEditor()
    {
        // Round-tripping a CSR profile through the editor requires CSR to be in
        // the list, otherwise the combo cannot show it and saving rewrites it.
        Assert.Contains("CSR", ProfileUpscalers.All);
    }
}
