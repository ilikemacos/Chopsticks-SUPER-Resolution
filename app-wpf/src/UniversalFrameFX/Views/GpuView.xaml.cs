using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class GpuView : UserControl
{
    public GpuView()
    {
        InitializeComponent();
        Build();
    }

    private void Build()
    {
        Root.Children.Add(Ui.SectionLabel("Hardware"));
        Root.Children.Add(Ui.Title("Detected hardware"));
        Root.Children.Add(new TextBlock
        {
            Text = "Read from WMI and the driver — never guessed from the brand.",
            FontSize = 13,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 0, 0, 18),
        });

        if (AppState.Gpus.Count == 0)
        {
            Root.Children.Add(Ui.Card(new TextBlock
            {
                Text = "No display adapter was reported by WMI. If you are running inside a VM or a "
                     + "remote session, GPU data may be unavailable.",
                Foreground = Ui.Brush("Bad"),
                TextWrapping = TextWrapping.Wrap,
            }));
            return;
        }

        var plat = AppState.Platform;

        foreach (var g in AppState.Gpus)
        {
            var stack = new StackPanel();

            var header = new StackPanel { Orientation = Orientation.Horizontal };
            header.Children.Add(new TextBlock
            {
                Text = g.Name,
                FontSize = 16,
                FontWeight = FontWeights.SemiBold,
                Foreground = Ui.Brush("Text"),
                Margin = new Thickness(0, 0, 12, 10),
                TextWrapping = TextWrapping.Wrap,
            });
            stack.Children.Add(header);

            stack.Children.Add(Ui.KeyValue("Vendor", g.Vendor));
            stack.Children.Add(Ui.KeyValue("Architecture", g.ArchDisplay));
            stack.Children.Add(Ui.KeyValue("VRAM", g.VramDisplay));
            stack.Children.Add(Ui.KeyValue("Driver version", string.IsNullOrEmpty(g.DriverVersion) ? "unknown" : g.DriverVersion));
            stack.Children.Add(Ui.KeyValue("PCI IDs", $"VEN_{g.VendorId:X4}  DEV_{g.DeviceId:X4}"));
            var dx11Text = plat.DX11
                ? (string.IsNullOrEmpty(plat.Dx11FeatureLevel)
                    ? "Device created" : $"Feature level {plat.Dx11FeatureLevel}")
                : "No device";
            stack.Children.Add(Ui.KeyValue("DirectX 11", dx11Text, plat.DX11 ? "Ok" : "Bad"));
            stack.Children.Add(Ui.KeyValue("DirectX 12", plat.DX12 ? "Device created" : "No device",
                                           plat.DX12 ? "Ok" : "Bad"));
            stack.Children.Add(Ui.KeyValue("Vulkan", plat.VulkanLoaderPresent ? "Loader present" : "Not detected", plat.VulkanLoaderPresent ? "Ok" : "Muted"));

            Root.Children.Add(Ui.Card(stack));
        }
    }
}
