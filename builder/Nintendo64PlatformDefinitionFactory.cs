using helengine.baseplatform.Definitions;
using helengine.baseplatform.Profiles;
using helengine.editor;

namespace helengine.n64.builder;

/// <summary>Creates the editor-facing N64 platform contract.</summary>
public static class Nintendo64PlatformDefinitionFactory {
    // Every id here must exist in the checked-in engine feature catalog, which
    // Create_UsesOnlyFeatureIdsDeclaredByHelengineCatalog asserts. This list deliberately does NOT match the
    // generator's own n64-minimal preset: that preset also forces render2d off and selects the custom-retro
    // runtime, which would strip the sprite pass and the std types the shipped generated Core is built on.
    // render2d and sprites stay enabled so authored SpriteComponents reach the runtime. `text2d` used to be
    // listed here too, but it never closed the gate: feature pruning restores a gated type whenever a
    // surviving type references it, and the ungated `FPSComponent` references `TextComponent` directly, so
    // the text machinery was compiled into the ROM the whole time `text2d` was listed. It is removed here
    // rather than left as dead weight.
    const string ForcedDisabledFeatures = "debug_overlay;shaders";

    /// <summary>Stable build-profile id that enables the optional N64 CPU profiler.</summary>
    public const string CpuProfileBuildProfileId = "cpu-profile";

    /// <summary>Stable codegen-profile id that enables generated Core CPU probes.</summary>
    public const string CpuProfileCodegenProfileId = "cpu-profile";

    /// <summary>Codegen option id selecting <c>std::string</c> as the generated Core's string storage.</summary>
    public const string UseStdStringSettingId = "codegen-use-std-string";

    /// <summary>
    /// Codegen option id selecting <c>std::vector</c> as the generated Core's dynamic list storage.
    /// </summary>
    public const string UseStdVectorSettingId = "codegen-use-std-vector";

    /// <summary>
    /// Codegen option id selecting <c>std::unordered_map</c> as the generated Core's dictionary storage.
    /// </summary>
    public const string UseStdUnorderedMapSettingId = "codegen-use-std-unordered-map";

    /// <summary>
    /// Codegen option id selecting <c>std::unordered_set</c> as the generated Core's hash-set storage.
    /// </summary>
    public const string UseStdUnorderedSetSettingId = "codegen-use-std-unordered-set";

    /// <summary>
    /// Codegen option id selecting <c>std::function</c> as the generated Core's delegate storage.
    /// </summary>
    public const string UseStdFunctionSettingId = "codegen-use-std-function";

    /// <summary>
    /// Codegen option id selecting <c>std::chrono</c> as the generated Core's monotonic clock source.
    /// </summary>
    public const string UseStdChronoSettingId = "codegen-use-std-chrono";

    /// <summary>
    /// Codegen option id selecting the N64 monotonic runtime-provider header.
    /// </summary>
    public const string RuntimeProviderHeaderSettingId = "codegen-runtime-provider-header";

    /// <summary>
    /// Codegen option id selecting <c>std::shared_ptr</c> as the generated Core's shared span ownership.
    /// </summary>
    public const string UseStdSharedPtrSettingId = "codegen-use-std-shared-ptr";

    /// <summary>
    /// Codegen option id selecting the standard C++ math library for the generated Core's numeric helpers.
    /// </summary>
    public const string UseStdMathSettingId = "codegen-use-std-math";

    /// <summary>
    /// Codegen option id selecting the hosted file-stream implementation, which the N64 build redirects to
    /// its own content stream source through the native file system header and type settings.
    /// </summary>
    public const string UseHostedFileSystemSettingId = "codegen-use-hosted-file-system";

    /// <summary>
    /// Codegen option id selecting C++ exception unwinding for the generated Core.
    /// </summary>
    public const string UseExceptionsSettingId = "codegen-use-exceptions";

