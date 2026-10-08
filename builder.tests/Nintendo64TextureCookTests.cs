using System.Drawing;
using System.Drawing.Imaging;
using helengine.baseplatform.Manifest;
using helengine.editor;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

/// <summary>
/// Verifies the N64 builder-owned texture cook decodes, downscales, and writes cooked runtime textures.
/// </summary>
public sealed class Nintendo64TextureCookTests {
    /// <summary>
    /// Keeps every authored 32 pixel glyph texel and the atlas dimensions through the N64 font capability,
    /// while ensuring the separate generic texture policy still reduces the same source to 64x32.
    /// </summary>
    [Fact]
    public void Cook_PreservesAuthoredBodyAtlasUsingFontCapabilityOnly() {
        helengine.baseplatform.Definitions.PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();
        byte[] colors = new byte[512 * 256 * 4];
        colors[0] = 73;
        colors[3] = 255;
        colors[^1] = 219;
        TextureAsset source = new TextureAsset {
            Width = 512,
            Height = 256,
            Colors = colors,
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = TextureAssetAlphaPrecision.A8
        };
        TextureAssetProcessorSettings fontSettings = Nintendo64TextureCookSettingsSerializer.Deserialize(
            definition.AssetCookCapabilities.Single(capability => capability.SourceAssetKind == "font-atlas-texture").DefaultSerializedPlatformSettings);
        TextureAssetProcessorSettings textureSettings = Nintendo64TextureCookSettingsSerializer.Deserialize(
            definition.AssetCookCapabilities.Single(capability => capability.SourceAssetKind == "texture").DefaultSerializedPlatformSettings);

        TextureAsset fontAtlas = new Nintendo64TextureCooker().Cook(source, fontSettings);
        TextureAsset ordinaryTexture = new Nintendo64TextureCooker().Cook(source, textureSettings);

        Assert.Equal(512, fontAtlas.Width);
        Assert.Equal(256, fontAtlas.Height);
        Assert.Equal(colors, fontAtlas.Colors);
        Assert.Equal(64, ordinaryTexture.Width);
        Assert.Equal(32, ordinaryTexture.Height);
    }

