using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace UniversalFrameFX.Controls;

/// <summary>Small factory helpers for building consistent, themed UI in code-behind.</summary>
public static class Ui
{
    public static SolidColorBrush Brush(string key)
        => (SolidColorBrush)Application.Current.FindResource(key);

    public static Brush GradientBrush(string key)
        => (Brush)Application.Current.FindResource(key);

    public static TextBlock Title(string text) => new()
    {
        Text = text,
        FontSize = 32,
        FontWeight = FontWeights.Bold,
        Foreground = GradientBrush("HeadingBrush"),
        Margin = new Thickness(0, 0, 0, 6),
    };

    /// <summary>Small uppercase mono label, e.g. "DETECTED HARDWARE".</summary>
    public static TextBlock SectionLabel(string text) => new()
    {
        Text = text.ToUpperInvariant(),
        FontFamily = (FontFamily)Application.Current.FindResource("Mono"),
        FontSize = 11,
        Foreground = Brush("Accent"),
        Margin = new Thickness(0, 0, 0, 10),
    };

    public static TextBlock Heading(string text) => new()
    {
        Text = text,
        FontSize = 16,
        FontWeight = FontWeights.SemiBold,
        Foreground = Brush("Text"),
    };

    public static TextBlock Body(string text) => new()
    {
        Text = text,
        FontSize = 13,
        Foreground = Brush("Text"),
    };

    public static TextBlock Muted(string text) => new()
    {
        Text = text,
        FontSize = 12.5,
        Foreground = Brush("Muted"),
    };

    public static TextBlock Caption(string text) => new()
    {
        Text = text,
        FontSize = 11.5,
        Foreground = Brush("Muted"),
        TextWrapping = TextWrapping.Wrap,
    };

    public static Border Card(UIElement child, Thickness? margin = null) => new()
    {
        Style = (Style)Application.Current.FindResource("Card"),
        Margin = margin ?? new Thickness(0, 0, 0, 14),
        Child = child,
    };

    /// <summary>A coloured status pill: "Ok" (green), "Warn" (amber), "Bad" (red), "Accent".</summary>
    public static Border Pill(string text, string brushKey)
    {
        var fill = Brush(brushKey);
        return new Border
        {
            Background = new SolidColorBrush(Color.FromArgb(38, fill.Color.R, fill.Color.G, fill.Color.B)),
            BorderBrush = fill,
            BorderThickness = new Thickness(1),
            CornerRadius = new CornerRadius(999),
            Padding = new Thickness(12, 3, 12, 3),
            VerticalAlignment = VerticalAlignment.Center,
            HorizontalAlignment = HorizontalAlignment.Left,
            Child = new TextBlock
            {
                Text = text,
                FontSize = 12,
                FontWeight = FontWeights.SemiBold,
                Foreground = fill,
            },
        };
    }

    public static StackPanel Stack(double spacingBottom = 0) => new()
    {
        Orientation = Orientation.Vertical,
    };

    /// <summary>A label/value row used inside cards.</summary>
    public static Grid KeyValue(string key, string value, string? valueBrushKey = null)
    {
        var grid = new Grid { Margin = new Thickness(0, 3, 0, 3) };
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(150) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

        var k = Muted(key);
        Grid.SetColumn(k, 0);
        var v = new TextBlock
        {
            Text = value,
            FontSize = 13,
            Foreground = Brush(valueBrushKey ?? "Text"),
            TextWrapping = TextWrapping.Wrap,
        };
        Grid.SetColumn(v, 1);

        grid.Children.Add(k);
        grid.Children.Add(v);
        return grid;
    }
}