    /// <summary>
    /// Codegen option id selecting compiler RTTI for the generated Core's casts and generic dispatch.
    /// </summary>
    public const string UseRttiSettingId = "codegen-use-rtti";

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
                new("native-file-system-type", "Native File System Type", PlatformSettingKind.Text, "helengine::n64::Nintendo64ContentStreamSource", true, []),
                // All eleven codegen-use-* knobs the C++ generator reads are declared here, not just the two
                // the Core visibly breaks without. Anything left undeclared falls through to whichever runtime
                // profile the CLI preset happens to carry, and a shift in that default silently regenerates a
                // Core this repository's native build cannot compile - which is exactly how this branch lost a
                // day. Every value below is true because that is what the shipped generated Core was built
                // with and what it needs: std::string and std::vector back the generated asset fields and
                // system/io/directory.hpp #errors without them, the unordered containers back the generated
                // dictionaries and hash sets, std::function backs delegates, std::chrono backs Stopwatch,
                // std::shared_ptr backs shared span storage, std::math backs system/math.hpp and
                // system/number.hpp, the hosted file stream is what the native file system settings below
                // redirect into Nintendo64ContentStreamSource, and the Core throws from generated
                // deserializers and selects generic implementations through dynamic_cast. The libdragon
                // toolchain compiles the generated Core with exceptions and RTTI on: n64.mk passes neither
                // -fno-exceptions nor -fno-rtti, so the GCC defaults apply.
                new(UseStdStringSettingId, "Use std::string", PlatformSettingKind.Boolean, "true", true, []),
                new(UseStdVectorSettingId, "Use std::vector", PlatformSettingKind.Boolean, "true", true, []),
                new(UseStdUnorderedMapSettingId, "Use std::unordered_map", PlatformSettingKind.Boolean, "true", true, []),
                new(UseStdUnorderedSetSettingId, "Use std::unordered_set", PlatformSettingKind.Boolean, "true", true, []),
                new(UseStdFunctionSettingId, "Use std::function", PlatformSettingKind.Boolean, "true", true, []),
                // libdragon's hosted gettimeofday path is unavailable on the console. The N64 provider
                // routes Stopwatch through get_ticks_us(), which is the platform's monotonic clock.
                new(UseStdChronoSettingId, "Use std::chrono", PlatformSettingKind.Boolean, "false", true, []),
                new(RuntimeProviderHeaderSettingId, "Runtime Provider Header", PlatformSettingKind.Text, "platform/n64/Nintendo64RuntimeProvider.hpp", true, []),
                new(UseStdSharedPtrSettingId, "Use std::shared_ptr", PlatformSettingKind.Boolean, "true", true, []),
                new(UseStdMathSettingId, "Use Standard Math", PlatformSettingKind.Boolean, "true", true, []),
                new(UseHostedFileSystemSettingId, "Use Hosted File System", PlatformSettingKind.Boolean, "true", true, []),
                new(UseExceptionsSettingId, "Use C++ Exceptions", PlatformSettingKind.Boolean, "true", true, []),
                new(UseRttiSettingId, "Use C++ RTTI", PlatformSettingKind.Boolean, "true", true, [])
            ]);

        PlatformCodegenProfileDefinition cpuProfileCodegen = new(
            CpuProfileCodegenProfileId,
            "CPU Profile",
            "N64 generated runtime with Core CPU probes enabled",
            PlatformCodegenLanguage.Cpp,
            PlatformSerializationEndianness.LittleEndian,
            [
                .. codegen.Settings,
                new(PlatformCodegenSettingIds.EnabledFeatures, "Enabled Features", PlatformSettingKind.Text, "runtime_profiler", true, [])
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
            new("release", "Release", "N64 release build", Nintendo64MaterialSchemaIds.StandardGraphicsProfileId, "default", buildSettings),
            new(CpuProfileBuildProfileId, "CPU Profile", "N64 build with opt-in Core and native CPU profiling", Nintendo64MaterialSchemaIds.StandardGraphicsProfileId, CpuProfileCodegenProfileId, buildSettings)
        ];

        PlatformAssetCookCapabilityDefinition[] assetCookCapabilities = [
            new PlatformAssetCookCapabilityDefinition(
                "texture",
                "runtime-texture",
                PlatformAssetCookOwnershipKind.BuilderOwned,
                "n64-texture",
                CreateDefaultSerializedTextureCookSettings(),
                CreateTextureFormatCapabilities()),
            new PlatformAssetCookCapabilityDefinition(
                "font-atlas-texture",
                "runtime-texture",
                PlatformAssetCookOwnershipKind.BuilderOwned,
                "n64-font-atlas-texture",
                CreateDefaultSerializedFontAtlasCookSettings(),
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
                new PlatformComponentSupportRule("helengine.SpotLightComponent", PlatformComponentSupportKind.Unsupported, "N64 lighting supports directional and ambient lights only.", "Remove spot lights or exclude them on N64."),
                new PlatformComponentSupportRule("helengine.TextComponent", PlatformComponentSupportKind.Transform, "Authored text renders through the shared 2D command list; selection and input are not driven.", string.Empty),
                new PlatformComponentSupportRule("helengine.FPSComponent", PlatformComponentSupportKind.Transform, "The FPS overlay renders as ordinary authored text.", string.Empty)
            ],
            [codegen, cpuProfileCodegen],
            [new PlatformStorageProfileDefinition("dragonfs-package", "DragonFS Package", PlatformStorageProfileKind.LooseFiles, "n64-dragonfs-package", false)],
            [new PlatformMediaProfileDefinition("n64-cartridge", "N64 Cartridge", PlatformMediaLayoutKind.InstallTree, false, true)],
            new RuntimeGenerationContract(RuntimeMaterialResolutionMode.CookedPlatformOwned, true, PackagedPathPolicy.ContentRelativeOnly,
                null, false, requiresCookedViewportLayout: true),
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
    /// Creates the serialized default N64 font atlas cook settings. Font atlases keep a far higher resolution
    /// cap than ordinary textures. A body font rasterized at the authored 32 pixel size uses a 512x256 atlas;
    /// reducing that atlas also reduces glyph metrics and makes text smaller despite an unchanged FontScale.
    /// Indexed8 retains all 256 coverage levels in its RGBA palette; the runtime chooses IA16 for grayscale
    /// glyph palettes. The 512x256 index surface uses 128 KiB without changing any glyph source rectangle.
    /// </summary>
    /// <returns>Serialized default N64 font atlas settings.</returns>
    static string CreateDefaultSerializedFontAtlasCookSettings() {
        return Nintendo64TextureCookSettingsSerializer.Serialize(new TextureAssetProcessorSettings {
            MaxResolution = 512,
            ColorFormat = TextureAssetColorFormat.Indexed8,
            AlphaPrecision = TextureAssetAlphaPrecision.A8,
            IndexingMethodId = TextureAssetIndexingMethod.QuantizedIndexed.ToString()
        });
    }

    /// <summary>
    /// Publishes the direct RGBA and palette-backed storage formats supported by the N64 runtime.
    /// </summary>
    /// <returns>N64 texture format capabilities.</returns>
    static PlatformTextureFormatCapabilityDefinition CreateTextureFormatCapabilities() {
        return new PlatformTextureFormatCapabilityDefinition(
            [TextureAssetColorFormat.Rgba32.ToString(), TextureAssetColorFormat.Indexed4.ToString(), TextureAssetColorFormat.Indexed8.ToString()],
            [TextureAssetAlphaPrecision.Opaque, TextureAssetAlphaPrecision.Binary, TextureAssetAlphaPrecision.A4, TextureAssetAlphaPrecision.A8],
            [
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Rgba32.ToString(), TextureAssetAlphaPrecision.Opaque),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Rgba32.ToString(), TextureAssetAlphaPrecision.A8),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed4.ToString(), TextureAssetAlphaPrecision.Opaque),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed4.ToString(), TextureAssetAlphaPrecision.Binary),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed4.ToString(), TextureAssetAlphaPrecision.A4),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed4.ToString(), TextureAssetAlphaPrecision.A8),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed8.ToString(), TextureAssetAlphaPrecision.Opaque),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed8.ToString(), TextureAssetAlphaPrecision.Binary),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed8.ToString(), TextureAssetAlphaPrecision.A4),
                new PlatformTextureFormatCombinationDefinition(TextureAssetColorFormat.Indexed8.ToString(), TextureAssetAlphaPrecision.A8)
            ]);
    }
}
