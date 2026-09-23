using System.Windows;
using System.Windows.Controls;
using UniversalFrameFX.Controls;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class FrameGenView : UserControl
{
    public FrameGenView()
    {
        InitializeComponent();
        Build();
    }

    private void Build()
    {
        Root.Children.Add(Ui.Title("Frame generation"));

        foreach (var f in CapabilityService.FrameGenerators(AppState.PrimaryGpu, AppState.Platform))
        {
            var stack = new StackPanel();

            var header = new Grid();
            header.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            header.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            var name = Ui.Heading(f.Name);
            Grid.SetColumn(name, 0);
            var pill = Ui.Pill(f.State, "Warn");
            Grid.SetColumn(pill, 1);
            header.Children.Add(name);
            header.Children.Add(pill);
            stack.Children.Add(header);

            stack.Children.Add(new TextBlock
            {
                Text = f.Note,
                FontSize = 12.5,
                Foreground = Ui.Brush("Muted"),
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 8, 0, 0),
            });

            Root.Children.Add(Ui.Card(stack));
        }

        Root.Children.Add(Ui.Card(new TextBlock
        {
            Text = "Frame generation inserts interpolated frames and needs the game's swapchain and engine "
                 + "motion vectors. It cannot be bolted onto a title that did not ship with it, so this "
                 + "page reports honest state rather than promising a universal toggle.",
            FontSize = 12.5,
            Foreground = Ui.Brush("Muted"),
            TextWrapping = TextWrapping.Wrap,
        }));
    }
}
