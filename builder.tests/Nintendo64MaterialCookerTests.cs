using helengine.baseplatform.Requests;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64MaterialCookerTests {
    [Theory]
    [InlineData("lit", true)]
    [InlineData("unlit", false)]
    public void Cook_PreservesTextureBindingAndOpaqueTint(string lighting, bool expectedLit) {
        var result = new Nintendo64MaterialCooker().Cook(Request(new() {
            [Nintendo64MaterialSchemaIds.TextureRelativePathFieldId] = "cooked/imported/checker.hetex",
            [Nintendo64MaterialSchemaIds.LightingModeFieldId] = lighting,
            [Nintendo64MaterialSchemaIds.BaseColorFieldId] = "#12A0F0FF"
        }));

        var material = Assert.IsType<helengine.PlatformMaterialAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(result.CookedMaterialBytes));
        Assert.Equal("cooked/imported/checker.hetex", material.TextureRelativePath);
        Assert.Equal("n64-standard", material.RendererFamilyId);
        Assert.Equal(expectedLit, material.Lit);
        Assert.Equal((byte)0x12, material.BaseColorR);
        Assert.Equal((byte)0xA0, material.BaseColorG);
        Assert.Equal((byte)0xF0, material.BaseColorB);
        Assert.Equal((byte)0xFF, material.BaseColorA);
        Assert.False(material.DoubleSided);
        Assert.False(material.UseVertexColor);
        Assert.Empty(result.ReferencedShaderDependencies);
    }

    [Fact]
    public void Cook_NormalizesTexturePathLikeTheContentStager() {
        var result = new Nintendo64MaterialCooker().Cook(Request(new() {
            [Nintendo64MaterialSchemaIds.TextureRelativePathFieldId] = @"cooked\imported\checker.hetex"
        }));

        var material = Assert.IsType<helengine.PlatformMaterialAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(result.CookedMaterialBytes));
        Assert.Equal("cooked/imported/checker.hetex", material.TextureRelativePath);
    }

    [Theory]
    [InlineData("")]
    [InlineData("   ")]
    public void Cook_KeepsAnUntexturedMaterialWhenNoBindingIsAuthored(string texturePath) {
        var result = new Nintendo64MaterialCooker().Cook(Request(new() {
            [Nintendo64MaterialSchemaIds.TextureRelativePathFieldId] = texturePath
        }));

        var material = Assert.IsType<helengine.PlatformMaterialAsset>(helengine.files.AssetSerializer.DeserializeFromBytes(result.CookedMaterialBytes));
        Assert.Equal(string.Empty, material.TextureRelativePath);
    }

    [Theory]
    [InlineData("../checker.hetex")]
    [InlineData("textures/../checker.hetex")]
    [InlineData(@"textures\..\checker.hetex")]
    [InlineData("textures/./checker.hetex")]
    [InlineData("/textures/checker.hetex")]
    [InlineData(@"\\server\textures\checker.hetex")]
    [InlineData("C:/textures/checker.hetex")]
    [InlineData("rom:/textures/checker.hetex")]
    [InlineData("https://example.com/checker.hetex")]
    [InlineData("textures./checker.hetex")]
    public void Cook_RejectsNonportableOrEscapingTexturePaths(string texturePath) {
        Assert.Throws<InvalidOperationException>(() => new Nintendo64MaterialCooker().Cook(Request(new() {
            [Nintendo64MaterialSchemaIds.TextureRelativePathFieldId] = texturePath
        })));
    }

    [Theory]
    [InlineData(Nintendo64MaterialSchemaIds.LightingModeFieldId, "pbr")]
    [InlineData(Nintendo64MaterialSchemaIds.TransparencyFieldId, "alpha")]
    [InlineData(Nintendo64MaterialSchemaIds.BaseColorFieldId, "#FFFFFF80")]
    public void Cook_TexturedMaterialsRetainLightingAndOpacityRestrictions(string field, string value) {
        Assert.Throws<InvalidOperationException>(() => new Nintendo64MaterialCooker().Cook(Request(new() {
            [Nintendo64MaterialSchemaIds.TextureRelativePathFieldId] = "cooked/imported/checker.hetex",
            [field] = value
        })));
    }

    static PlatformMaterialCookRequest Request(Dictionary<string, string> fields) => new(
        "checker-material", "materials/checker.hasset", "n64", "debug", "n64-standard",
        Nintendo64MaterialSchemaIds.StandardSchemaId, fields, numericPolicy: global::helengine.NumericPrecisionPolicy.Default);
}
