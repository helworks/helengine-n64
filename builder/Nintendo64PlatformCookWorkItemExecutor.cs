using helengine.baseplatform.Manifest;
using helengine.editor;

namespace helengine.n64.builder;

/// <summary>
/// Executes builder-owned N64 platform cook work items by writing cooked runtime payloads into the package source root.
/// </summary>
public sealed class Nintendo64PlatformCookWorkItemExecutor {
    /// <summary>
    /// Source asset kinds this executor accepts, mirroring the source kinds published by the platform's asset
    /// cook capabilities: the ordinary "texture" capability and the higher-resolution "font-atlas-texture"
    /// capability that keeps authored glyph atlases from being crushed to the 64 pixel texture cap.
    /// </summary>
    static readonly HashSet<string> SupportedSourceAssetKinds = new(StringComparer.OrdinalIgnoreCase) { "texture", "font-atlas-texture" };

    /// <summary>
    /// Decodes authored source images into raw texture assets.
    /// </summary>
    readonly Nintendo64SourceTextureDecoder SourceTextureDecoder = new();

    /// <summary>
    /// Downscales decoded textures into the cooked N64 runtime form.
    /// </summary>
    readonly Nintendo64TextureCooker TextureCooker = new();

    /// <summary>
    /// Decodes and cooks one source texture work item into the package source root. The settings used for the
    /// cook are the ones the work item's owning capability already resolved: the work item's own
    /// <see cref="PlatformCookWorkItem.SerializedPlatformSettings"/> carries whichever capability (texture or
    /// font-atlas-texture) matched its <see cref="PlatformCookWorkItem.SourceAssetKind"/>. This method only
    /// selects the accepted source kind and disposes its own decoded and cooked assets after serialization.
    /// </summary>
    /// <param name="workItem">Work item describing the source asset, settings, and cooked output path.</param>
    /// <param name="packageSourceRootPath">Package source root that the DragonFS stager reads.</param>
    public void Execute(PlatformCookWorkItem workItem, string packageSourceRootPath) {
        if (workItem == null) {
            throw new ArgumentNullException(nameof(workItem));
        }
        if (string.IsNullOrWhiteSpace(packageSourceRootPath)) {
            throw new ArgumentException("Package source root path must be provided.", nameof(packageSourceRootPath));
        }
        if (!SupportedSourceAssetKinds.Contains(workItem.SourceAssetKind)) {
            throw new InvalidOperationException($"N64 does not support source asset kind '{workItem.SourceAssetKind}' for work item '{workItem.WorkItemId}' at '{workItem.SourceAssetPath}'.");
        }

        TextureAssetProcessorSettings settings = Nintendo64TextureCookSettingsSerializer.Deserialize(workItem.SerializedPlatformSettings);
        using TextureAsset sourceTexture = SourceTextureDecoder.Decode(workItem.SourceAssetPath);
        using TextureAsset cookedTexture = TextureCooker.Cook(sourceTexture, settings);
        cookedTexture.Id = workItem.OutputLogicalArtifactId;

        string destinationPath = Path.Combine(
            Path.GetFullPath(packageSourceRootPath),
            Nintendo64ContentStager.NormalizeRelativePath(workItem.OutputRelativePath).Replace('/', Path.DirectorySeparatorChar));
        string destinationDirectory = Path.GetDirectoryName(destinationPath)
            ?? throw new InvalidOperationException($"N64 cooked texture destination directory could not be resolved for '{destinationPath}'.");
        Directory.CreateDirectory(destinationDirectory);
        File.WriteAllBytes(destinationPath, helengine.files.AssetSerializer.SerializeToBytes(cookedTexture));
    }
}
