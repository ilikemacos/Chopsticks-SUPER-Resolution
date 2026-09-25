using System.ComponentModel;
using System.Runtime.Versioning;
using System.Windows.Forms;
using Csr.Capture;
using Vortice.Direct3D11;

namespace Csr.LiveUpscale;

// The window CSR's upscaled frames are presented into. WinForms owns the HWND,
// the message loop and lifecycle; CsrPresenter owns the swapchain.
//
// Threading: WGC delivers frames on a free-threaded pool thread, and the compute
// dispatch and the present both run on that thread. The form is only touched from
// the UI thread except for size sync, which is marshalled with BeginInvoke. The
// device's immediate context is therefore only ever used from the capture thread.
[SupportedOSPlatform("windows10.0.17763.0")]
internal sealed class PresentForm : Form
{
    private readonly ID3D11Device _device;
    private readonly string? _windowMatch;
    private readonly float _ratio;

    private CsrPresenter? _presenter;
    private CsrCaptureSession? _session;

    public PresentForm(ID3D11Device device, string? windowMatch, float ratio)
    {
        _device = device;
        _windowMatch = windowMatch;
        _ratio = ratio;

        Text = "CSR live upscale";
        // Fixed client area: the swapchain is kept 1:1 with CSR's output, so the
        // OS never stretches (and blurs) the result. It resizes to follow the
        // source window instead.
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(1280, 720);
        BackColor = Color.Black;
        DoubleBuffered = false;   // we present via the swapchain, not GDI
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        _presenter = new CsrPresenter(_device, Handle, ClientSize.Width, ClientSize.Height);
        StartCapture();
    }

    private async void StartCapture()
    {
        try
        {
            if (_windowMatch is not null)
            {
                IntPtr hwnd = Program.FindWindowByTitle(_windowMatch);
                if (hwnd == IntPtr.Zero)
                {
                    MessageBox.Show($"No visible window title contains \"{_windowMatch}\".",
                        "CSR live upscale", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    Close();
                    return;
                }
                _session = CsrCaptureSession.StartForWindow(_device, hwnd, _ratio);
            }
            else
            {
                _session = await CsrCaptureSession.StartWithPickerAsync(_device, _ratio);
            }
            _session.FrameUpscaled += OnFrameUpscaled;
        }
        catch (OperationCanceledException)
        {
            Close();   // the user dismissed the picker
        }
        catch (Exception ex)
        {
            MessageBox.Show(ex.Message, "CSR live upscale",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            Close();
        }
    }

    private void OnFrameUpscaled(ID3D11Texture2D upscaled)
    {
        // Runs on the WGC pool thread, same thread as the compute dispatch, so the
        // present shares that single-threaded use of the immediate context.
        var presenter = _presenter;
        if (presenter is null || IsDisposed) return;

        var (w, h) = presenter.Present(upscaled);

        // Keep the window's client area matched to the presented size. This is the
        // only cross-thread touch of the form, so it is marshalled.
        if ((w != ClientSize.Width || h != ClientSize.Height) && IsHandleCreated)
        {
            try { BeginInvoke(() => { if (!IsDisposed) ClientSize = new Size(w, h); }); }
            catch (InvalidOperationException) { /* handle went away during shutdown */ }
        }
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        if (_session is not null) _session.FrameUpscaled -= OnFrameUpscaled;
        _session?.Dispose();
        _presenter?.Dispose();
        base.OnFormClosing(e);
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _session?.Dispose();
            _presenter?.Dispose();
        }
        base.Dispose(disposing);
    }
}
