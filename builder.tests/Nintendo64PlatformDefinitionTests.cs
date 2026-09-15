using helengine.baseplatform.Definitions;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Results;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64PlatformDefinitionTests {
    [Fact]
    public void Create_PublishesStableN64IdsAndRestrictedCapabilities() {
        PlatformDefinition definition = Nintendo64PlatformDefinitionFactory.Create();

        Assert.Equal("n64", definition.PlatformId);
        Assert.Equal("n64-unlit", Assert.Single(definition.GraphicsProfiles).ProfileId);
        Assert.Equal("n64-unlit-v1", Assert.Single(definition.MaterialSchemas).SchemaId);
        Assert.Equal(new[] { "debug", "release" }, definition.BuildProfiles.Select(profile => profile.ProfileId));
        Assert.Empty(definition.AssetCookCapabilities);
        Assert.Contains(definition.CodegenProfiles.Single().Settings, setting =>
            setting.SettingId == PlatformCodegenSettingIds.ForcedDisabledFeatures
            && setting.DefaultValue.Contains("textures", StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public void CookMaterial_EmitsOpaqueUnlitAuthoredColor() {
        Nintendo64PlatformAssetBuilder builder = new();
        PlatformMaterialCookResult result = builder.CookMaterial(new PlatformMaterialCookRequest(
            "mat-cube",
            "materials/cube",
            "n64",
            "debug",
            "n64-unlit",
            Nintendo64MaterialSchemaIds.UnlitSchemaId,
            new Dictionary<string, string> {
                [Nintendo64MaterialSchemaIds.BaseColorFieldId] = "#12A0FF"
            }));

        helengine.PlatformMaterialAsset cooked = Assert.IsType<helengine.PlatformMaterialAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(result.CookedMaterialBytes));
        Assert.Equal("n64-unlit", cooked.RendererFamilyId);
        Assert.False(cooked.Lit);
        Assert.False(cooked.UseVertexColor);
        Assert.True(string.IsNullOrEmpty(cooked.TextureRelativePath));
        Assert.Equal((byte)0x12, cooked.BaseColorR);
        Assert.Equal((byte)0xA0, cooked.BaseColorG);
        Assert.Equal((byte)0xFF, cooked.BaseColorB);
        Assert.Equal((byte)0xFF, cooked.BaseColorA);
    }

    [Fact]
    public void CookMaterial_RejectsUnsupportedFeatures() {
        Nintendo64PlatformAssetBuilder builder = new();
        PlatformMaterialCookRequest request(string key, string value) => new(
            "mat-cube",
            "materials/cube",
            "n64",
            "debug",
            "n64-unlit",
            Nintendo64MaterialSchemaIds.UnlitSchemaId,
            new Dictionary<string, string> { [key] = value });

        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.TextureRelativePathFieldId, "textures/cube.hetex")));
        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.LightingModeFieldId, "lit")));
        Assert.Throws<InvalidOperationException>(() => builder.CookMaterial(request(Nintendo64MaterialSchemaIds.TransparencyFieldId, "alpha")));
    }
}
