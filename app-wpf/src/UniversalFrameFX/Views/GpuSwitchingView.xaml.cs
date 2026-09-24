using System.Windows;
using System.Windows.Controls;
using Microsoft.Win32;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class GpuSwitchingView : UserControl
{
    private TextBox _exeBox = null!;
    private ComboBox _prefBox = null!;
    private TextBlock _current = null!;

    public GpuSwitchingView()
    {
        InitializeComponent();
        Build();
    }

    private void Build()
    {
        Root.Children.Add(Ui.SectionLabel("Graphics preference"));
        Root.Children.Add(Ui.Title("GPU switching"));
        Root.Children.Add(new TextBlock
        {
            Text = "Choose which GPU a game launches on. This writes the same per-app preference "
                 + "Windows uses under Settings ▸ Display ▸ Graphics — no driver hacks. On a laptop, "
                 + "\"High performance\" maps to your discrete GPU and \"Power saving\" to the integrated one.",
            FontSize = 13,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 0, 0, 18),
        });

        // Detected adapters summary
        var gpuStack = new StackPanel();
        gpuStack.Children.Add(Ui.SectionLabel("Detected adapters"));
        if (AppState.Gpus.Count == 0)
        {
            gpuStack.Children.Add(Ui.Muted("No adapters detected."));
        }
        else
        {
            foreach (var g in AppState.Gpus)
            {
                var role = g.Vendor is "NVIDIA" or "AMD" && !g.Name.Contains("Radeon Graphics", StringComparison.OrdinalIgnoreCase)
                    ? "likely discrete"
                    : g.Vendor == "Intel" || g.Name.Contains("Graphics", StringComparison.OrdinalIgnoreCase)
                        ? "likely integrated"
                        : "";
                gpuStack.Children.Add(Ui.KeyValue(
                    g.Name,
                    string.IsNullOrEmpty(role) ? g.VramDisplay : $"{g.VramDisplay} · {role}"));
            }
        }
        Root.Children.Add(Ui.Card(gpuStack));

        // Per-app assignment
        var form = new StackPanel();
        form.Children.Add(Ui.SectionLabel("Assign a game"));

        form.Children.Add(new TextBlock { Text = "Executable", Style = (Style)Application.Current.FindResource("FieldLabel") });
        var row = new Grid();
        row.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        row.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(8) });
        row.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        _exeBox = new TextBox();
        _exeBox.LostFocus += (_, _) => RefreshCurrent();
        Grid.SetColumn(_exeBox, 0);
        var browse = new Button
        {
            Content = "Browse…",
            Style = (Style)Application.Current.FindResource("GhostButton"),
            Width = 96,
        };
        browse.Click += Browse_Click;
        Grid.SetColumn(browse, 2);
        row.Children.Add(_exeBox);
        row.Children.Add(browse);
        form.Children.Add(row);

        form.Children.Add(new TextBlock { Text = "Preference", Style = (Style)Application.Current.FindResource("FieldLabel") });
        _prefBox = new ComboBox { Width = 320, HorizontalAlignment = HorizontalAlignment.Left };
        foreach (GpuPreferenceService.Preference p in Enum.GetValues(typeof(GpuPreferenceService.Preference)))
            _prefBox.Items.Add(GpuPreferenceService.Describe(p));
        _prefBox.SelectedIndex = 2; // High performance
        form.Children.Add(_prefBox);

        _current = new TextBlock
        {
            FontSize = 12.5,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 14, 0, 0),
        };
        form.Children.Add(_current);

        var apply = new Button
        {
            Content = "Apply preference",
            Style = (Style)Application.Current.FindResource("PrimaryButton"),
            Width = 170,
            HorizontalAlignment = HorizontalAlignment.Left,
            Margin = new Thickness(0, 16, 0, 0),
        };
        apply.Click += Apply_Click;
        form.Children.Add(apply);

        Root.Children.Add(Ui.Card(form));
    }

    private void Browse_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog { Filter = "Executables (*.exe)|*.exe|All files (*.*)|*.*" };
        if (dlg.ShowDialog() == true)
        {
            _exeBox.Text = dlg.FileName;
            RefreshCurrent();
        }
    }

    private void RefreshCurrent()
    {
        var path = _exeBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(path))
        {
            _current.Text = "";
            return;
        }
        var pref = GpuPreferenceService.Get(path);
        _current.Text = "Current: " + GpuPreferenceService.Describe(pref);
        _prefBox.SelectedIndex = (int)pref;
    }

    private void Apply_Click(object sender, RoutedEventArgs e)
    {
        var path = _exeBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(path))
        {
            MessageBox.Show("Pick the game's .exe first.", "Universal FrameFX");
            return;
        }
        try
        {
            var pref = (GpuPreferenceService.Preference)_prefBox.SelectedIndex;
            GpuPreferenceService.Set(path, pref);
            RefreshCurrent();
            MessageBox.Show(
                $"Set '{System.IO.Path.GetFileName(path)}' to {GpuPreferenceService.Describe(pref)}.\n"
                + "Restart the game for it to take effect.",
                "Universal FrameFX");
        }
        catch (Exception ex)
        {
            MessageBox.Show("Could not set the preference:\n" + ex.Message, "Universal FrameFX");
        }
    }
}
