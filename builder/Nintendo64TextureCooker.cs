using helengine.editor;

namespace helengine.n64.builder;

/// <summary>
/// Box-filters straight-RGBA sources before producing direct RGBA or generic palette-backed N64 textures.
/// </summary>
public sealed class Nintendo64TextureCooker {
    /// <summary>
    /// Cooks one decoded RGBA texture into the platform's downscaled runtime form. A source at or under the
    /// configured maximum resolution is never upscaled; a larger source is box-filtered down to it.
    /// </summary>
    /// <param name="sourceTexture">Decoded straight-RGBA texture asset to cook.</param>
    /// <param name="settings">Resolved N64 texture cook settings.</param>
    /// <returns>Independent cooked texture with preserved identity, requested storage and the configured size cap.</returns>
    public TextureAsset Cook(TextureAsset sourceTexture, TextureAssetProcessorSettings settings) {
        if (sourceTexture == null) {
            throw new ArgumentNullException(nameof(sourceTexture));
        }
        if (settings == null) {
            throw new ArgumentNullException(nameof(settings));
        }
        if (sourceTexture.ColorFormat != TextureAssetColorFormat.Rgba32) {
            throw new InvalidOperationException($"N64 texture cook requires an Rgba32 source texture; received '{sourceTexture.ColorFormat}'.");
        }
        Nintendo64TextureCookSettingsSerializer.Validate(settings);
        if (sourceTexture.Width < 1 || sourceTexture.Height < 1) {
            throw new InvalidOperationException("N64 source textures must have positive dimensions.");
        }
        // Width and Height are 16-bit, so the product of the two and four bytes per pixel leaves the 32-bit
        // range above roughly 23170 pixels per side and would wrap into a bogus expected length. Widen the
        // arithmetic so an oversized source fails the length check rather than passing a wrapped one.
        long expectedSourceByteLength = (long)sourceTexture.Width * sourceTexture.Height * 4L;
        if (sourceTexture.Colors == null || sourceTexture.Colors.Length != expectedSourceByteLength) {
            throw new InvalidOperationException("N64 texture cook requires source pixel bytes whose length matches Width * Height * 4.");
        }
        if (settings.MaxResolution <= 0) {
            throw new InvalidOperationException($"N64 texture cook requires a positive maximum resolution; received '{settings.MaxResolution}'.");
        }

        int maxSide = settings.MaxResolution;
        int targetWidth = sourceTexture.Width;
        int targetHeight = sourceTexture.Height;
        if (sourceTexture.Width > maxSide || sourceTexture.Height > maxSide) {
            double factor = maxSide / (double)Math.Max(sourceTexture.Width, sourceTexture.Height);
            targetWidth = Math.Max(1, (int)Math.Round(sourceTexture.Width * factor));
            targetHeight = Math.Max(1, (int)Math.Round(sourceTexture.Height * factor));
        }

        byte[] cookedColors = BoxFilter(sourceTexture, targetWidth, targetHeight);
        if (settings.AlphaPrecision == TextureAssetAlphaPrecision.Opaque) {
            ForceOpaqueAlpha(cookedColors);
        }

        TextureAsset resizedTexture = new TextureAsset {
            Id = sourceTexture.Id,
            RuntimeAssetId = sourceTexture.RuntimeAssetId,
            AuthoringAssetId = sourceTexture.AuthoringAssetId,
            FormerAuthoringAssetIds = sourceTexture.FormerAuthoringAssetIds == null ? null : (string[])sourceTexture.FormerAuthoringAssetIds.Clone(),
            IsEngineOwned = sourceTexture.IsEngineOwned,
            Width = checked((ushort)targetWidth),
            Height = checked((ushort)targetHeight),
            Colors = cookedColors,
            PaletteColors = Array.Empty<byte>(),
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = settings.AlphaPrecision
        };
        if (settings.ColorFormat == TextureAssetColorFormat.Rgba32) {
            return resizedTexture;
        }

        // Quantization follows the existing box filter, so changing storage format cannot change resize sampling.
        resizedTexture.AlphaPrecision = TextureAssetAlphaPrecision.A8;
        using (resizedTexture) {
            TextureAsset indexedTexture = new TextureAssetProcessor().Apply(resizedTexture, new TextureAssetProcessorSettings {
                MaxResolution = 0,
                ColorFormat = settings.ColorFormat,
                AlphaPrecision = settings.AlphaPrecision,
                IndexingMethodId = settings.IndexingMethodId
            });
            indexedTexture.AuthoringAssetId = resizedTexture.AuthoringAssetId;
            indexedTexture.FormerAuthoringAssetIds = resizedTexture.FormerAuthoringAssetIds == null ? null : (string[])resizedTexture.FormerAuthoringAssetIds.Clone();
            indexedTexture.IsEngineOwned = resizedTexture.IsEngineOwned;
            return indexedTexture;
        }
    }

