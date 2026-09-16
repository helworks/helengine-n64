using helengine.baseplatform.Manifest;

namespace helengine.n64.builder;

/// <summary>
/// Stages the cooked files the ROM must carry into the DragonFS input tree: the manifest-declared artifacts plus
/// the outputs this builder cooked itself. The editor keeps builder-owned outputs out of the manifest, so staging
/// the manifest alone would ship a ROM whose sprite textures and font atlases are missing from the filesystem.
/// </summary>
public sealed class Nintendo64ContentStager {
    /// <summary>
    /// Stages the manifest-declared cooked artifacts together with the builder's own cook outputs. There is
    /// deliberately no artifacts-only overload: staging the manifest alone is exactly the bug that shipped a
    /// ROM with no sprite textures, so every caller has to say what the builder cooked, even if that is empty.
    /// </summary>
    /// <param name="artifacts">Manifest-declared cooked artifacts.</param>
    /// <param name="builderCookedRelativePaths">Runtime-relative outputs this builder cooked from platform work items.</param>
    /// <param name="sourceRootPath">Package source root holding every cooked file.</param>
    /// <param name="stagingRootPath">DragonFS input tree to rebuild.</param>
    /// <param name="cancellationToken">Token observed between files.</param>
    /// <returns>Runtime-relative paths staged, in staging order.</returns>
    public IReadOnlyList<string> Stage(
        IReadOnlyList<PlatformBuildArtifact> artifacts,
        IReadOnlyList<string> builderCookedRelativePaths,
        string sourceRootPath,
        string stagingRootPath,
        CancellationToken cancellationToken) {
        if (artifacts == null) throw new ArgumentNullException(nameof(artifacts));
        if (builderCookedRelativePaths == null) throw new ArgumentNullException(nameof(builderCookedRelativePaths));
        if (string.IsNullOrWhiteSpace(sourceRootPath)) throw new ArgumentException("Source root is required.", nameof(sourceRootPath));
        if (string.IsNullOrWhiteSpace(stagingRootPath)) throw new ArgumentException("Staging root is required.", nameof(stagingRootPath));
        string sourceRoot = Path.GetFullPath(sourceRootPath);
        string stagingRoot = Path.GetFullPath(stagingRootPath);
        if (string.Equals(sourceRoot.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar), stagingRoot.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar), StringComparison.OrdinalIgnoreCase)) {
            throw new InvalidOperationException("N64 source and staging roots must be different directories.");
        }
        if (Directory.Exists(stagingRoot)) Directory.Delete(stagingRoot, true);
        Directory.CreateDirectory(stagingRoot);
        HashSet<string> paths = new(StringComparer.OrdinalIgnoreCase);
        List<string> stagedPaths = new(artifacts.Count + builderCookedRelativePaths.Count);
        foreach (PlatformBuildArtifact artifact in artifacts) {
            cancellationToken.ThrowIfCancellationRequested();
            StageOne(artifact.RelativePath, sourceRoot, stagingRoot, paths, stagedPaths);
        }
        foreach (string builderCookedRelativePath in builderCookedRelativePaths) {
            cancellationToken.ThrowIfCancellationRequested();
            StageOne(builderCookedRelativePath, sourceRoot, stagingRoot, paths, stagedPaths);
        }
        return stagedPaths;
    }

    /// <summary>
    /// Copies one cooked file from the package source root into the DragonFS input tree.
    /// </summary>
    /// <param name="candidateRelativePath">Runtime-relative path of the cooked file.</param>
    /// <param name="sourceRoot">Absolute package source root.</param>
    /// <param name="stagingRoot">Absolute DragonFS input tree root.</param>
    /// <param name="paths">Paths staged so far, used to reject collisions.</param>
    /// <param name="stagedPaths">Accumulated staged path list.</param>
    static void StageOne(
        string candidateRelativePath,
        string sourceRoot,
        string stagingRoot,
        HashSet<string> paths,
        List<string> stagedPaths) {
        string relativePath = NormalizeRelativePath(candidateRelativePath);
        if (!paths.Add(relativePath)) throw new InvalidOperationException($"N64 cooked artifact path collision: '{relativePath}'.");
        string sourcePath = ResolveUnderRoot(sourceRoot, relativePath);
        if (!File.Exists(sourcePath)) throw new FileNotFoundException($"N64 cooked artifact '{relativePath}' was not found under package source root '{sourceRoot}'.", sourcePath);
        string destinationPath = ResolveUnderRoot(stagingRoot, relativePath);
        string destinationDirectory = Path.GetDirectoryName(destinationPath) ?? stagingRoot;
        Directory.CreateDirectory(destinationDirectory);
        File.Copy(sourcePath, destinationPath, true);
        stagedPaths.Add(relativePath);
    }

    public static string NormalizeRelativePath(string path) {
        if (string.IsNullOrWhiteSpace(path)) throw new InvalidOperationException("N64 cooked artifact path is required.");
        string normalized = path.Replace('\\', '/');
        if (normalized.StartsWith('/') || Path.IsPathRooted(normalized) || normalized.Split('/').Any(segment => segment == "..")) {
            throw new InvalidOperationException($"N64 cooked artifact path must remain relative: '{path}'.");
        }
        return normalized;
    }

    static string ResolveUnderRoot(string root, string relativePath) {
        string fullRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
        string fullPath = Path.GetFullPath(Path.Combine(fullRoot, relativePath.Replace('/', Path.DirectorySeparatorChar)));
        if (!fullPath.StartsWith(fullRoot, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException($"N64 path escapes staging root: '{relativePath}'.");
        return fullPath;
    }
}
