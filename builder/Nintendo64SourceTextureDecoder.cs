using System.Drawing;

namespace helengine.n64.builder;

/// <summary>
/// Decodes one authored source image into an uncooked straight-RGBA texture asset for the N64 cook.
/// </summary>
public sealed class Nintendo64SourceTextureDecoder {
    /// <summary>
    /// Decodes one source image file into a raw RGBA texture asset.
    /// </summary>
    /// <param name="sourceAssetPath">Absolute source image path.</param>
    /// <returns>Raw texture asset containing straight RGBA pixel bytes.</returns>
    public TextureAsset Decode(string sourceAssetPath) {
        if (string.IsNullOrWhiteSpace(sourceAssetPath)) {
            throw new ArgumentException("Source asset path is required.", nameof(sourceAssetPath));
        }
        if (!File.Exists(sourceAssetPath)) {
            throw new FileNotFoundException("Source texture file was not found.", sourceAssetPath);
        }

        using Bitmap bitmap = new(sourceAssetPath);
        if (bitmap.Width < 1 || bitmap.Height < 1) {
            throw new InvalidOperationException("Source textures must have positive dimensions.");
        }

        byte[] rgbaBytes = new byte[bitmap.Width * bitmap.Height * 4];
        int writeOffset = 0;
        for (int y = 0; y < bitmap.Height; y++) {
            for (int x = 0; x < bitmap.Width; x++) {
                Color pixel = bitmap.GetPixel(x, y);
                rgbaBytes[writeOffset++] = pixel.R;
                rgbaBytes[writeOffset++] = pixel.G;
                rgbaBytes[writeOffset++] = pixel.B;
                rgbaBytes[writeOffset++] = pixel.A;
            }
        }

        return new TextureAsset {
            Width = checked((ushort)bitmap.Width),
            Height = checked((ushort)bitmap.Height),
            Colors = rgbaBytes,
            PaletteColors = Array.Empty<byte>(),
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = TextureAssetAlphaPrecision.A8
        };
    }
}
