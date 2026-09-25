namespace Csr.Core;

/// <summary>
/// Tightly packed float RGB image, row-major, three floats per pixel.
/// </summary>
/// <remarks>
/// Values are expected to be perceptually encoded (sRGB/gamma) in [0,1], not
/// linear light. CSR's edge detection assumes this; feeding it linear values
/// produces haloing that looks like an algorithm fault but is not one.
/// </remarks>
public sealed class ImageBuffer
{
    public int Width { get; }
    public int Height { get; }
    public float[] Data { get; }

    public ImageBuffer(int width, int height)
    {
        if (width <= 0 || height <= 0)
            throw new ArgumentOutOfRangeException(nameof(width), "dimensions must be positive");
        Width = width;
        Height = height;
        Data = new float[checked(width * height * 3)];
    }

    public ImageBuffer(int width, int height, float[] data) : this(width, height)
    {
        ArgumentNullException.ThrowIfNull(data);
        if (data.Length != Data.Length)
            throw new ArgumentException(
                $"expected {Data.Length} floats for {width}x{height}, got {data.Length}", nameof(data));
        Array.Copy(data, Data, data.Length);
    }

    /// <summary>Index of the first component of a pixel, with edge clamping.</summary>
    public int ClampedIndex(int x, int y)
    {
        if (x < 0) x = 0; else if (x >= Width) x = Width - 1;
        if (y < 0) y = 0; else if (y >= Height) y = Height - 1;
        return (y * Width + x) * 3;
    }
}
