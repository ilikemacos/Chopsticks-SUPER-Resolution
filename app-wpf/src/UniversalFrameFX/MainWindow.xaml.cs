using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Interop;
using UniversalFrameFX.Services;
using UniversalFrameFX.Views;

namespace UniversalFrameFX;

public partial class MainWindow : Window
{
    private readonly List<Button> _navButtons = new();

    private static readonly (string Label, Func<UserControl> Factory)[] Pages =
    {
        ("Dashboard", () => new DashboardView()),
        ("GPU", () => new GpuView()),
        ("Upscaling", () => new UpscalingView()),
        ("Frame Generation", () => new FrameGenView()),
        ("Profiles", () => new ProfilesView()),
        ("About", () => new AboutView()),
    };

    public MainWindow()
    {
        InitializeComponent();
        VersionText.Text = "v" + AppState.Version;
        BuildNav();
        SourceInitialized += (_, _) => DarkTitleBar.Apply(this);
        Select(0);
    }

    private void BuildNav()
    {
        for (int i = 0; i < Pages.Length; i++)
        {
            var index = i;
            var btn = new Button
            {
                Content = Pages[i].Label,
                Style = (Style)FindResource("NavButton"),
                Tag = index,
            };
            btn.Click += (_, _) => Select(index);
            NavPanel.Children.Add(btn);
            _navButtons.Add(btn);
        }
    }

    private void Select(int index)
    {
        for (int i = 0; i < _navButtons.Count; i++)
        {
            bool active = i == index;
            _navButtons[i].Background = active
                ? (System.Windows.Media.Brush)FindResource("Accent")
                : System.Windows.Media.Brushes.Transparent;
            _navButtons[i].Foreground = active
                ? System.Windows.Media.Brushes.White
                : (System.Windows.Media.Brush)FindResource("Muted");
        }

        try
        {
            PageHost.Content = Pages[index].Factory();
        }
        catch (Exception ex)
        {
            PageHost.Content = new TextBlock
            {
                Text = "This page could not be loaded:\n" + ex.Message,
                Foreground = (System.Windows.Media.Brush)FindResource("Bad"),
                TextWrapping = TextWrapping.Wrap,
            };
        }
    }
}
