using helengine.baseplatform.Manifest;
using helengine.editor;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

/// <summary>
/// Exercises indexed cooking and real work-item serialization without changing glyph metrics or caller pixels.
/// </summary>
public sealed class Nintendo64IndexedTextureCookTests {
    /// <summary>Checks the image default remains RGBA while fallback font atlases use full-coverage Indexed8.</summary>
    [Fact]
    public void CapabilitiesPublishIndexedFormatsAndFontDefault() {
        var capabilities = Nintendo64PlatformDefinitionFactory.Create().AssetCookCapabilities;
        foreach (var capability in capabilities) {
            bool font = capability.SourceAssetKind == "font-atlas-texture";
            TextureAssetProcessorSettings settings = Nintendo64TextureCookSettingsSerializer.Deserialize(capability.DefaultSerializedPlatformSettings);
            Assert.Equal(font ? 512 : 64, settings.MaxResolution);
            Assert.Equal(font ? TextureAssetColorFormat.Indexed8 : TextureAssetColorFormat.Rgba32, settings.ColorFormat);
            Assert.Equal(TextureAssetAlphaPrecision.A8, settings.AlphaPrecision);
            Assert.Equal(font ? "QuantizedIndexed" : string.Empty, settings.IndexingMethodId);
            Assert.Equal(new[] { "Rgba32", "Indexed4", "Indexed8" }, capability.TextureFormatCapabilities.SupportedColorFormatIds);
            Assert.Equal(10, capability.TextureFormatCapabilities.SupportedCombinations.Length);
        }
    }

    /// <summary>Verifies compact odd-row Indexed4 packing and Indexed8 row order against golden bytes.</summary>
    /// <param name="format">Index storage to cook.</param>
    [Theory]
    [InlineData(TextureAssetColorFormat.Indexed4)]
    [InlineData(TextureAssetColorFormat.Indexed8)]
    public void CookPreservesIdentityAndUsesGenericIndexPacking(TextureAssetColorFormat format) {
        using TextureAsset source = CreateTexture(3, 3);
        for (int i = 0; i < 9; i++) {
            source.Colors[i * 4] = (byte)(i + 1);
        }
        byte[] original = (byte[])source.Colors.Clone();
        using TextureAsset cooked = new Nintendo64TextureCooker().Cook(source, Settings(format, 64));
        Assert.Equal(format, cooked.ColorFormat);
        Assert.Equal(format == TextureAssetColorFormat.Indexed4 ? new byte[] { 0x10, 0x32, 0x54, 0x76, 0x08 } : new byte[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 }, cooked.Colors);
        Assert.Equal(format == TextureAssetColorFormat.Indexed4 ? 64 : 1024, cooked.PaletteColors.Length);
        Assert.Equal(original, TextureAssetPixelCodec.DecodeToRgba32(cooked));
        Assert.Equal(source.Id, cooked.Id);
        Assert.Equal(source.RuntimeAssetId, cooked.RuntimeAssetId);
        Assert.Equal(source.AuthoringAssetId, cooked.AuthoringAssetId);
        Assert.Equal(source.FormerAuthoringAssetIds, cooked.FormerAuthoringAssetIds);
        Assert.NotSame(source.FormerAuthoringAssetIds, cooked.FormerAuthoringAssetIds);
        Assert.True(cooked.IsEngineOwned);
        string root = Environment.GetEnvironmentVariable("HELENGINE_N64_INDEXED_TEST_ROOT")
            ?? throw new InvalidOperationException("The fixed N64 indexed test root must be configured.");
        Directory.CreateDirectory(root);
        string generatedPath = Path.Combine(root, format + "-odd-width.hetex");
        File.WriteAllBytes(generatedPath, helengine.files.AssetSerializer.SerializeToBytes(cooked));
        using (TextureAsset decoded = new Nintendo64SourceTextureDecoder().Decode(generatedPath)) {
            Assert.Equal(original, decoded.Colors);
            Assert.Equal(TextureAssetColorFormat.Rgba32, decoded.ColorFormat);
        }
        cooked.Dispose();
        Assert.Equal(original, source.Colors);
    }

