using System.Windows;
using System.Windows.Threading;
using UniversalFrameFX.Services;

namespace UniversalFrameFX;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        // Detect hardware and platform once, before the first window renders.
        AppState.Initialize();
        DispatcherUnhandledException += OnUnhandledException;
        base.OnStartup(e);
    }

    private void OnUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
    {
        MessageBox.Show(
            "Universal FrameFX hit an unexpected error and will keep running:\n\n" + e.Exception.Message,
            "Universal FrameFX",
            MessageBoxButton.OK,
            MessageBoxImage.Warning);
        e.Handled = true;
    }
}
