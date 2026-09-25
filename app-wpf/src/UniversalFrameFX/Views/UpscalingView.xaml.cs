using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Csr.Core;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Models;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class UpscalingView : UserControl
{
    // Preview input is capped so the CPU reference stays interactive. The shipped
    // GPU path is a compute shader; this panel exists to show real CSR output and
    // a real timing, not to be the fast path.
    private const int PreviewMaxInputWidth = 640;
    private const int PreviewMaxInputHeight = 360;

    private TextBox _widthBox = null!;
    private TextBox _heightBox = null!;
    private ComboBox _modeBox = null!;
    private TextBlock _result = null!;

    private Image _beforeImage = null!;
    private Image _afterImage = null!;
    private TextBlock _previewStatus = null!;
    private Button _pickButton = null!;
    private ComboBox _previewModeBox = null!;
    private string? _loadedPath;

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
            Text = "What each upscaler can honestly do on your hardware. "
                 + "Only CSR can be applied without the game supporting it.",
            FontSize = 13,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 0, 0, 18),
        });

        var gpu = AppState.PrimaryGpu;
        foreach (var u in CapabilityService.Upscalers(gpu, AppState.Platform))
            Root.Children.Add(Ui.Card(BuildRow(u)));

        Root.Children.Add(BuildCsrPreview());
        Root.Children.Add(BuildEstimator());
    }

    private static StackPanel BuildRow(CapabilityRow u)
    {
        var stack = new StackPanel();

        var header = new Grid();
        header.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        header.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        header.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

        var name = Ui.Heading(u.Name);
        Grid.SetColumn(name, 0);
        header.Children.Add(name);

        // The integration pill is the important one: it separates "your hardware
        // allows this" from "this application can actually do it for you".
        var integration = Ui.Pill(
            u.Integration switch
            {
                Integration.External => "Works on any app",
                Integration.Native => "Needs game support",
                _ => "Not possible",
            },
            // Green only when this build can actually do it. "Works on any app" is
            // true of the technology; a Preview row has not earned the colour.
            u.Integration == Integration.External && u.Stage == Stage.Shipping
                ? "Ok" : "Muted");
        integration.Margin = new Thickness(0, 0, 8, 0);
        Grid.SetColumn(integration, 1);
        header.Children.Add(integration);

        var status = Ui.Pill(u.StatusText, u.StatusBrush);
        Grid.SetColumn(status, 2);
        header.Children.Add(status);

        stack.Children.Add(header);

        stack.Children.Add(new TextBlock
        {
            Text = u.Requirements,
            FontSize = 12.5,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 8, 0, 0),
        });

        // Show the reason whenever there is one, not only when unavailable — an
        // "undetermined" row has the most important reason of all.
        if (!string.IsNullOrWhiteSpace(u.Reason))
        {
            stack.Children.Add(new TextBlock
            {
                Text = u.Reason,
                FontSize = 12,
                Foreground = Ui.Brush(u.Determined is null ? "Warn" : "Bad"),
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 6, 0, 0),
            });
        }

        return stack;
    }

    private Border BuildCsrPreview()
    {
        var stack = new StackPanel();
        stack.Children.Add(Ui.Heading("CSR preview"));
        stack.Children.Add(new TextBlock
        {
            Text = "Runs the real CSR upscaler on an image so you can judge it yourself. "
                 + "This is the CPU reference, verified against the same golden vectors as "
                 + "the GPU shader — not a mock-up. Bilinear is shown alongside as the "
                 + "baseline CSR has to beat.",
            FontSize = 12,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 4, 0, 14),
        });

        var controls = new StackPanel { Orientation = Orientation.Horizontal };
        _pickButton = new Button
        {
            Content = "Choose an image…",
            Style = (Style)Application.Current.FindResource("PrimaryButton"),
            Width = 160,
            VerticalAlignment = VerticalAlignment.Bottom,
        };
        _pickButton.Click += (_, _) => PickAndRun();
        controls.Children.Add(_pickButton);

        _previewModeBox = new ComboBox { Width = 170, VerticalAlignment = VerticalAlignment.Bottom, Margin = new Thickness(12, 0, 0, 0) };
        foreach (var m in CapabilityService.QualityModes) _previewModeBox.Items.Add(m);
        _previewModeBox.SelectedItem = "Quality";
        _previewModeBox.SelectionChanged += (_, _) =>
        {
            if (_loadedPath is not null) RunPreview(_loadedPath);
        };
        controls.Children.Add(_previewModeBox);
        stack.Children.Add(controls);

        _previewStatus = new TextBlock
        {
            Text = "No image loaded.",
            FontSize = 12,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 12, 0, 10),
        };
        stack.Children.Add(_previewStatus);

        var pair = new Grid();
        pair.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        pair.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

        _beforeImage = MakePreviewImage();
        _afterImage = MakePreviewImage();
        pair.Children.Add(LabelledImage("Bilinear", _beforeImage, 0));
        pair.Children.Add(LabelledImage("CSR", _afterImage, 1));
        stack.Children.Add(pair);

        return Ui.Card(stack);
    }

    private static Image MakePreviewImage() => new()
    {
        Stretch = Stretch.Uniform,
        MaxHeight = 320,
        HorizontalAlignment = HorizontalAlignment.Left,
        SnapsToDevicePixels = true,
    };

    private static StackPanel LabelledImage(string label, Image image, int column)
    {
        var sp = new StackPanel { Margin = new Thickness(column == 0 ? 0 : 10, 0, 0, 0) };
        sp.Children.Add(new TextBlock
        {
            Text = label,
            FontSize = 11.5,
            Foreground = Ui.Brush("Muted"),
            Margin = new Thickness(0, 0, 0, 6),
        });
        sp.Children.Add(image);
        // Nearest-neighbour so the viewer sees CSR's actual pixels rather than
        // WPF's own smoothing applied on top of them.
        RenderOptions.SetBitmapScalingMode(image, BitmapScalingMode.NearestNeighbor);
        Grid.SetColumn(sp, column);
        return sp;
    }

    private void PickAndRun()
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Choose an image to upscale",
            Filter = "Images|*.png;*.jpg;*.jpeg;*.bmp;*.webp|All files|*.*",
            CheckFileExists = true,
        };
        if (dlg.ShowDialog() != true) return;
        _loadedPath = dlg.FileName;
        RunPreview(_loadedPath);
    }

    private async void RunPreview(string path)
    {
        var mode = _previewModeBox.SelectedItem as string ?? "Quality";
        double ratio = CapabilityService.ScaleRatio(mode);

        // The mode combo also starts a run, so it has to be disabled alongside the
        // button. Otherwise two runs overlap, the first to finish re-enables the
        // controls, and whichever finishes last wins the images — which can leave
        // the status line describing a different mode than the panes show.
        _pickButton.IsEnabled = false;
        _previewModeBox.IsEnabled = false;
        _previewStatus.Foreground = Ui.Brush("Muted");
        _previewStatus.Text = "Upscaling…";

        try
        {
            var result = await Task.Run(() => BuildPreview(path, ratio));

            _beforeImage.Source = result.Bilinear;
            _afterImage.Source = result.Csr;
            _previewStatus.Foreground = Ui.Brush("Accent");
            _previewStatus.Text =
                $"{result.InW} × {result.InH}  →  {result.OutW} × {result.OutH}   "
                + $"({mode}, {ratio:0.0}× per axis, {result.Pixels / 1_000_000.0:0.00} MP out)   "
                + $"CSR took {result.Milliseconds:0} ms on the CPU reference"
                + (ratio <= 1.0
                    ? "   —  at 1.0× CSR bypasses entirely, so both panes are identical"
                    : "");
        }
        catch (Exception ex) when (ex is IOException or NotSupportedException
                                  or FileFormatException or UnauthorizedAccessException
                                  or ArgumentException or FormatException
                                  or OutOfMemoryException)
        {
            _previewStatus.Foreground = Ui.Brush("Bad");
            _previewStatus.Text = $"Could not read that image: {ex.Message}";
            _beforeImage.Source = null;
            _afterImage.Source = null;
        }
        finally
        {
            _pickButton.IsEnabled = true;
            _previewModeBox.IsEnabled = true;
        }
    }

    private sealed record PreviewResult(
        BitmapSource Bilinear, BitmapSource Csr,
        int InW, int InH, int OutW, int OutH, long Pixels, double Milliseconds);

    /// <summary>
    /// Decodes, downscales to a size the CPU reference can handle interactively,
    /// then produces both a bilinear and a CSR upscale of the same input.
    /// </summary>
    private static PreviewResult BuildPreview(string path, double ratio)
    {
        var src = LoadClamped(path);
        int inW = src.PixelWidth, inH = src.PixelHeight;
        int outW = Math.Max(1, (int)Math.Round(inW * ratio));
        int outH = Math.Max(1, (int)Math.Round(inH * ratio));

        var input = ToImageBuffer(src);

        var sw = Stopwatch.StartNew();
        var csr = CsrUpscaler.Upscale(input, outW, outH);
        sw.Stop();

        // The baseline is Csr.Core's own bilinear, not WPF's resampler. WPF's
        // TransformedBitmap uses Fant, so a pane filled from it and labelled
        // "bilinear" would name the wrong filter — and it would not be the
        // baseline the quality figures in csr/README.md were measured against.
        // Csr.Core.Bilinear is pinned to the Python reference by
        // csr/tests/Csr.Core.Tests/BilinearTests.cs.
        var baseline = Bilinear.Upscale(input, outW, outH);

        return new PreviewResult(ToBitmap(baseline), ToBitmap(csr),
                                 inW, inH, outW, outH,
                                 (long)outW * outH, sw.Elapsed.TotalMilliseconds);
    }

    /// <summary>Decodes to Bgra32, downscaling so the CPU reference stays responsive.</summary>
    private static BitmapSource LoadClamped(string path)
    {
        var decoded = new BitmapImage();
        decoded.BeginInit();
        decoded.UriSource = new Uri(path);
        decoded.CacheOption = BitmapCacheOption.OnLoad;
        decoded.CreateOptions = BitmapCreateOptions.IgnoreColorProfile;
        decoded.EndInit();

        double scale = Math.Min(1.0, Math.Min(
            PreviewMaxInputWidth / (double)decoded.PixelWidth,
            PreviewMaxInputHeight / (double)decoded.PixelHeight));

        BitmapSource source = decoded;
        if (scale < 1.0)
            source = new TransformedBitmap(decoded, new ScaleTransform(scale, scale));

        var converted = new FormatConvertedBitmap(source, PixelFormats.Bgra32, null, 0);
        converted.Freeze();
        return converted;
    }

    /// <summary>Bgra32 to CSR's float RGB, via the tested converter in Csr.Core.</summary>
    private static ImageBuffer ToImageBuffer(BitmapSource src)
    {
        int w = src.PixelWidth, h = src.PixelHeight;
        int stride = w * 4;
        var bytes = new byte[stride * h];
        src.CopyPixels(bytes, stride, 0);
        return Bgra32.Decode(bytes, w, h, stride);
    }

    private static BitmapSource ToBitmap(ImageBuffer img)
    {
        var bytes = Bgra32.Encode(img, out var stride);
        var bmp = BitmapSource.Create(img.Width, img.Height, 96, 96,
                                     PixelFormats.Bgra32, null, bytes, stride);
        bmp.Freeze();
        return bmp;
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

        // Recalculate as the user types, rather than only on the button.
        _widthBox.TextChanged += (_, _) => Calculate();
        _heightBox.TextChanged += (_, _) => Calculate();
        _modeBox.SelectionChanged += (_, _) => Calculate();

        Calculate();
        return Ui.Card(stack);
    }

    private static TextBox MakeNumberBox(string value) => new() { Text = value, Width = 90 };

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
        // Guard against running before BuildEstimator has wired everything up,
        // since the TextChanged handlers fire during construction.
        if (_result is null || _widthBox is null || _heightBox is null) return;

        if (!int.TryParse(_widthBox.Text, out var ow) || !int.TryParse(_heightBox.Text, out var oh)
            || ow <= 0 || oh <= 0)
        {
            _result.Text = "Enter valid output width and height.";
            _result.Foreground = Ui.Brush("Bad");
            return;
        }

        // 16K a side is past any real display and keeps the arithmetic sane.
        const int MaxDimension = 16384;
        if (ow > MaxDimension || oh > MaxDimension)
        {
            _result.Text = $"Output must be {MaxDimension} or less per axis.";
            _result.Foreground = Ui.Brush("Bad");
            return;
        }

        var mode = _modeBox.SelectedItem as string ?? "Quality";
        var r = CapabilityService.ScaleRatio(mode);
        var iw = (int)Math.Round(ow / r);
        var ih = (int)Math.Round(oh / r);
        double pixelFraction = (double)iw * ih / ((double)ow * oh);

        _result.Foreground = Ui.Brush("Accent");
        _result.Text = r <= 1.0
            ? $"Native {ow} × {oh} — no upscaling, so CSR is bypassed entirely."
            : $"Internal {iw} × {ih}  →  Output {ow} × {oh}   ({mode}, {r:0.0}× per axis, "
              + $"{pixelFraction * 100:0.#}% of the pixels drawn)";
    }
}
