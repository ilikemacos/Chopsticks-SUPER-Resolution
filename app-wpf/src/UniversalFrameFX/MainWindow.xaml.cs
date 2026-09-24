using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Interop;
using UniversalFrameFX.Services;
using UniversalFrameFX.Views;

namespace UniversalFrameFX;

public partial class MainWindow : Window
{
    private readonly List<Button> _navButtons = new();
    private UpdateService.UpdateInfo? _update;

    private static readonly (string Label, Func<UserControl> Factory)[] Pages =
    {
        ("Dashboard", () => new DashboardView()),
        ("GPU", () => new GpuView()),
        ("Upscaling", () => new UpscalingView()),
        ("Frame Generation", () => new FrameGenView()),
        ("GPU Switching", () => new GpuSwitchingView()),
        ("Profiles", () => new ProfilesView()),
        ("About", () => new AboutView()),
    };

    public MainWindow()
    {
        InitializeComponent();
        VersionText.Text = "v" + AppState.Version;
        BuildNav();
        SourceInitialized += (_, _) => DarkTitleBar.Apply(this);
        Loaded += async (_, _) => await CheckForUpdatesAsync();
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
                ? (System.Windows.Media.Brush)FindResource("AccentSoft")
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

    private async Task CheckForUpdatesAsync()
    {
        _update = await UpdateService.CheckAsync();
        if (_update == null) return;

        UpdatePillText.Text = $"Update available — {_update.TagName}";
        UpdatePill.Visibility = Visibility.Visible;
    }

    private async void Update_Click(object sender, RoutedEventArgs e)
    {
        if (_update == null) return;

        // No packaged asset — just open the releases page.
        if (string.IsNullOrEmpty(_update.AssetUrl))
        {
            OpenInBrowser(string.IsNullOrEmpty(_update.HtmlUrl)
                ? "https://github.com/ilikemacos/Chopsticks-SUPER-Resolution/releases/latest"
                : _update.HtmlUrl);
            return;
        }

        UpdateButton.IsEnabled = false;
        UpdateStatus.Visibility = Visibility.Visible;
        UpdateStatus.Text = "Downloading… 0%";
        var progress = new Progress<double>(p => UpdateStatus.Text = $"Downloading… {p * 100:0}%");

        try
        {
            var path = await UpdateService.DownloadAsync(_update.AssetUrl, progress);
            UpdateStatus.Text = "Starting the new version…";
            Process.Start(new ProcessStartInfo(path) { UseShellExecute = true });
            Application.Current.Shutdown();
        }
        catch (Exception ex)
        {
            UpdateStatus.Text = "Download failed — opening the page instead.";
            UpdateButton.IsEnabled = true;
            OpenInBrowser(string.IsNullOrEmpty(_update.HtmlUrl)
                ? "https://github.com/ilikemacos/Chopsticks-SUPER-Resolution/releases/latest"
                : _update.HtmlUrl);
            _ = ex;
        }
    }

    private static void OpenInBrowser(string url)
    {
        try { Process.Start(new ProcessStartInfo(url) { UseShellExecute = true }); }
        catch { /* nothing else to do */ }
    }
}
