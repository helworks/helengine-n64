using System.Drawing;

namespace helengine.n64.builder;

/// <summary>
/// Decodes one cook work item source into an uncooked straight-RGBA texture asset for the N64 cook. Work item
/// sources are either authored image files or engine texture assets that the editor generated for the platform,
/// such as the cached font atlases written next to each platform font variant.
/// </summary>
public sealed class Nintendo64SourceTextureDecoder {
    /// <summary>
    /// Extension used by serialized engine texture assets that the editor hands to builder-owned cooks.
    /// </summary>
    const string EngineTextureAssetExtension = ".hetex";

    /// <summary>
    /// Decodes one source texture into a raw RGBA texture asset.
    /// </summary>
    /// <param name="sourceAssetPath">Absolute source image or engine texture asset path.</param>
    /// <returns>Raw texture asset containing straight RGBA pixel bytes.</returns>
    public TextureAsset Decode(string sourceAssetPath) {
        if (string.IsNullOrWhiteSpace(sourceAssetPath)) {
            throw new ArgumentException("Source asset path is required.", nameof(sourceAssetPath));
        }
        if (!File.Exists(sourceAssetPath)) {
            throw new FileNotFoundException("Source texture file was not found.", sourceAssetPath);
        }

        if (string.Equals(Path.GetExtension(sourceAssetPath), EngineTextureAssetExtension, StringComparison.OrdinalIgnoreCase)) {
            return DecodeEngineTextureAsset(sourceAssetPath);
        }

        return DecodeImageFile(sourceAssetPath);
    }

    /// <summary>
    /// Decodes one authored image file through the system image decoder.
    /// </summary>
    /// <param name="sourceAssetPath">Absolute source image path.</param>
    /// <returns>Raw texture asset containing straight RGBA pixel bytes.</returns>
    TextureAsset DecodeImageFile(string sourceAssetPath) {
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

    /// <summary>
    /// Reads one serialized engine texture asset, which the editor already stored as straight RGBA pixel bytes.
    /// </summary>
    /// <param name="sourceAssetPath">Absolute engine texture asset path.</param>
    /// <returns>Raw texture asset containing straight RGBA pixel bytes.</returns>
    TextureAsset DecodeEngineTextureAsset(string sourceAssetPath) {
        Asset deserializedAsset = helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(sourceAssetPath));
        if (deserializedAsset is not TextureAsset sourceTexture) {
            throw new InvalidOperationException($"N64 texture cook source '{sourceAssetPath}' did not contain a texture asset.");
        }
        if (sourceTexture.ColorFormat != TextureAssetColorFormat.Rgba32) {
            throw new InvalidOperationException($"N64 texture cook source '{sourceAssetPath}' must be Rgba32; received '{sourceTexture.ColorFormat}'.");
        }
        if (sourceTexture.Width < 1 || sourceTexture.Height < 1) {
            throw new InvalidOperationException("Source textures must have positive dimensions.");
        }

        int expectedByteLength = sourceTexture.Width * sourceTexture.Height * 4;
        if (sourceTexture.Colors == null || sourceTexture.Colors.Length != expectedByteLength) {
            throw new InvalidOperationException($"N64 texture cook source '{sourceAssetPath}' pixel bytes must match Width * Height * 4.");
        }

        return new TextureAsset {
            Width = sourceTexture.Width,
            Height = sourceTexture.Height,
            Colors = sourceTexture.Colors,
            PaletteColors = Array.Empty<byte>(),
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = sourceTexture.AlphaPrecision
        };
    }
}
