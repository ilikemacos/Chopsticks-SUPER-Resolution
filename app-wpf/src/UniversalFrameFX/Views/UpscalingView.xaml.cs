using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class UpscalingView : UserControl
{
    private TextBox _widthBox = null!;
    private TextBox _heightBox = null!;
    private ComboBox _modeBox = null!;
    private TextBlock _result = null!;

    public UpscalingView()
    {
        InitializeComponent();
        Build();
    }

    private void Build()
    {
        Root.Children.Add(Ui.SectionLabel("Super resolution"));
        Root.Children.Add(Ui.Title("Upscaling"));
        Root.Children.Add(new TextBlock
        {
            Text = "What each upscaler can honestly do on your hardware.",
            FontSize = 13,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 0, 0, 18),
        });

        var gpu = AppState.PrimaryGpu;
        foreach (var u in CapabilityService.Upscalers(gpu, AppState.Platform))
        {
            var stack = new StackPanel();

            var header = new Grid();
            header.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            header.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

            var name = Ui.Heading(u.Name);
            Grid.SetColumn(name, 0);
            var pill = Ui.Pill(u.Available ? "Available" : "Unavailable", u.Available ? "Ok" : "Bad");
            Grid.SetColumn(pill, 1);
            header.Children.Add(name);
            header.Children.Add(pill);
            stack.Children.Add(header);

            stack.Children.Add(new TextBlock
            {
                Text = u.Available ? u.Requirements : u.Reason,
                FontSize = 12.5,
                Foreground = Ui.Brush("Muted"),
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 8, 0, 0),
            });

            Root.Children.Add(Ui.Card(stack));
        }

        Root.Children.Add(BuildEstimator());
    }

    private Border BuildEstimator()
    {
        var stack = new StackPanel();
        stack.Children.Add(Ui.Heading("Render-resolution estimate"));
        stack.Children.Add(new TextBlock
        {
            Text = "Configuration estimate — not a measured frame rate.",
            FontSize = 12,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 4, 0, 14),
        });

        var row = new StackPanel { Orientation = Orientation.Horizontal };

        row.Children.Add(Field("Output width", _widthBox = MakeNumberBox("2560")));
        row.Children.Add(Field("Output height", _heightBox = MakeNumberBox("1440")));

        _modeBox = new ComboBox { Width = 170, VerticalAlignment = VerticalAlignment.Bottom };
        foreach (var m in CapabilityService.QualityModes) _modeBox.Items.Add(m);
        _modeBox.SelectedItem = "Quality";
        row.Children.Add(Field("Quality mode", _modeBox));

        var calc = new Button
        {
            Content = "Calculate",
            Style = (Style)Application.Current.FindResource("PrimaryButton"),
            Width = 120,
            VerticalAlignment = VerticalAlignment.Bottom,
            Margin = new Thickness(4, 0, 0, 0),
        };
        calc.Click += (_, _) => Calculate();
        row.Children.Add(calc);

        stack.Children.Add(row);

        _result = new TextBlock
        {
            FontSize = 14,
            Foreground = Ui.Brush("Accent"),
            Margin = new Thickness(0, 16, 0, 0),
            TextWrapping = TextWrapping.Wrap,
        };
        stack.Children.Add(_result);

        Calculate();
        return Ui.Card(stack);
    }

    private static TextBox MakeNumberBox(string value) => new()
    {
        Text = value,
        Width = 90,
    };

    private static StackPanel Field(string label, FrameworkElement control)
    {
        var sp = new StackPanel { Margin = new Thickness(0, 0, 14, 0) };
        sp.Children.Add(new TextBlock
        {
            Text = label,
            FontSize = 12,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 0, 0, 5),
        });
        sp.Children.Add(control);
        return sp;
    }

    private void Calculate()
    {
        if (!int.TryParse(_widthBox.Text, out var ow) || !int.TryParse(_heightBox.Text, out var oh)
            || ow <= 0 || oh <= 0)
        {
            _result.Text = "Enter valid output width and height.";
            _result.Foreground = Ui.Brush("Bad");
            return;
        }

        var mode = _modeBox.SelectedItem as string ?? "Quality";
        var r = CapabilityService.ScaleRatio(mode);
        var iw = (int)Math.Round(ow / r);
        var ih = (int)Math.Round(oh / r);
        _result.Foreground = Ui.Brush("Accent");
        _result.Text = $"Internal {iw} × {ih}  →  Output {ow} × {oh}   ({mode}, {r:0.0}× scale)";
    }
}
