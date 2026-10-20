namespace helengine.n64.builder;

/// <summary>Stable editor field identifiers and values for the N64 opaque material schema.</summary>
public static class Nintendo64MaterialSchemaIds {
    /// <summary>Schema id for opaque N64 materials with optional textures and lighting.</summary>
    public const string StandardSchemaId = "n64-standard-v1";
    /// <summary>Graphics profile and cooked renderer family id shared by every N64 build profile.</summary>
    public const string StandardGraphicsProfileId = "n64-standard";
    /// <summary>Field id for the opaque base color.</summary>
    public const string BaseColorFieldId = "base-color";
    /// <summary>Field id for the optional content-relative cooked texture binding.</summary>
    public const string TextureRelativePathFieldId = "texture-relative-path";
    /// <summary>Field id selecting Tiny3D vertex lighting or unlit rendering.</summary>
    public const string LightingModeFieldId = "lighting-mode";
    /// <summary>Lighting value that enables Tiny3D ambient and directional vertex lighting.</summary>
    public const string LightingLitValue = "lit";
    /// <summary>Lighting value that draws the base color without any light contribution.</summary>
    public const string LightingUnlitValue = "unlit";
    /// <summary>Field id for the transparency mode, which only accepts opaque.</summary>
    public const string TransparencyFieldId = "transparency";
}
