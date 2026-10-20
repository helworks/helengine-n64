using helengine.baseplatform.Definitions;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Results;
using helengine.editor;
using helengine.n64.builder;
using System.Text.Json;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64PlatformDefinitionTests {
    [Fact]
    public void Create_PublishesStableN64IdsAndRestrictedCapabilities() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        Assert.Equal("n64", definition.PlatformId);
        Assert.Equal("n64-standard", Assert.Single(definition.GraphicsProfiles).ProfileId);
        PlatformMaterialSchemaDefinition schema = Assert.Single(definition.MaterialSchemas);
        Assert.Equal("n64-standard-v1", schema.SchemaId);
        PlatformMaterialFieldDefinition lighting = Assert.Single(schema.Fields, field => field.FieldId == Nintendo64MaterialSchemaIds.LightingModeFieldId);
        Assert.Equal("lit", lighting.DefaultValue);
        Assert.Equal(new[] { "lit", "unlit" }, lighting.AllowedValues);
        Assert.Equal(new[] { "debug", "release", "cpu-profile" }, definition.BuildProfiles.Select(profile => profile.ProfileId));
        PlatformBuildProfileDefinition cpuProfile = Assert.Single(definition.BuildProfiles, profile => profile.ProfileId == "cpu-profile");
        Assert.Equal(Nintendo64PlatformDefinitionFactory.CpuProfileCodegenProfileId, cpuProfile.CodegenProfileId);
        Assert.All(definition.BuildProfiles, profile => Assert.Equal("n64-standard", profile.GraphicsProfileId));
        PlatformCodegenProfileDefinition profiled = Assert.Single(definition.CodegenProfiles, profile => profile.ProfileId == "cpu-profile");
        Assert.Contains(profiled.Settings, setting => setting.SettingId == PlatformCodegenSettingIds.EnabledFeatures && setting.DefaultValue == "runtime_profiler");
        PlatformSettingDefinition forcedDisabled = Assert.Single(
            definition.CodegenProfiles.Single(profile => profile.ProfileId == "default").Settings,
            setting => setting.SettingId == PlatformCodegenSettingIds.ForcedDisabledFeatures);
        Assert.Contains("shaders", forcedDisabled.DefaultValue, StringComparison.OrdinalIgnoreCase);
        // `text2d` was removed deliberately. Listing it never closed the gate: feature pruning restores a gated
        // type whenever a surviving type references it, and the ungated `FPSComponent` references
        // `TextComponent`, so the text machinery was compiled into the ROM the whole time it was listed here.
        Assert.DoesNotContain("text2d", forcedDisabled.DefaultValue, StringComparison.OrdinalIgnoreCase);
        Assert.Contains("debug_overlay", forcedDisabled.DefaultValue, StringComparison.OrdinalIgnoreCase);
        Assert.DoesNotContain("render2d", forcedDisabled.DefaultValue, StringComparison.OrdinalIgnoreCase);
        Assert.DoesNotContain("sprites", forcedDisabled.DefaultValue, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void Create_DeclaresExceptionsAndRttiBecauseTheGeneratedCoreNeedsBoth() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        PlatformSettingDefinition useExceptions = Assert.Single(
            definition.CodegenProfiles.Single(profile => profile.ProfileId == "default").Settings,
            setting => setting.SettingId == Nintendo64PlatformDefinitionFactory.UseExceptionsSettingId);
        PlatformSettingDefinition useRtti = Assert.Single(
            definition.CodegenProfiles.Single(profile => profile.ProfileId == "default").Settings,
            setting => setting.SettingId == Nintendo64PlatformDefinitionFactory.UseRttiSettingId);
        Assert.Equal("true", useExceptions.DefaultValue);
        Assert.Equal("true", useRtti.DefaultValue);
    }

    /// <summary>
    /// Pins the complete set of <c>codegen-use-*</c> knobs the C++ generator reads, with the values the
    /// shipped generated Core was produced with. Any knob the profile leaves undeclared falls through to
    /// whichever runtime profile the generator CLI preset carries, so a shift in that default would otherwise
    /// surface as a native compile error hundreds of generated files deep instead of as a failing test here.
    /// </summary>
    [Fact]
    public void Create_DeclaresEveryCodegenUseKnobTheGeneratorReads() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        Dictionary<string, string> expectedCodegenUseSettings = new(StringComparer.Ordinal) {
            [Nintendo64PlatformDefinitionFactory.UseStdStringSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseStdVectorSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseStdUnorderedMapSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseStdUnorderedSetSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseStdFunctionSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseStdChronoSettingId] = "false",
            [Nintendo64PlatformDefinitionFactory.UseStdSharedPtrSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseStdMathSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseHostedFileSystemSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseExceptionsSettingId] = "true",
            [Nintendo64PlatformDefinitionFactory.UseRttiSettingId] = "true"
        };

        Dictionary<string, string> declaredCodegenUseSettings = definition.CodegenProfiles
            .Single(profile => profile.ProfileId == "default")
            .Settings
            .Where(setting => setting.SettingId.StartsWith("codegen-use-", StringComparison.Ordinal))
            .ToDictionary(setting => setting.SettingId, setting => setting.DefaultValue, StringComparer.Ordinal);

        Assert.Equal(expectedCodegenUseSettings.OrderBy(entry => entry.Key, StringComparer.Ordinal), declaredCodegenUseSettings.OrderBy(entry => entry.Key, StringComparer.Ordinal));
        Assert.All(
            definition.CodegenProfiles.Single(profile => profile.ProfileId == "default").Settings.Where(setting => setting.SettingId.StartsWith("codegen-use-", StringComparison.Ordinal)),
            setting => Assert.Equal(PlatformSettingKind.Boolean, setting.SettingKind));
    }

    [Fact]
    public void Create_UsesNativeMonotonicClockProvider() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();
        PlatformSettingDefinition provider = Assert.Single(definition.CodegenProfiles.Single(profile => profile.ProfileId == "default").Settings,
            setting => setting.SettingId == "codegen-runtime-provider-header");

        Assert.Equal(PlatformSettingKind.Text, provider.SettingKind);
        Assert.Equal("platform/n64/Nintendo64RuntimeProvider.hpp", provider.DefaultValue);
    }

    [Fact]
    public void Create_SupportsDirectionalAndAmbientLightsAndRejectsPointAndSpot() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        PlatformComponentSupportRule directional = Assert.Single(definition.ComponentSupportRules, rule => rule.ComponentTypeId == "helengine.DirectionalLightComponent");
        PlatformComponentSupportRule ambient = Assert.Single(definition.ComponentSupportRules, rule => rule.ComponentTypeId == "helengine.AmbientLightComponent");
        PlatformComponentSupportRule point = Assert.Single(definition.ComponentSupportRules, rule => rule.ComponentTypeId == "helengine.PointLightComponent");
        PlatformComponentSupportRule spot = Assert.Single(definition.ComponentSupportRules, rule => rule.ComponentTypeId == "helengine.SpotLightComponent");
        Assert.Equal(PlatformComponentSupportKind.Transform, directional.SupportKind);
        Assert.Equal(PlatformComponentSupportKind.Transform, ambient.SupportKind);
        Assert.Equal(PlatformComponentSupportKind.Unsupported, point.SupportKind);
        Assert.Equal(PlatformComponentSupportKind.Unsupported, spot.SupportKind);
        Assert.DoesNotContain(definition.ComponentSupportRules, rule => rule.ComponentTypeId == "helengine.lightcomponent");
    }

    [Fact]
    public void Create_DoesNotOptIntoCodegenFeaturesForMinimalN64Profile() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        Assert.DoesNotContain(definition.CodegenProfiles.Single(profile => profile.ProfileId == "default").Settings, setting => setting.SettingId == PlatformCodegenSettingIds.EnabledFeatures);
    }

    [Fact]
    public void Create_UsesOnlyFeatureIdsDeclaredByHelengineCatalog() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();
        string catalogPath = ResolveFeatureCatalogPath();
        Assert.True(File.Exists(catalogPath), $"HelEngine feature catalog was not found at '{catalogPath}'.");

        using JsonDocument catalog = JsonDocument.Parse(File.ReadAllText(catalogPath));
        HashSet<string> knownFeatureIds = catalog.RootElement
            .GetProperty("features")
            .EnumerateArray()
            .Select(feature => feature.GetProperty("id").GetString() ?? string.Empty)
            .Where(featureId => !string.IsNullOrWhiteSpace(featureId))
            .ToHashSet(StringComparer.Ordinal);

        foreach (PlatformSettingDefinition setting in definition.CodegenProfiles.SelectMany(profile => profile.Settings).Where(setting =>
                     setting.SettingId == PlatformCodegenSettingIds.EnabledFeatures
                     || setting.SettingId == PlatformCodegenSettingIds.ForcedDisabledFeatures)) {
            foreach (string featureId in (setting.DefaultValue ?? string.Empty).Split(
                         [';', ',', ' '],
                         StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)) {
                // This opt-in marker is consumed by the editor preprocessor resolver rather than the feature catalog.
                if (string.Equals(featureId, "runtime_profiler", StringComparison.Ordinal)) continue;
                Assert.Contains(featureId, knownFeatureIds);
            }
        }
    }

    [Fact]
    public void CookMaterial_EmitsLitOpaqueAuthoredColorByDefault() {
        Nintendo64PlatformAssetBuilder builder = new();
        PlatformMaterialCookResult result = builder.CookMaterial(new PlatformMaterialCookRequest(
            "mat-cube",
            "materials/cube",
            "n64",
            "debug",
            "n64-standard",
            Nintendo64MaterialSchemaIds.StandardSchemaId,
            new Dictionary<string, string> {
                [Nintendo64MaterialSchemaIds.BaseColorFieldId] = "#12A0FF"
            }, numericPolicy: global::helengine.NumericPrecisionPolicy.Default));

        helengine.PlatformMaterialAsset cooked = Assert.IsType<helengine.PlatformMaterialAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(result.CookedMaterialBytes));
        Assert.Equal("n64-standard", cooked.RendererFamilyId);
        Assert.True(cooked.Lit);
        Assert.False(cooked.UseVertexColor);
        Assert.True(string.IsNullOrEmpty(cooked.TextureRelativePath));
        Assert.Equal((byte)0x12, cooked.BaseColorR);
        Assert.Equal((byte)0xA0, cooked.BaseColorG);
        Assert.Equal((byte)0xFF, cooked.BaseColorB);
        Assert.Equal((byte)0xFF, cooked.BaseColorA);
    }

    [Fact]
    public void CookMaterial_EmitsUnlitWhenRequested() {
        Nintendo64PlatformAssetBuilder builder = new();
        PlatformMaterialCookResult result = builder.CookMaterial(new PlatformMaterialCookRequest(
            "mat-cube",
            "materials/cube",
            "n64",
            "debug",
            "n64-standard",
            Nintendo64MaterialSchemaIds.StandardSchemaId,
            new Dictionary<string, string> {
                [Nintendo64MaterialSchemaIds.LightingModeFieldId] = "unlit"
            }, numericPolicy: global::helengine.NumericPrecisionPolicy.Default));

        helengine.PlatformMaterialAsset cooked = Assert.IsType<helengine.PlatformMaterialAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(result.CookedMaterialBytes));
        Assert.False(cooked.Lit);
    }

    [Fact]
    public void CookMaterial_RejectsUnsupportedFeatures() {
        Nintendo64PlatformAssetBuilder builder = new();
        PlatformMaterialCookRequest request(string key, string value) => new(
            "mat-cube",
            "materials/cube",
            "n64",
            "debug",
            "n64-standard",
            Nintendo64MaterialSchemaIds.StandardSchemaId,
            new Dictionary<string, string> { [key] = value });

        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.BaseColorFieldId, "#FFFFFF80")));
        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.LightingModeFieldId, "pbr")));
        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.TransparencyFieldId, "alpha")));
    }

    [Fact]
    public void Create_DeclaresBuilderOwnedTextureCookCapability() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        // N64 now publishes a second, builder-owned capability for font-atlas-texture sources (see
        // Create_ExposesASeparateFontAtlasCookCapability), so this filters to the plain texture capability
        // rather than asserting there is only one capability overall.
        PlatformAssetCookCapabilityDefinition capability = Assert.Single(
            definition.AssetCookCapabilities,
            candidate => candidate.SourceAssetKind == "texture");
        Assert.Equal("texture", capability.SourceAssetKind);
        Assert.Equal("runtime-texture", capability.TargetArtifactKind);
        Assert.Equal(PlatformAssetCookOwnershipKind.BuilderOwned, capability.OwnershipKind);
        Assert.Equal("n64-texture", capability.SettingsContractId);

        TextureAssetProcessorSettings settings = Nintendo64TextureCookSettingsSerializer.Deserialize(capability.DefaultSerializedPlatformSettings);
        Assert.Equal(64, settings.MaxResolution);
        Assert.Equal(TextureAssetColorFormat.Rgba32, settings.ColorFormat);
        Assert.Equal(TextureAssetAlphaPrecision.A8, settings.AlphaPrecision);
        Assert.Contains(TextureAssetColorFormat.Rgba32.ToString(), capability.TextureFormatCapabilities.SupportedColorFormatIds);
    }

    /// <summary>
    /// Ensures the font-atlas-texture capability is a distinct builder-owned capability from the plain texture
    /// capability, with its own settings contract id and its own, much higher resolution cap: the shared
    /// "texture" cap of 64 crushes the editor default (256x128) and authored body (512x256) font atlases to
    /// two or three pixel glyphs, which is exactly what a second capability exists to avoid.
    /// </summary>
    [Fact]
    public void Create_ExposesASeparateFontAtlasCookCapability() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        PlatformAssetCookCapabilityDefinition texture = Assert.Single(
            definition.AssetCookCapabilities,
            capability => capability.SourceAssetKind == "texture");
        PlatformAssetCookCapabilityDefinition fontAtlas = Assert.Single(
            definition.AssetCookCapabilities,
            capability => capability.SourceAssetKind == "font-atlas-texture");

        Assert.Equal("n64-texture", texture.SettingsContractId);
        Assert.Equal("n64-font-atlas-texture", fontAtlas.SettingsContractId);
        Assert.Equal("runtime-texture", fontAtlas.TargetArtifactKind);
        Assert.Equal(PlatformAssetCookOwnershipKind.BuilderOwned, fontAtlas.OwnershipKind);

        TextureAssetProcessorSettings textureSettings =
            Nintendo64TextureCookSettingsSerializer.Deserialize(texture.DefaultSerializedPlatformSettings);
        TextureAssetProcessorSettings fontSettings =
            Nintendo64TextureCookSettingsSerializer.Deserialize(fontAtlas.DefaultSerializedPlatformSettings);

        Assert.Equal(64, textureSettings.MaxResolution);
        Assert.Equal(512, fontSettings.MaxResolution);
        // The two caps must not drift back together: one shared value is what crushed font atlases to 64.
        Assert.NotEqual(textureSettings.MaxResolution, fontSettings.MaxResolution);
    }

    /// <summary>
    /// Ensures both the authored text component and the FPS overlay (which is built entirely out of authored
    /// text components) are recognized as transform-only supported component types, rather than falling through
    /// to the default unsupported/rejected path now that the N64 text pass exists.
    /// </summary>
    [Fact]
    public void Create_SupportsTextAndFpsComponentsWithTransformOnly() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        // The brief's fixture text used `ComponentTypeName`; the real property on
        // PlatformComponentSupportRule is `ComponentTypeId` (see other assertions in this file), so this uses
        // that name while keeping the same assertion meaning: exactly one rule per component type name.
        PlatformComponentSupportRule text = Assert.Single(
            definition.ComponentSupportRules,
            rule => rule.ComponentTypeId == "helengine.TextComponent");
        PlatformComponentSupportRule fps = Assert.Single(
            definition.ComponentSupportRules,
            rule => rule.ComponentTypeId == "helengine.FPSComponent");

        Assert.Equal(PlatformComponentSupportKind.Transform, text.SupportKind);
        Assert.Equal(PlatformComponentSupportKind.Transform, fps.SupportKind);
    }

    [Fact]
    public void TextureCookSettings_RoundTripThroughSerializer() {
        TextureAssetProcessorSettings original = new() {
            MaxResolution = 32,
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = TextureAssetAlphaPrecision.Opaque
        };

        TextureAssetProcessorSettings roundTripped = Nintendo64TextureCookSettingsSerializer.Deserialize(
            Nintendo64TextureCookSettingsSerializer.Serialize(original));

        Assert.Equal(32, roundTripped.MaxResolution);
        Assert.Equal(TextureAssetColorFormat.Rgba32, roundTripped.ColorFormat);
        Assert.Equal(TextureAssetAlphaPrecision.Opaque, roundTripped.AlphaPrecision);
    }

    [Fact]
    public void TextureCookSettings_RejectUnsupportedColorFormat() {
        // "Bogus" is not an enum member at all, so it fails the parse and never reaches the supported-format
        // check. "Rgba4444" parses cleanly, which is what actually exercises the N64-only-emits-Rgba32 branch.
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize(
            "{\"maxResolution\":64,\"colorFormat\":\"Bogus\",\"alphaPrecision\":\"A8\"}"));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize(
            "{\"maxResolution\":64,\"colorFormat\":\"Rgba4444\",\"alphaPrecision\":\"A8\"}"));
    }

    [Fact]
    public void TextureCookSettings_RejectPayloadMissingRequiredProperties() {
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize(
            "{\"colorFormat\":\"Rgba32\",\"alphaPrecision\":\"A8\"}"));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize(
            "{\"maxResolution\":64,\"alphaPrecision\":\"A8\"}"));
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize(
            "{\"maxResolution\":64,\"colorFormat\":\"Rgba32\"}"));
    }

    static string ResolveFeatureCatalogPath() {
        string configuredRoot = Environment.GetEnvironmentVariable("HELENGINE_ROOT") ?? string.Empty;
        if (!string.IsNullOrWhiteSpace(configuredRoot)) {
            return Path.Combine(configuredRoot, "engine", "helengine.editor", "codegen", "features", "helengine-feature-catalog.json");
        }

        DirectoryInfo currentDirectory = new(AppContext.BaseDirectory);
        for (int depth = 0; depth < 10 && currentDirectory != null; depth++) {
            string siblingCandidate = Path.Combine(
                currentDirectory.FullName,
                "helengine",
                "engine",
                "helengine.editor",
                "codegen",
                "features",
                "helengine-feature-catalog.json");
            if (File.Exists(siblingCandidate)) {
                return siblingCandidate;
            }

            currentDirectory = currentDirectory.Parent;
        }

        return Path.Combine(AppContext.BaseDirectory, "helengine", "engine", "helengine.editor", "codegen", "features", "helengine-feature-catalog.json");
    }
}
