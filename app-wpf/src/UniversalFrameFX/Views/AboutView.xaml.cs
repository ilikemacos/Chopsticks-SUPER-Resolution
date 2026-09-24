using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class AboutView : UserControl
{
    public AboutView()
    {
        InitializeComponent();
        Build();
    }

    private void Build()
    {
        Root.Children.Add(Ui.SectionLabel("Universal FrameFX"));
        Root.Children.Add(Ui.Title("About"));

        var stack = new StackPanel();
        stack.Children.Add(new TextBlock
        {
            Text = "Universal FrameFX",
            FontSize = 18,
            FontWeight = FontWeights.SemiBold,
            Foreground = Ui.Brush("Text"),
        });
        stack.Children.Add(Ui.Muted("Version " + AppState.Version + " · Open source (MIT)"));

        stack.Children.Add(Para(
            "A unified interface for configuring FSR, XeSS and frame generation for the games you "
            + "already own. This is the native desktop edition, built with .NET 8 / WPF."));

        stack.Children.Add(Para(
            "GPU data is read from WMI and the driver — never guessed from the brand. VRAM comes "
            + "from the driver's registry entry, so cards above 4 GB report correctly."));

        stack.Children.Add(Para(
            "It never fakes support for a graphics technology, never fabricates FPS, and only modifies "
            + "a game's own folder after taking a backup. It refuses to touch anti-cheat-protected folders."));

        stack.Children.Add(Para(
            "GPU switching writes the per-app graphics preference Windows itself uses, and the app "
            + "checks GitHub for newer releases on startup so updates are one click."));

        stack.Children.Add(new TextBlock
        {
            Text = "Data directory: " + AppPaths.Root,
            FontSize = 12,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 8, 0, 0),
            TextWrapping = TextWrapping.Wrap,
        });

        Root.Children.Add(Ui.Card(stack));
    }

    private static TextBlock Para(string text) => new()
    {
        Text = text,
        FontSize = 13,
        Foreground = Ui.Brush("Muted"),
        TextWrapping = TextWrapping.Wrap,
        Margin = new Thickness(0, 14, 0, 0),
    };
}