    /// <summary>Confirms alpha policy is applied to the palette, including antialiased coverage levels.</summary>
    /// <param name="format">Palette-backed output format.</param>
    /// <param name="alpha">Selected palette alpha precision.</param>
    [Theory]
    [InlineData(TextureAssetColorFormat.Indexed4, TextureAssetAlphaPrecision.Opaque)]
    [InlineData(TextureAssetColorFormat.Indexed4, TextureAssetAlphaPrecision.Binary)]
    [InlineData(TextureAssetColorFormat.Indexed4, TextureAssetAlphaPrecision.A4)]
    [InlineData(TextureAssetColorFormat.Indexed4, TextureAssetAlphaPrecision.A8)]
    [InlineData(TextureAssetColorFormat.Indexed8, TextureAssetAlphaPrecision.Opaque)]
    [InlineData(TextureAssetColorFormat.Indexed8, TextureAssetAlphaPrecision.Binary)]
    [InlineData(TextureAssetColorFormat.Indexed8, TextureAssetAlphaPrecision.A4)]
    [InlineData(TextureAssetColorFormat.Indexed8, TextureAssetAlphaPrecision.A8)]
    public void PaletteHonorsAlphaPolicy(TextureAssetColorFormat format, TextureAssetAlphaPrecision alpha) {
        using TextureAsset source = CreateTexture(4, 1);
        byte[] coverage = [0, 85, 170, 255];
        for (int i = 0; i < 4; i++) {
            source.Colors[i * 4 + 3] = coverage[i];
        }
        TextureAssetProcessorSettings settings = Settings(format, 64);
        settings.AlphaPrecision = alpha;
        using TextureAsset cooked = new Nintendo64TextureCooker().Cook(source, settings);
        byte[] decoded = TextureAssetPixelCodec.DecodeToRgba32(cooked);
        byte[] expected = alpha == TextureAssetAlphaPrecision.Opaque ? [255, 255, 255, 255]
            : alpha == TextureAssetAlphaPrecision.Binary ? [0, 0, 255, 255] : coverage;
        Assert.Equal(expected, Enumerable.Range(0, 4).Select(i => decoded[i * 4 + 3]).ToArray());
        Assert.Equal(alpha, cooked.AlphaPrecision);
    }

    /// <summary>Preserves the historical box-filter golden samples before palette conversion.</summary>
    [Fact]
    public void IndexedResizeUsesExistingBoxFilter() {
        using TextureAsset source = CreateTexture(4, 2);
        for (int i = 0; i < 8; i++) {
            source.Colors[i * 4] = (byte)((i % 4 + 1) * 10);
        }
        using TextureAsset cooked = new Nintendo64TextureCooker().Cook(source, Settings(TextureAssetColorFormat.Indexed4, 2));
        Assert.Equal(2, cooked.Width);
        Assert.Equal(1, cooked.Height);
        Assert.Equal(new byte[] { 15, 255, 255, 255, 35, 255, 255, 255 }, TextureAssetPixelCodec.DecodeToRgba32(cooked));
    }

