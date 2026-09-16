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
        Assert.Equal(new[] { "debug", "release" }, definition.BuildProfiles.Select(profile => profile.ProfileId));
        Assert.All(definition.BuildProfiles, profile => Assert.Equal("n64-standard", profile.GraphicsProfileId));
        Assert.Contains(definition.CodegenProfiles.Single().Settings, setting =>
            setting.SettingId == PlatformCodegenSettingIds.ForcedDisabledFeatures
            && setting.DefaultValue.Contains("shaders", StringComparison.OrdinalIgnoreCase)
            && setting.DefaultValue.Contains("render2d", StringComparison.OrdinalIgnoreCase)
            && setting.DefaultValue.Contains("text2d", StringComparison.OrdinalIgnoreCase));
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

        Assert.DoesNotContain(
            definition.CodegenProfiles.Single().Settings,
            setting => setting.SettingId == PlatformCodegenSettingIds.EnabledFeatures);
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

        foreach (PlatformSettingDefinition setting in definition.CodegenProfiles.Single().Settings.Where(setting =>
                     setting.SettingId == PlatformCodegenSettingIds.EnabledFeatures
                     || setting.SettingId == PlatformCodegenSettingIds.ForcedDisabledFeatures)) {
            foreach (string featureId in (setting.DefaultValue ?? string.Empty).Split(
                         [';', ',', ' '],
                         StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)) {
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
            }));

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
            }));

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

        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.TextureRelativePathFieldId, "textures/cube.hetex")));
        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.LightingModeFieldId, "pbr")));
        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.TransparencyFieldId, "alpha")));
    }

    [Fact]
    public void Create_DeclaresBuilderOwnedTextureCookCapability() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        PlatformAssetCookCapabilityDefinition capability = Assert.Single(definition.AssetCookCapabilities);
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
        Assert.Throws<InvalidOperationException>(() => Nintendo64TextureCookSettingsSerializer.Deserialize(
            "{\"maxResolution\":64,\"colorFormat\":\"Bogus\",\"alphaPrecision\":\"A8\"}"));
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
