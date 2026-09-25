using System.IO;
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
        UpscalerBox.ItemsSource = ProfileUpscalers.All;
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

        // Fall back to the folder name rather than writing an empty GameName,
        // which made snapshots unidentifiable in the restore prompt.
        var gameName = NameBox.Text.Trim();
        if (string.IsNullOrEmpty(gameName))
            gameName = new DirectoryInfo(dir).Name;

        RunBackup(gameName, dir);
    }

    /// <summary>
    /// Walks the game folder off the UI thread. A large install has tens of
    /// thousands of files, and doing this inline froze the window.
    /// </summary>
    private async void RunBackup(string gameName, string dir)
    {
        IsEnabled = false;
        try
        {
            var meta = await Task.Run(() => BackupService.Backup(gameName, dir));
            MessageBox.Show(
                meta.Files.Count == 0
                    ? $"No config files (.ini/.cfg/.json/.xml) found in:\n{dir}\n\n"
                      + "Nothing was backed up."
                    : $"Backed up {meta.Files.Count} config file(s) to snapshot {meta.Id}.",
                "Universal FrameFX");
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException
                                  or PathTooLongException)
        {
            MessageBox.Show($"Backup failed: {ex.Message}", "Universal FrameFX",
                            MessageBoxButton.OK, MessageBoxImage.Error);
        }
        finally
        {
            IsEnabled = true;
        }
    }

    private void Restore_Click(object sender, RoutedEventArgs e)
    {
        // This used to restore BackupService.LatestSnapshotId() with no
        // confirmation, which writes into whichever GameDir the newest snapshot
        // recorded — potentially a different game than the one on screen. It
        // must name the target and be confirmed.
        var snapshots = BackupService.ListSnapshots();
        if (snapshots.Count == 0)
        {
            MessageBox.Show("No backups yet.", "Universal FrameFX");
            return;
        }

        // Prefer a snapshot for the profile currently being edited; otherwise
        // fall back to the newest, but say so plainly in the prompt.
        var name = NameBox.Text.Trim();
        var match = snapshots.FirstOrDefault(sn =>
            !string.IsNullOrEmpty(name)
            && string.Equals(sn.GameName, name, StringComparison.OrdinalIgnoreCase));
        var chosen = match ?? snapshots[0];

        var lead = match is not null
            ? $"Restore the most recent backup for \"{chosen.GameName}\"?"
            : $"No backup matches \"{(string.IsNullOrEmpty(name) ? "(no profile name)" : name)}\".\n"
              + "The most recent backup of any game will be restored instead.";

        var confirm = MessageBox.Show(
            $"{lead}\n\n"
            + $"Snapshot:   {chosen.Id}\n"
            + $"Game:       {(string.IsNullOrWhiteSpace(chosen.GameName) ? "(unnamed)" : chosen.GameName)}\n"
            + $"Taken:      {(chosen.TakenUtc == DateTime.MinValue ? "unknown" : chosen.TakenUtc.ToLocalTime().ToString("yyyy-MM-dd HH:mm"))}\n"
            + $"Files:      {chosen.Files.Count}\n\n"
            + $"These files will be OVERWRITTEN in:\n{chosen.GameDir}\n\n"
            + "This cannot be undone.",
            "Confirm restore", MessageBoxButton.OKCancel, MessageBoxImage.Warning,
            MessageBoxResult.Cancel);

        if (confirm != MessageBoxResult.OK) return;

        try
        {
            BackupService.Restore(chosen.Id);
            MessageBox.Show(
                $"Restored {chosen.Files.Count} file(s) from snapshot {chosen.Id}.",
                "Universal FrameFX");
        }
        catch (Exception ex) when (ex is IOException or InvalidDataException
                                  or FileNotFoundException or UnauthorizedAccessException)
        {
            MessageBox.Show($"Restore failed: {ex.Message}", "Universal FrameFX",
                            MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }
}