    /// <summary>Checks every 8-bit coverage level survives font-atlas work items at both menu font dimensions.</summary>
    /// <param name="width">Atlas width.</param>
    /// <param name="height">Atlas height.</param>
    /// <param name="editorIndexed">Whether the editor already applied the selected platform format to the generated source.</param>
    [Theory]
    [InlineData(512, 256, false)]
    [InlineData(256, 128, false)]
    [InlineData(512, 256, true)]
    [InlineData(256, 128, true)]
    public void FontAtlasWorkItemKeepsResolutionAndAllCoverageLevels(int width, int height, bool editorIndexed) {
        string root = Environment.GetEnvironmentVariable("HELENGINE_N64_INDEXED_TEST_ROOT")
            ?? throw new InvalidOperationException("The fixed N64 indexed test root must be configured.");
        string caseRoot = Path.Combine(root, "atlas-" + width + (editorIndexed ? "-editor-indexed" : "-rgba"));
        Directory.CreateDirectory(caseRoot);
        string sourcePath = Path.Combine(caseRoot, editorIndexed ? "DemoDiscBody.hetex" : "source.hasset");
        using TextureAsset source = CreateTexture((ushort)width, (ushort)height);
        for (int i = 0; i < source.Width * source.Height; i++) {
            source.Colors[i * 4 + 3] = (byte)(i % 256);
        }
        using TextureAsset generatedTexture = editorIndexed
            ? new Nintendo64TextureCooker().Cook(source, Settings(TextureAssetColorFormat.Indexed8, 512))
            : new Nintendo64TextureCooker().Cook(source, Settings(TextureAssetColorFormat.Rgba32, 512));
        byte[] originalIndices = (byte[])generatedTexture.Colors.Clone();
        byte[] originalPalette = (byte[])generatedTexture.PaletteColors.Clone();
        File.WriteAllBytes(sourcePath, helengine.files.AssetSerializer.SerializeToBytes(generatedTexture));
        using TextureAsset serializedSource = Assert.IsType<TextureAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(sourcePath)));
        using (TextureAsset normalized = new Nintendo64SourceTextureDecoder().Decode(sourcePath)) {
            Assert.Equal(TextureAssetColorFormat.Rgba32, normalized.ColorFormat);
            Assert.Equal(TextureAssetAlphaPrecision.A8, normalized.AlphaPrecision);
            Assert.Empty(normalized.PaletteColors);
            Assert.Equal(source.Colors, normalized.Colors);
            Assert.Equal(source.Id, normalized.Id);
            Assert.Equal(source.RuntimeAssetId, normalized.RuntimeAssetId);
            Assert.Equal(source.AuthoringAssetId, normalized.AuthoringAssetId);
            Assert.Equal(source.FormerAuthoringAssetIds, normalized.FormerAuthoringAssetIds);
            Assert.Equal(serializedSource.IsEngineOwned, normalized.IsEngineOwned);
        }
        Assert.Equal(originalIndices, generatedTexture.Colors);
        Assert.Equal(originalPalette, generatedTexture.PaletteColors);
        string payload = Nintendo64PlatformDefinitionFactory.Create().AssetCookCapabilities.Single(c => c.SourceAssetKind == "font-atlas-texture").DefaultSerializedPlatformSettings;
        var item = new PlatformCookWorkItem("atlas", sourcePath, "font-atlas-texture", "n64", "runtime-texture", "atlas.hetex", "menu-atlas", "source-hash", "settings-hash", payload, []);
        new Nintendo64PlatformCookWorkItemExecutor().Execute(item, caseRoot);
        using TextureAsset cooked = Assert.IsType<TextureAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(Path.Combine(caseRoot, "atlas.hetex"))));
        Assert.Equal("menu-atlas", cooked.Id);
        Assert.Equal(width, cooked.Width);
        Assert.Equal(height, cooked.Height);
        Assert.Equal(TextureAssetColorFormat.Indexed8, cooked.ColorFormat);
        Assert.Equal(source.Colors, TextureAssetPixelCodec.DecodeToRgba32(cooked));
        Assert.Equal("authoring-id", cooked.AuthoringAssetId);
        Assert.Equal("QuantizedIndexed", Nintendo64TextureCookSettingsSerializer.Deserialize(payload).IndexingMethodId);
    }

    /// <summary>Rejects unsupported methods instead of silently executing the default quantizer.</summary>
    /// <param name="format">Format used with the unsupported method.</param>
    /// <param name="method">Unsupported method identifier.</param>
    [Theory]
    [InlineData(TextureAssetColorFormat.Indexed4, "Exact")]
    [InlineData(TextureAssetColorFormat.Indexed8, "Unknown")]
    [InlineData(TextureAssetColorFormat.Rgba32, "QuantizedIndexed")]
    public void SettingsRejectUnsupportedIndexingMethods(TextureAssetColorFormat format, string method) {
        TextureAssetProcessorSettings settings = Settings(format, 64);
        settings.IndexingMethodId = method;
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Serialize(settings));
        using TextureAsset source = CreateTexture(1, 1);
        Assert.Throws<InvalidOperationException>(() => new Nintendo64TextureCooker().Cook(source, settings));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize($"{{\"maxResolution\":64,\"colorFormat\":\"{format}\",\"alphaPrecision\":\"A8\",\"indexingMethod\":\"{method}\"}}"));
    }

    /// <summary>Rejects malformed pixel lengths and zero dimensions before resize or quantization reads them.</summary>
    [Fact]
    public void CookRejectsTruncatedPixelsAndZeroDimensions() {
        using TextureAsset source = CreateTexture(2, 1);
        source.Colors = [1, 2, 3, 4];
        Assert.Throws<InvalidOperationException>(() => new Nintendo64TextureCooker().Cook(source, Settings(TextureAssetColorFormat.Indexed8, 64)));
        source.Width = 0;
        Assert.Throws<InvalidOperationException>(() => new Nintendo64TextureCooker().Cook(source, Settings(TextureAssetColorFormat.Indexed8, 64)));
    }

    /// <summary>Rejects corrupt serialized palettes, indices, texel lengths and unknown formats before cooking output.</summary>
    /// <param name="corruption">Malformed source payload to create.</param>
    [Theory]
    [InlineData("texels")]
    [InlineData("palette")]
    [InlineData("index")]
    [InlineData("format")]
    public void SourceDecoderRejectsMalformedSerializedIndexedTexture(string corruption) {
        string root = Environment.GetEnvironmentVariable("HELENGINE_N64_INDEXED_TEST_ROOT")
            ?? throw new InvalidOperationException("The fixed N64 indexed test root must be configured.");
        Directory.CreateDirectory(root);
        using TextureAsset source = new TextureAsset {
            Width = 3, Height = 1, ColorFormat = TextureAssetColorFormat.Indexed4,
            AlphaPrecision = TextureAssetAlphaPrecision.A8, Colors = [0, 0], PaletteColors = [255, 255, 255, 255]
        };
        if (corruption == "texels") {
            source.Colors = [0];
        } else if (corruption == "palette") {
            source.PaletteColors = [255, 255, 255];
        } else if (corruption == "index") {
            source.Colors = [0xFF, 0x0F];
        } else {
            source.ColorFormat = (TextureAssetColorFormat)255;
        }
        string path = Path.Combine(root, "malformed-" + corruption + ".hetex");
        File.WriteAllBytes(path, helengine.files.AssetSerializer.SerializeToBytes(source));
        Assert.ThrowsAny<Exception>(() => new Nintendo64SourceTextureDecoder().Decode(path));
    }

    /// <summary>Preserves legacy JSON and rejects unsupported alpha values and duplicate properties.</summary>
    [Fact]
    public void SettingsKeepLegacyPayloadAndRejectInvalidEnums() {
        string legacy = "{\"maxResolution\":64,\"colorFormat\":\"Rgba32\",\"alphaPrecision\":\"A8\"}";
        Assert.Equal(legacy, Nintendo64TextureCookSettingsSerializer.Serialize(Nintendo64TextureCookSettingsSerializer.Deserialize(legacy)));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize("{\"maxResolution\":64,\"colorFormat\":\"Indexed8\",\"alphaPrecision\":\"255\"}"));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize("{\"maxResolution\":64,\"colorFormat\":\"Indexed8\",\"alphaPrecision\":\"A2\"}"));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize("{\"maxResolution\":64,\"maxResolution\":32,\"colorFormat\":\"Indexed8\",\"alphaPrecision\":\"A8\"}"));
    }

    /// <summary>Creates settings with the explicitly selected shared indexing strategy.</summary>
    /// <param name="format">Output storage format.</param>
    /// <param name="maximum">Existing N64 size cap to apply before conversion.</param>
    /// <returns>Valid settings for the requested storage.</returns>
    static TextureAssetProcessorSettings Settings(TextureAssetColorFormat format, int maximum) {
        return new TextureAssetProcessorSettings {
            MaxResolution = maximum,
            ColorFormat = format,
            AlphaPrecision = TextureAssetAlphaPrecision.A8,
            IndexingMethodId = format == TextureAssetColorFormat.Rgba32 ? string.Empty : "QuantizedIndexed"
        };
    }

    /// <summary>Creates a caller-owned white RGBA fixture with complete authoring metadata.</summary>
    /// <param name="width">Fixture width.</param>
    /// <param name="height">Fixture height.</param>
    /// <returns>Independent RGBA pixels the cooker must not mutate or dispose.</returns>
    static TextureAsset CreateTexture(ushort width, ushort height) {
        return new TextureAsset {
            Id = "source", RuntimeAssetId = 123, AuthoringAssetId = "authoring-id", FormerAuthoringAssetIds = ["former-id"], IsEngineOwned = true,
            Width = width, Height = height, Colors = Enumerable.Repeat((byte)255, width * height * 4).ToArray(),
            ColorFormat = TextureAssetColorFormat.Rgba32, AlphaPrecision = TextureAssetAlphaPrecision.A8
        };
    }
}
