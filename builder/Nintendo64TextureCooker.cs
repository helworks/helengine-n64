using helengine.editor;

namespace helengine.n64.builder;

/// <summary>
/// Downscales decoded straight-RGBA texture assets into the cooked N64 runtime form using a box filter.
/// </summary>
public sealed class Nintendo64TextureCooker {
    /// <summary>
    /// Cooks one decoded RGBA texture into the platform's downscaled runtime form. A source at or under the
    /// configured maximum resolution is never upscaled; a larger source is box-filtered down to it.
    /// </summary>
    /// <param name="sourceTexture">Decoded straight-RGBA texture asset to cook.</param>
    /// <param name="settings">Resolved N64 texture cook settings.</param>
    /// <returns>Cooked texture asset sized to the configured maximum resolution.</returns>
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
        int expectedSourceByteLength = sourceTexture.Width * sourceTexture.Height * 4;
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

        return new TextureAsset {
            Width = checked((ushort)targetWidth),
            Height = checked((ushort)targetHeight),
            Colors = cookedColors,
            PaletteColors = Array.Empty<byte>(),
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = settings.AlphaPrecision
        };
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
