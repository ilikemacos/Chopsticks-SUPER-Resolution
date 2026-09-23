using System.Windows;
using System.Windows.Controls;
using Microsoft.Win32;
using UniversalFrameFX.Models;
using UniversalFrameFX.Services;

namespace UniversalFrameFX.Views;

public partial class ProfilesView : UserControl
{
    public ProfilesView()
    {
        InitializeComponent();
        StorageNote.Text = "Profiles are stored as JSON in\n" + AppPaths.Profiles;

        ApiBox.ItemsSource = new[] { "DirectX 11", "DirectX 12", "Vulkan" };
        UpscalerBox.ItemsSource = new[] { "None", "FSR1", "FSR2", "FSR3", "FSR4", "XeSS" };
        QualityBox.ItemsSource = CapabilityService.QualityModes;
        FrameGenBox.ItemsSource = new[] { "None", "FSR3-FG", "XeSS-FG" };

        ApiBox.SelectedItem = "DirectX 12";
        UpscalerBox.SelectedItem = "FSR3";
        QualityBox.SelectedItem = "Quality";
        FrameGenBox.SelectedItem = "None";
        WidthBox.Text = "2560";
        HeightBox.Text = "1440";

        ProfileList.SelectionChanged += ProfileList_SelectionChanged;
        Refresh();
    }

    private void Refresh()
    {
        var selected = ProfileList.SelectedItem as string;
        ProfileList.ItemsSource = null;
        var names = ProfileService.Load().Select(p => p.Name).ToList();
        ProfileList.ItemsSource = names;
        if (selected != null && names.Contains(selected)) ProfileList.SelectedItem = selected;
    }

    private void ProfileList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ProfileList.SelectedItem is not string name) return;
        var p = ProfileService.Load().FirstOrDefault(x => x.Name == name);
        if (p == null) return;

        NameBox.Text = p.Name;
        ExeBox.Text = p.ExecutablePath;
        ApiBox.SelectedItem = p.Api;
        UpscalerBox.SelectedItem = p.Upscaler;
        QualityBox.SelectedItem = p.Quality;
        FrameGenBox.SelectedItem = p.FrameGen;
        WidthBox.Text = p.OutputWidth.ToString();
        HeightBox.Text = p.OutputHeight.ToString();
        FrameGenEnabled.IsChecked = p.FrameGenEnabled;
    }

    private GameProfile Collect() => new()
    {
        Schema = 1,
        Name = NameBox.Text.Trim(),
        ExecutablePath = ExeBox.Text.Trim(),
        Api = ApiBox.SelectedItem as string ?? "DirectX 12",
        Upscaler = UpscalerBox.SelectedItem as string ?? "None",
        Quality = QualityBox.SelectedItem as string ?? "Quality",
        FrameGen = FrameGenBox.SelectedItem as string ?? "None",
        FrameGenEnabled = FrameGenEnabled.IsChecked == true,
        Sharpness = 0.5,
        OutputWidth = int.TryParse(WidthBox.Text, out var w) ? w : 2560,
        OutputHeight = int.TryParse(HeightBox.Text, out var h) ? h : 1440,
    };

    private void Browse_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog { Filter = "Executables (*.exe)|*.exe|All files (*.*)|*.*" };
        if (dlg.ShowDialog() == true) ExeBox.Text = dlg.FileName;
    }

    private void Save_Click(object sender, RoutedEventArgs e)
    {
        if (string.IsNullOrWhiteSpace(NameBox.Text))
        {
            MessageBox.Show("Enter a profile name.", "Universal FrameFX");
            return;
        }
        var p = Collect();
        ProfileService.Save(p);
        Refresh();
        ProfileList.SelectedItem = p.Name;
        MessageBox.Show($"Saved profile '{p.Name}'.", "Universal FrameFX");
    }

    private void Delete_Click(object sender, RoutedEventArgs e)
    {
        if (ProfileList.SelectedItem is not string name) return;
        ProfileService.Delete(name);
        Refresh();
    }

    private void Export_Click(object sender, RoutedEventArgs e)
    {
        if (ProfileList.SelectedItem is not string name) return;
        var p = ProfileService.Load().FirstOrDefault(x => x.Name == name);
        if (p == null) return;
        var dlg = new SaveFileDialog { Filter = "JSON (*.json)|*.json", FileName = ProfileService.Slug(p.Name) + ".json" };
        if (dlg.ShowDialog() == true) ProfileService.Export(p, dlg.FileName);
    }

    private void Import_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog { Filter = "JSON (*.json)|*.json" };
        if (dlg.ShowDialog() != true) return;
        try
        {
            var p = ProfileService.Read(dlg.FileName);
            if (p == null || string.IsNullOrWhiteSpace(p.Name))
            {
                MessageBox.Show("That file is not a valid profile.", "Universal FrameFX");
                return;
            }
            ProfileService.Save(p);
            Refresh();
            ProfileList.SelectedItem = p.Name;
        }
        catch
        {
            MessageBox.Show("That file is not a valid profile.", "Universal FrameFX");
        }
    }

    private void Backup_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new OpenFolderDialog { Title = "Select the game folder to back up" };
        if (dlg.ShowDialog() != true) return;
        var dir = dlg.FolderName;

        var marker = AntiCheatScanner.Detect(dir);
        if (marker != null)
        {
            MessageBox.Show(
                $"Refused: anti-cheat detected ({marker}).\nNever modify online-game folders.",
                "Universal FrameFX", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        var meta = BackupService.Backup(NameBox.Text.Trim(), dir);
        MessageBox.Show(
            $"Backed up {meta.Files.Count} config file(s) to snapshot {meta.Id}.",
            "Universal FrameFX");
    }

    private void Restore_Click(object sender, RoutedEventArgs e)
    {
        var id = BackupService.LatestSnapshotId();
        if (id == null)
        {
            MessageBox.Show("No backups yet.", "Universal FrameFX");
            return;
        }
        BackupService.Restore(id);
        MessageBox.Show($"Restored snapshot {id}.", "Universal FrameFX");
    }
}