    /// <summary>
    /// Ensures a large source image is downscaled to the configured maximum resolution while preserving alpha.
    /// </summary>
    [Fact]
    public void Cook_DownscalesLargeSourceAndPreservesAlpha() {
        string sourcePath = WriteSourcePng(256, 256);
        try {
            TextureAsset decoded = new Nintendo64SourceTextureDecoder().Decode(sourcePath);
            Assert.Equal(256, decoded.Width);
            Assert.Equal(TextureAssetColorFormat.Rgba32, decoded.ColorFormat);

            TextureAsset cooked = new Nintendo64TextureCooker().Cook(decoded, new TextureAssetProcessorSettings {
                MaxResolution = 64,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            });

            Assert.Equal(64, cooked.Width);
            Assert.Equal(64, cooked.Height);
            Assert.Equal(64 * 64 * 4, cooked.Colors.Length);
            Assert.Equal((byte)0, cooked.Colors[3]);
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures a source smaller than the maximum resolution is never upscaled.
    /// </summary>
    [Fact]
    public void Cook_DoesNotUpscaleSmallSource() {
        string sourcePath = WriteSourcePng(16, 16);
        try {
            TextureAsset cooked = new Nintendo64TextureCooker().Cook(
                new Nintendo64SourceTextureDecoder().Decode(sourcePath),
                new TextureAssetProcessorSettings { MaxResolution = 64, ColorFormat = TextureAssetColorFormat.Rgba32, AlphaPrecision = TextureAssetAlphaPrecision.A8 });

            Assert.Equal(16, cooked.Width);
            Assert.Equal(16, cooked.Height);
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures the executor writes the cooked payload under the package source root at the work item's output path.
    /// </summary>
    [Fact]
    public void Execute_WritesCookedTextureUnderPackageSourceRoot() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\cube-test\\tests", Guid.NewGuid().ToString("N"));
        string sourcePath = WriteSourcePng(32, 32);
        try {
            Directory.CreateDirectory(root);
            PlatformCookWorkItem workItem = CreateWorkItem(sourcePath, "texture", "cooked/textures/icon.hasset");

            new Nintendo64PlatformCookWorkItemExecutor().Execute(workItem, root);

            string cookedPath = Path.Combine(root, "cooked", "textures", "icon.hasset");
            Assert.True(File.Exists(cookedPath));
            helengine.TextureAsset cooked = Assert.IsType<helengine.TextureAsset>(
                helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(cookedPath)));
            Assert.Equal(32, cooked.Width);
            Assert.Equal("icon", cooked.Id);
        } finally {
            File.Delete(sourcePath);
            if (Directory.Exists(root)) {
                Directory.Delete(root, true);
            }
        }
    }

    /// <summary>
    /// Ensures an unsupported source asset kind fails loudly instead of being silently skipped.
    /// </summary>
    [Fact]
    public void Execute_RejectsUnsupportedSourceAssetKind() {
        string sourcePath = WriteSourcePng(8, 8);
        try {
            PlatformCookWorkItem workItem = CreateWorkItem(sourcePath, "audio", "cooked/audio/clip.hasset");
            Assert.Throws<InvalidOperationException>(() => new Nintendo64PlatformCookWorkItemExecutor().Execute(workItem, Path.GetTempPath()));
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures the executor accepts the font-atlas-texture source kind and cooks it with whichever settings the
    /// work item carries, rather than only accepting the literal string while silently falling back to the 64
    /// pixel texture cap. The work item here carries the resolved font-atlas-texture capability's 256 pixel
    /// settings (the same settings <see cref="Nintendo64PlatformDefinitionFactory.Create"/> publishes for that
    /// capability), and the source is wider than the 64 pixel texture cap, so a cooked width of 256 proves both
    /// that the kind was accepted and that the right settings were carried through the routing, not just that
    /// nothing threw.
    /// </summary>
    [Fact]
    public void Execute_WritesCookedFontAtlasUnderPackageSourceRootUsingFontSettings() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\cube-test\\tests", Guid.NewGuid().ToString("N"));
        string sourcePath = WriteSourcePng(256, 128);
        try {
            Directory.CreateDirectory(root);
            string fontSettings = Nintendo64TextureCookSettingsSerializer.Serialize(new TextureAssetProcessorSettings {
                MaxResolution = 256,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            });
            PlatformCookWorkItem workItem = CreateWorkItem(sourcePath, "font-atlas-texture", "cooked/fonts/atlas.hasset", fontSettings);

            new Nintendo64PlatformCookWorkItemExecutor().Execute(workItem, root);

            string cookedPath = Path.Combine(root, "cooked", "fonts", "atlas.hasset");
            Assert.True(File.Exists(cookedPath));
            helengine.TextureAsset cooked = Assert.IsType<helengine.TextureAsset>(
                helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(cookedPath)));
            // 256, not the 64 pixel texture cap: this is what would silently regress to 64 if the executor's
            // accepted-kinds literal were mistyped or dropped, since CreateWorkItem's settings still say 256 but
            // a rejected/misrouted kind would either throw (caught by this test) or never reach this assertion.
            Assert.Equal(256, cooked.Width);
            Assert.Equal("icon", cooked.Id);
        } finally {
            File.Delete(sourcePath);
            if (Directory.Exists(root)) {
                Directory.Delete(root, true);
            }
        }
    }

    /// <summary>
    /// Ensures a non-positive maximum resolution fails loudly instead of silently skipping the downscale.
    /// </summary>
    [Fact]
    public void Cook_ThrowsWhenMaxResolutionIsNotPositive() {
        string sourcePath = WriteSourcePng(16, 16);
        try {
            TextureAsset decoded = new Nintendo64SourceTextureDecoder().Decode(sourcePath);
            Assert.Throws<InvalidOperationException>(() => new Nintendo64TextureCooker().Cook(decoded, new TextureAssetProcessorSettings {
                MaxResolution = 0,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            }));
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures a cook settings color format the cook cannot emit fails loudly. The cook always writes Rgba32,
    /// so accepting a different requested format would silently hand back something other than what was asked
    /// for, and the settings parameter would be decorative.
    /// </summary>
    [Fact]
    public void Cook_ThrowsWhenSettingsRequestAColorFormatItCannotEmit() {
        string sourcePath = WriteSourcePng(16, 16);
        try {
            TextureAsset decoded = new Nintendo64SourceTextureDecoder().Decode(sourcePath);
            InvalidOperationException failure = Assert.Throws<InvalidOperationException>(() => new Nintendo64TextureCooker().Cook(decoded, new TextureAssetProcessorSettings {
                MaxResolution = 64,
                ColorFormat = TextureAssetColorFormat.Rgba4444,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            }));
            Assert.Contains("Rgba4444", failure.Message, StringComparison.Ordinal);
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures box filtering averages real pixel values and preserves aspect ratio on a non-square source.
    /// </summary>
    [Fact]
    public void Cook_AveragesPixelsAndPreservesAspectRatio() {
        // Four columns tinted 10/20/30/40 red over two rows collapse to two columns averaging 15 and 35.
        string sourcePath = WriteColumnTintedSourcePng(4, 2);
        try {
            TextureAsset cooked = new Nintendo64TextureCooker().Cook(
                new Nintendo64SourceTextureDecoder().Decode(sourcePath),
                new TextureAssetProcessorSettings { MaxResolution = 2, ColorFormat = TextureAssetColorFormat.Rgba32, AlphaPrecision = TextureAssetAlphaPrecision.A8 });

            Assert.Equal(2, cooked.Width);
            Assert.Equal(1, cooked.Height);
            Assert.Equal(2 * 1 * 4, cooked.Colors.Length);
            Assert.Equal((byte)15, cooked.Colors[0]);
            Assert.Equal((byte)100, cooked.Colors[1]);
            Assert.Equal((byte)200, cooked.Colors[2]);
            Assert.Equal((byte)255, cooked.Colors[3]);
            Assert.Equal((byte)35, cooked.Colors[4]);
            Assert.Equal((byte)100, cooked.Colors[5]);
            Assert.Equal((byte)200, cooked.Colors[6]);
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures an opaque alpha precision forces every cooked texel to full alpha.
    /// </summary>
    [Fact]
    public void Cook_ForcesFullAlphaForOpaquePrecision() {
        string sourcePath = WriteSourcePng(8, 8);
        try {
            TextureAsset cooked = new Nintendo64TextureCooker().Cook(
                new Nintendo64SourceTextureDecoder().Decode(sourcePath),
                new TextureAssetProcessorSettings { MaxResolution = 4, ColorFormat = TextureAssetColorFormat.Rgba32, AlphaPrecision = TextureAssetAlphaPrecision.Opaque });

            for (int alphaOffset = 3; alphaOffset < cooked.Colors.Length; alphaOffset += 4) {
                Assert.Equal((byte)255, cooked.Colors[alphaOffset]);
            }
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures a serialized engine texture asset source is decoded through the asset serializer instead of the
    /// system image decoder, whichever extension the editor gave it: the cached platform font atlas arrives as
    /// <c>.hetex</c> and the externalized editor default font atlas as <c>.hasset</c>.
    /// </summary>
    /// <param name="sourceExtension">Extension the editor gave the generated source asset.</param>
    [Theory]
    [InlineData(".hetex")]
    [InlineData(".hasset")]
    public void Decode_ReadsGeneratedEngineTextureAssetSource(string sourceExtension) {
        string sourcePath = WriteSourceEngineTexture(128, 96, sourceExtension);
        try {
            TextureAsset decoded = new Nintendo64SourceTextureDecoder().Decode(sourcePath);

            Assert.Equal(128, decoded.Width);
            Assert.Equal(96, decoded.Height);
            Assert.Equal(TextureAssetColorFormat.Rgba32, decoded.ColorFormat);
            Assert.Equal(128 * 96 * 4, decoded.Colors.Length);

            TextureAsset cooked = new Nintendo64TextureCooker().Cook(decoded, new TextureAssetProcessorSettings {
                MaxResolution = 64,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            });

            Assert.Equal(64, cooked.Width);
            Assert.Equal(48, cooked.Height);
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Ensures the font-atlas-texture settings (256 max resolution) and the plain texture settings (64 max
    /// resolution) produce visibly different cook results from the identical source atlas, which is the entire
    /// point of publishing a second capability: without it, every font atlas was crushed to the 64 pixel cap.
    /// </summary>
    [Fact]
    public void Cook_FontSettingsLeaveA256WideAtlasUnscaledWhileTextureSettingsCrushIt() {
        // The brief's fixture code called a `CreateColumnTintedTexture(width, height)` helper that returns a
        // TextureAsset directly. This file's existing fixtures instead write a source PNG and decode it through
        // Nintendo64SourceTextureDecoder, matching every other cook test here. WriteColumnTintedSourcePng's per-
        // column tint formula (x + 1) * 10 only stays within the 0-255 Color.FromArgb range up to width 25, so a
        // 256-wide atlas reuses WriteSourcePng instead: this test only asserts cooked width/height, which do not
        // depend on source pixel content.
        string sourcePath = WriteSourcePng(256, 128);
        try {
            TextureAsset atlas = new Nintendo64SourceTextureDecoder().Decode(sourcePath);

            TextureAsset cookedAsFont = new Nintendo64TextureCooker().Cook(atlas, new TextureAssetProcessorSettings {
                MaxResolution = 256,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            });

            TextureAsset cookedAsTexture = new Nintendo64TextureCooker().Cook(atlas, new TextureAssetProcessorSettings {
                MaxResolution = 64,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            });

            Assert.Equal(256, cookedAsFont.Width);
            Assert.Equal(128, cookedAsFont.Height);
            Assert.Equal(64, cookedAsTexture.Width);
            Assert.Equal(32, cookedAsTexture.Height);
        } finally {
            File.Delete(sourcePath);
        }
    }

    /// <summary>
    /// Writes one serialized engine texture asset mirroring the editor's generated platform font atlas.
    /// </summary>
    /// <param name="width">Source width in pixels.</param>
    /// <param name="height">Source height in pixels.</param>
    /// <param name="sourceExtension">Extension the editor gave the generated source asset.</param>
    /// <returns>Absolute path of the written engine texture asset.</returns>
    static string WriteSourceEngineTexture(int width, int height, string sourceExtension) {
        string path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + sourceExtension);
        byte[] colors = new byte[width * height * 4];
        for (int offset = 0; offset < colors.Length; offset += 4) {
            colors[offset] = 40;
            colors[offset + 1] = 80;
            colors[offset + 2] = 120;
            colors[offset + 3] = 255;
        }

        File.WriteAllBytes(path, helengine.files.AssetSerializer.SerializeToBytes(new TextureAsset {
            Id = "font-atlas",
            Width = checked((ushort)width),
            Height = checked((ushort)height),
            Colors = colors,
            PaletteColors = Array.Empty<byte>(),
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = TextureAssetAlphaPrecision.A8
        }));
        return path;
    }

    /// <summary>
    /// Writes one transparent-cornered test PNG and returns its path.
    /// </summary>
    /// <param name="width">Source width in pixels.</param>
    /// <param name="height">Source height in pixels.</param>
    /// <returns>Absolute path of the written PNG.</returns>
    static string WriteSourcePng(int width, int height) {
        string path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + ".png");
        using Bitmap bitmap = new(width, height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bitmap.SetPixel(x, y, Color.FromArgb(0, 10, 20, 30));
            }
        }
        bitmap.Save(path, ImageFormat.Png);
        return path;
    }

    /// <summary>
    /// Writes one test PNG whose columns carry distinct red values so averaging is observable.
    /// </summary>
    /// <param name="width">Source width in pixels.</param>
    /// <param name="height">Source height in pixels.</param>
    /// <returns>Absolute path of the written PNG.</returns>
    static string WriteColumnTintedSourcePng(int width, int height) {
        string path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + ".png");
        using Bitmap bitmap = new(width, height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bitmap.SetPixel(x, y, Color.FromArgb(255, (x + 1) * 10, 100, 200));
            }
        }
        bitmap.Save(path, ImageFormat.Png);
        return path;
    }

    /// <summary>
    /// Creates one platform cook work item for the supplied source path, kind, and output path, carrying the
    /// default 64 pixel N64 texture cook settings.
    /// </summary>
    /// <param name="sourcePath">Absolute source asset path.</param>
    /// <param name="sourceAssetKind">Source asset kind carried by the work item.</param>
    /// <param name="outputRelativePath">Runtime-relative cooked output path.</param>
    /// <returns>Platform cook work item.</returns>
    static PlatformCookWorkItem CreateWorkItem(string sourcePath, string sourceAssetKind, string outputRelativePath) {
        return CreateWorkItem(
            sourcePath,
            sourceAssetKind,
            outputRelativePath,
            Nintendo64TextureCookSettingsSerializer.Serialize(new TextureAssetProcessorSettings {
                MaxResolution = 64,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            }));
    }

    /// <summary>
    /// Creates one platform cook work item for the supplied source path, kind, output path, and already-serialized
    /// platform settings. This overload lets a test carry settings other than the default 64 pixel texture cap -
    /// for example the resolved font-atlas-texture capability's settings - through to the executor, mirroring how
    /// the editor-owned build graph resolves settings from the matching capability before the work item ever
    /// reaches <see cref="Nintendo64PlatformCookWorkItemExecutor"/>.
    /// </summary>
    /// <param name="sourcePath">Absolute source asset path.</param>
    /// <param name="sourceAssetKind">Source asset kind carried by the work item.</param>
    /// <param name="outputRelativePath">Runtime-relative cooked output path.</param>
    /// <param name="serializedPlatformSettings">Serialized resolved platform cook settings to carry on the work item.</param>
    /// <returns>Platform cook work item.</returns>
    static PlatformCookWorkItem CreateWorkItem(string sourcePath, string sourceAssetKind, string outputRelativePath, string serializedPlatformSettings) {
        return new PlatformCookWorkItem(
            "work-1",
            sourcePath,
            sourceAssetKind,
            "n64",
            "runtime-texture",
            outputRelativePath,
            "icon",
            "source-hash",
            "settings-hash",
            serializedPlatformSettings,
            []);
    }
}
