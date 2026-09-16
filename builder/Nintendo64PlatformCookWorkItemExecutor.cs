using helengine.baseplatform.Manifest;
using helengine.editor;

namespace helengine.n64.builder;

/// <summary>
/// Executes builder-owned N64 platform cook work items by writing cooked runtime payloads into the package source root.
/// </summary>
public sealed class Nintendo64PlatformCookWorkItemExecutor {
    /// <summary>
    /// Decodes authored source images into raw texture assets.
    /// </summary>
    readonly Nintendo64SourceTextureDecoder SourceTextureDecoder = new();

    /// <summary>
    /// Downscales decoded textures into the cooked N64 runtime form.
    /// </summary>
    readonly Nintendo64TextureCooker TextureCooker = new();

    /// <summary>
    /// Decodes and cooks one source texture work item into the package source root.
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
        if (!string.Equals(workItem.SourceAssetKind, "texture", StringComparison.OrdinalIgnoreCase)) {
            throw new InvalidOperationException($"N64 does not support source asset kind '{workItem.SourceAssetKind}'.");
        }

        TextureAssetProcessorSettings settings = Nintendo64TextureCookSettingsSerializer.Deserialize(workItem.SerializedPlatformSettings);
        TextureAsset cookedTexture = TextureCooker.Cook(SourceTextureDecoder.Decode(workItem.SourceAssetPath), settings);
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