    /// <summary>
    /// Box-filters the source pixel bytes down to the target resolution, averaging each destination pixel's
    /// corresponding source rectangle.
    /// </summary>
    /// <param name="sourceTexture">Decoded source texture to sample from.</param>
    /// <param name="targetWidth">Target width in pixels.</param>
    /// <param name="targetHeight">Target height in pixels.</param>
    /// <returns>Filtered straight-RGBA pixel bytes sized to <c>targetWidth * targetHeight * 4</c>.</returns>
    byte[] BoxFilter(TextureAsset sourceTexture, int targetWidth, int targetHeight) {
        int sourceWidth = sourceTexture.Width;
        int sourceHeight = sourceTexture.Height;
        byte[] sourceColors = sourceTexture.Colors;
        byte[] targetColors = new byte[targetWidth * targetHeight * 4];

        for (int targetY = 0; targetY < targetHeight; targetY++) {
            int sourceRowStart = targetY * sourceHeight / targetHeight;
            int sourceRowEnd = Math.Max(sourceRowStart + 1, (targetY + 1) * sourceHeight / targetHeight);
            for (int targetX = 0; targetX < targetWidth; targetX++) {
                int sourceColumnStart = targetX * sourceWidth / targetWidth;
                int sourceColumnEnd = Math.Max(sourceColumnStart + 1, (targetX + 1) * sourceWidth / targetWidth);

                long redTotal = 0;
                long greenTotal = 0;
                long blueTotal = 0;
                long alphaTotal = 0;
                int sampleCount = 0;
                for (int sourceY = sourceRowStart; sourceY < sourceRowEnd; sourceY++) {
                    for (int sourceX = sourceColumnStart; sourceX < sourceColumnEnd; sourceX++) {
                        int sourceOffset = ((sourceY * sourceWidth) + sourceX) * 4;
                        redTotal += sourceColors[sourceOffset];
                        greenTotal += sourceColors[sourceOffset + 1];
                        blueTotal += sourceColors[sourceOffset + 2];
                        alphaTotal += sourceColors[sourceOffset + 3];
                        sampleCount++;
                    }
                }

                int targetOffset = ((targetY * targetWidth) + targetX) * 4;
                targetColors[targetOffset] = (byte)(redTotal / sampleCount);
                targetColors[targetOffset + 1] = (byte)(greenTotal / sampleCount);
                targetColors[targetOffset + 2] = (byte)(blueTotal / sampleCount);
                targetColors[targetOffset + 3] = (byte)(alphaTotal / sampleCount);
            }
        }

        return targetColors;
    }

    /// <summary>
    /// Overwrites every alpha byte in the supplied straight-RGBA pixel buffer with 255.
    /// </summary>
    /// <param name="colors">Straight-RGBA pixel bytes to update in place.</param>
    void ForceOpaqueAlpha(byte[] colors) {
        for (int alphaOffset = 3; alphaOffset < colors.Length; alphaOffset += 4) {
            colors[alphaOffset] = 255;
        }
    }
}
