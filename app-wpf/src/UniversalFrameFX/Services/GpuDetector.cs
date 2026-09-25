using System.Management;
using System.Text.RegularExpressions;
using Microsoft.Win32;
using UniversalFrameFX.Models;

namespace UniversalFrameFX.Services;

/// <summary>Real GPU detection via WMI (Win32_VideoController) plus accurate VRAM from the driver's registry key.</summary>
public static class GpuDetector
{
    private const string DisplayClassKey =
        @"SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}";

    public static List<GpuInfo> Enumerate()
    {
        var list = new List<GpuInfo>();
        try
        {
            using var searcher = new ManagementObjectSearcher(
                "SELECT Name, PNPDeviceID, DriverVersion, AdapterRAM FROM Win32_VideoController");
            foreach (ManagementObject mo in searcher.Get())
            {
                var name = mo["Name"] as string ?? "Unknown GPU";
                var pnp = mo["PNPDeviceID"] as string ?? "";
                int ven = ParseHex(pnp, "VEN_");
                int dev = ParseHex(pnp, "DEV_");
                string vendor = VendorOf(ven);

                long vram = VramFromRegistry(name);
                if (vram <= 0)
                {
                    try
                    {
                        var raw = mo["AdapterRAM"];
                        if (raw != null) vram = Convert.ToInt64(raw);
                    }
                    catch { /* AdapterRAM is capped at 4 GB and can be absent; ignore. */ }
                }

                var (arch, archCertainty) = ArchOf(vendor, dev);
                list.Add(new GpuInfo
                {
                    Name = name,
                    Vendor = vendor,
                    Arch = arch,
                    ArchCertainty = archCertainty,
                    VendorId = ven,
                    DeviceId = dev,
                    VramBytes = vram,
                    DriverVersion = mo["DriverVersion"] as string ?? "",
                });
            }
        }
        catch
        {
            // WMI unavailable; return whatever we have (possibly empty).
        }

        // Put a real, non-basic adapter first so the dashboard shows the discrete GPU.
        list.Sort((a, b) => Rank(b).CompareTo(Rank(a)));
        return list;
    }

    private static int Rank(GpuInfo g)
    {
        if (g.Vendor is "NVIDIA" or "AMD") return 3;
        if (g.Vendor == "Intel") return 2;
        if (g.Vendor.StartsWith("Microsoft")) return 0;
        return 1;
    }

    private static int ParseHex(string pnp, string token)
    {
        var m = Regex.Match(pnp, token + "([0-9A-Fa-f]{4})");
        return m.Success ? Convert.ToInt32(m.Groups[1].Value, 16) : 0;
    }

    private static string VendorOf(int venId) => venId switch
    {
        0x10DE => "NVIDIA",
        0x1002 => "AMD",
        0x1022 => "AMD",
        0x8086 => "Intel",
        0x1414 => "Microsoft Basic Render",
        _ => "Unknown",
    };

    /// <summary>
    /// Infers architecture from the PCI device ID.
    /// </summary>
    /// <remarks>
    /// This is a lookup over ID <i>ranges</i>, and device IDs are not monotonic by
    /// architecture — mobile parts, refreshes and workstation cards all break the
    /// ordering. So the result carries a <see cref="Certainty"/> and callers must
    /// not treat <see cref="Certainty.Unknown"/> as evidence either way. It is
    /// also the reason the FSR 4 row can report "undetermined" rather than
    /// "unavailable".
    /// </remarks>
    private static (string Arch, Certainty Certainty) ArchOf(string vendor, int devId)
    {
        switch (vendor)
        {
            case "NVIDIA":
                if (devId is >= 0x2B00 and <= 0x2BFF) return ("Blackwell", Certainty.Inferred);
                if (devId is >= 0x2600 and <= 0x28FF) return ("Ada Lovelace", Certainty.Inferred);
                if (devId is >= 0x2200 and <= 0x25FF) return ("Ampere", Certainty.Inferred);
                if (devId is >= 0x1E00 and <= 0x21FF) return ("Turing", Certainty.Inferred);
                if (devId is > 0x0000 and < 0x1E00) return ("Pascal or older", Certainty.Inferred);
                return ("Unknown", Certainty.Unknown);

            case "AMD":
                // RDNA 4 is the only one that gates a feature, so it is the one
                // range that must not be guessed loosely.
                if (devId is >= 0x7550 and <= 0x759F) return ("RDNA 4", Certainty.Inferred);
                if (devId is >= 0x7440 and <= 0x754F) return ("RDNA 3", Certainty.Inferred);
                if (devId is >= 0x73A0 and <= 0x743F) return ("RDNA 2", Certainty.Inferred);
                if (devId is >= 0x7310 and <= 0x739F) return ("RDNA 1", Certainty.Inferred);
                if (devId is > 0x0000 and < 0x7310) return ("GCN or older", Certainty.Inferred);
                return ("Unknown", Certainty.Unknown);

            case "Intel":
                if (devId is >= 0xE200 and <= 0xE2FF) return ("Xe2 (Battlemage)", Certainty.Inferred);
                if (devId is >= 0x5690 and <= 0x56FF) return ("Xe-HPG (Arc Alchemist)", Certainty.Inferred);
                if (devId is > 0x0000 and < 0x5690) return ("Xe-LP or older", Certainty.Inferred);
                return ("Unknown", Certainty.Unknown);

            default:
                return ("Unknown", Certainty.Unknown);
        }
    }

    /// <summary>Reads HardwareInformation.qwMemorySize for the matching adapter — accurate for cards above 4 GB.</summary>
    private static long VramFromRegistry(string name)
    {
        try
        {
            using var baseKey = Registry.LocalMachine.OpenSubKey(DisplayClassKey);
            if (baseKey == null) return 0;
            foreach (var sub in baseKey.GetSubKeyNames())
            {
                using var k = baseKey.OpenSubKey(sub);
                if (k == null) continue;
                var desc = k.GetValue("DriverDesc") as string;
                if (string.IsNullOrEmpty(desc)) continue;
                if (!(desc == name || name.Contains(desc) || desc.Contains(name))) continue;

                var qw = k.GetValue("HardwareInformation.qwMemorySize");
                switch (qw)
                {
                    case byte[] bytes when bytes.Length >= 8:
                        return BitConverter.ToInt64(bytes, 0);
                    case long l:
                        return l;
                    case int i:
                        return i;
                    case uint ui:
                        return ui;
                }
            }
        }
        catch { /* registry blocked; fall back to WMI. */ }
        return 0;
    }
}
