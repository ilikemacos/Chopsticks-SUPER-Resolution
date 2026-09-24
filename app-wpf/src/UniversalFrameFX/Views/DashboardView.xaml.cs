using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class DashboardView : UserControl
{
    public DashboardView()
    {
        InitializeComponent();
        Build();
    }

    private void Build()
    {
        Root.Children.Add(Ui.SectionLabel("Overview"));
        Root.Children.Add(Ui.Title("Dashboard"));
        Root.Children.Add(new TextBlock
        {
            Text = "Your hardware and what it can honestly run.",
            FontSize = 13,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 0, 0, 18),
        });

        var gpu = AppState.PrimaryGpu;
        var plat = AppState.Platform;

        // Two summary cards side by side.
        var grid = new Grid { Margin = new Thickness(0, 0, 0, 14) };
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(14) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

        // GPU card
        var gpuStack = new StackPanel();
        gpuStack.Children.Add(Ui.SectionLabel("Primary GPU"));
        if (gpu != null)
        {
            gpuStack.Children.Add(new TextBlock
            {
                Text = gpu.Name,
                FontSize = 18,
                FontWeight = FontWeights.SemiBold,
                Foreground = Ui.Brush("Text"),
                Margin = new Thickness(0, 6, 0, 8),
                TextWrapping = TextWrapping.Wrap,
            });
            gpuStack.Children.Add(Ui.Muted($"{gpu.Vendor} · {gpu.Arch}"));
            gpuStack.Children.Add(Ui.Muted($"{gpu.VramDisplay} VRAM"));
            gpuStack.Children.Add(Ui.Muted($"Driver {gpu.DriverVersion}"));
        }
        else
        {
            gpuStack.Children.Add(new TextBlock
            {
                Text = "No GPU detected",
                FontSize = 16,
                Foreground = Ui.Brush("Bad"),
                Margin = new Thickness(0, 6, 0, 0),
            });
        }
        var gpuCard = Ui.Card(gpuStack, new Thickness(0));
        Grid.SetColumn(gpuCard, 0);

        // Platform card
        var platStack = new StackPanel();
        platStack.Children.Add(Ui.SectionLabel("Platform"));
        platStack.Children.Add(new TextBlock
        {
            Text = plat.Win11 ? "Windows 11" : (plat.Build > 0 ? "Windows (older)" : "Unknown"),
            FontSize = 18,
            FontWeight = FontWeights.SemiBold,
            Foreground = Ui.Brush("Text"),
            Margin = new Thickness(0, 6, 0, 8),
        });
        platStack.Children.Add(RuntimeLine("DirectX 11", plat.DX11));
        platStack.Children.Add(RuntimeLine("DirectX 12", plat.DX12));
        platStack.Children.Add(RuntimeLine("Vulkan", plat.Vulkan));
        var platCard = Ui.Card(platStack, new Thickness(0));
        Grid.SetColumn(platCard, 2);

        grid.Children.Add(gpuCard);
        grid.Children.Add(platCard);
        Root.Children.Add(grid);

        // Honesty note
        var note = new TextBlock
        {
            Text = "Universal FrameFX configures the upscalers a game already supports and manages "
                 + "profiles and backups. It does not add FSR / XeSS or frame generation to games that "
                 + "were not built for them, and it never fabricates FPS. The resolution figures on the "
                 + "Upscaling page are configuration estimates, labelled as such.",
            FontSize = 13,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
        };
        Root.Children.Add(Ui.Card(note));
    }

    private static StackPanel RuntimeLine(string label, bool ok)
    {
        var sp = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 3, 0, 3) };
        sp.Children.Add(new TextBlock
        {
            Text = ok ? "●  " : "○  ",
            Foreground = Ui.Brush(ok ? "Ok" : "Muted"),
            FontSize = 13,
        });
        sp.Children.Add(new TextBlock
        {
            Text = label + (ok ? "" : "  (not detected)"),
            Foreground = Ui.Brush(ok ? "Text" : "Muted"),
            FontSize = 13,
        });
        return sp;
    }
}
