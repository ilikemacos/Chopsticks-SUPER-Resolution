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

                list.Add(new GpuInfo
                {
                    Name = name,
                    Vendor = vendor,
                    Arch = ArchOf(vendor, dev),
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

    private static string ArchOf(string vendor, int devId) => vendor switch
    {
        "NVIDIA" =>
            devId >= 0x2B00 ? "Blackwell" :
            devId >= 0x2600 ? "Ada Lovelace" :
            devId >= 0x2200 ? "Ampere" :
            devId >= 0x1E00 ? "Turing" : "Pascal or older",
        "AMD" =>
            devId >= 0x7500 ? "RDNA 4" :
            devId >= 0x7440 ? "RDNA 3" :
            devId >= 0x73A0 ? "RDNA 2" :
            devId >= 0x7310 ? "RDNA 1" : "GCN or older",
        "Intel" =>
            devId >= 0xE200 ? "Xe2 (Battlemage)" :
            devId >= 0x5690 ? "Xe-HPG (Arc Alchemist)" : "Xe-LP or older",
        _ => "Unknown",
    };

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
