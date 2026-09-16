using helengine.baseplatform.Definitions;
using helengine.baseplatform.Profiles;
using helengine.editor;

namespace helengine.n64.builder;

/// <summary>Creates the editor-facing N64 platform contract.</summary>
public static class Nintendo64PlatformDefinitionFactory {
    // Keep this list aligned with the checked-in codegen catalog and the n64-minimal preset.
    // render2d and sprites are enabled so authored SpriteComponents reach the runtime;
    // text2d stays disabled until the N64 text pass exists.
    const string ForcedDisabledFeatures = "debug_overlay;shaders;text2d";

    public static PlatformDefinition Create() {
        PlatformSettingDefinition[] buildSettings = [
            new("game-name", "Game Name", PlatformSettingKind.Text, "helengine", true, []),
            new("enable-native-runtime-diagnostics", "Enable Native Runtime Diagnostics", PlatformSettingKind.Boolean, "false", true, [])
        ];

        PlatformCodegenProfileDefinition codegen = new(
            "default",
            "Default",
            "N64 C# to C++ generated runtime profile",
            PlatformCodegenLanguage.Cpp,
            PlatformSerializationEndianness.LittleEndian,
            [
                new(PlatformCodegenSettingIds.ForcedDisabledFeatures, "Forced Disabled Features", PlatformSettingKind.Text, ForcedDisabledFeatures, true, []),
                new("generated-math-convention", "Generated Math Convention", PlatformSettingKind.Text, "native-column-vector", true, []),
                new("pointer-size-bytes", "Pointer Size (Bytes)", PlatformSettingKind.Text, "4", true, []),
                new("type-remaps", "Type Remaps", PlatformSettingKind.Text, "System.Numerics.Vector2=helengine.float2;System.Numerics.Vector3=helengine.float3;System.Numerics.Vector4=helengine.float4;System.Numerics.Quaternion=helengine.float4", true, []),
                new("native-file-system-header", "Native File System Header", PlatformSettingKind.Text, "\"platform/n64/Nintendo64ContentStreamSource.hpp\"", true, []),
                new("native-file-system-type", "Native File System Type", PlatformSettingKind.Text, "helengine::n64::Nintendo64ContentStreamSource", true, [])
            ]);

        PlatformMaterialSchemaDefinition materialSchema = new(
            Nintendo64MaterialSchemaIds.StandardSchemaId,
            "N64 Opaque",
            [Nintendo64MaterialSchemaIds.StandardGraphicsProfileId],
            [
                new(Nintendo64MaterialSchemaIds.BaseColorFieldId, "Base Color", PlatformMaterialFieldKind.Color, "#FFFFFFFF", true, []),
                new(Nintendo64MaterialSchemaIds.TextureRelativePathFieldId, "Texture", PlatformMaterialFieldKind.Text, string.Empty, false, []),
                new(Nintendo64MaterialSchemaIds.LightingModeFieldId, "Lighting", PlatformMaterialFieldKind.Choice, Nintendo64MaterialSchemaIds.LightingLitValue, true, [Nintendo64MaterialSchemaIds.LightingLitValue, Nintendo64MaterialSchemaIds.LightingUnlitValue]),
                new(Nintendo64MaterialSchemaIds.TransparencyFieldId, "Transparency", PlatformMaterialFieldKind.Choice, "opaque", true, ["opaque"])
            ]);

        PlatformBuildProfileDefinition[] profiles = [
            new("debug", "Debug", "N64 development build", Nintendo64MaterialSchemaIds.StandardGraphicsProfileId, "default", buildSettings),
            new("release", "Release", "N64 release build", Nintendo64MaterialSchemaIds.StandardGraphicsProfileId, "default", buildSettings)
        ];

        PlatformAssetCookCapabilityDefinition[] assetCookCapabilities = [
            new PlatformAssetCookCapabilityDefinition(
                "texture",
                "runtime-texture",
                PlatformAssetCookOwnershipKind.BuilderOwned,
                "n64-texture",
                CreateDefaultSerializedTextureCookSettings(),
                CreateTextureFormatCapabilities())
        ];

        return new PlatformDefinition(
            "n64",
            "Nintendo 64",
            profiles,
            [new PlatformGraphicsProfileDefinition(Nintendo64MaterialSchemaIds.StandardGraphicsProfileId, "N64 Standard", "Tiny3D opaque renderer with hardware vertex lighting", [])],
            [new PlatformAssetRequirementDefinition("scene", "Scene", true, ["helen"])],
            [materialSchema],
            [
                new PlatformComponentSupportRule("helengine.meshcomponent", PlatformComponentSupportKind.Transform, "One static triangle-list mesh is supported.", "Remove unsupported mesh components."),
                new PlatformComponentSupportRule("helengine.cameracomponent", PlatformComponentSupportKind.Transform, "One perspective camera is supported.", "Keep one authored camera."),
                new PlatformComponentSupportRule("helengine.DirectionalLightComponent", PlatformComponentSupportKind.Transform, "Tiny3D supports up to seven directional lights.", string.Empty),
                new PlatformComponentSupportRule("helengine.AmbientLightComponent", PlatformComponentSupportKind.Transform, "Tiny3D supports one summed ambient color.", string.Empty),
                new PlatformComponentSupportRule("helengine.PointLightComponent", PlatformComponentSupportKind.Unsupported, "N64 lighting supports directional and ambient lights only.", "Remove point lights or exclude them on N64."),
                new PlatformComponentSupportRule("helengine.SpotLightComponent", PlatformComponentSupportKind.Unsupported, "N64 lighting supports directional and ambient lights only.", "Remove spot lights or exclude them on N64.")
            ],
            [codegen],
            [new PlatformStorageProfileDefinition("dragonfs-package", "DragonFS Package", PlatformStorageProfileKind.LooseFiles, "n64-dragonfs-package", false)],
            [new PlatformMediaProfileDefinition("n64-cartridge", "N64 Cartridge", PlatformMediaLayoutKind.InstallTree, false, true)],
            new RuntimeGenerationContract(RuntimeMaterialResolutionMode.CookedPlatformOwned, true, PackagedPathPolicy.ContentRelativeOnly),
            assetCookCapabilities: assetCookCapabilities);
    }

    /// <summary>
    /// Creates the serialized default N64 texture cook settings used when an asset carries no N64 override.
    /// </summary>
    /// <returns>Serialized default N64 texture settings.</returns>
    static string CreateDefaultSerializedTextureCookSettings() {
        return Nintendo64TextureCookSettingsSerializer.Serialize(new TextureAssetProcessorSettings {
            MaxResolution = 64,
            ColorFormat = TextureAssetColorFormat.Rgba32,
            AlphaPrecision = TextureAssetAlphaPrecision.A8
        });
    }

    /// <summary>
    /// Creates the N64 texture format capability set: one straight RGBA source format with opaque or eight-bit alpha.
    /// </summary>
    /// <returns>N64 texture format capabilities.</returns>
    static PlatformTextureFormatCapabilityDefinition CreateTextureFormatCapabilities() {
        return new PlatformTextureFormatCapabilityDefinition(
            [TextureAssetColorFormat.Rgba32.ToString()],
            [TextureAssetAlphaPrecision.Opaque, TextureAssetAlphaPrecision.A8],
            [
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Rgba32.ToString(), TextureAssetAlphaPrecision.Opaque),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Rgba32.ToString(), TextureAssetAlphaPrecision.A8)
            ]);
    }
}
