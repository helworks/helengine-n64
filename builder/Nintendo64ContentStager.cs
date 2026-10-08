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
        if (IsWithin(sourceRoot, stagingRoot) || IsWithin(stagingRoot, sourceRoot)) {
            throw new InvalidOperationException("N64 source and staging roots must be separate, non-overlapping directories.");
        }
        EnsureNoReparsePoints(sourceRoot);
        EnsureNoReparsePoints(stagingRoot);
        HashSet<string> paths = new(StringComparer.OrdinalIgnoreCase);
        List<string> stagedPaths = new(artifacts.Count + builderCookedRelativePaths.Count);
        List<(string Source, string Destination)> copies = new(artifacts.Count + builderCookedRelativePaths.Count);
        foreach (PlatformBuildArtifact artifact in artifacts) {
            cancellationToken.ThrowIfCancellationRequested();
            PlanOne(artifact.RelativePath, sourceRoot, stagingRoot, paths, stagedPaths, copies);
        }
        foreach (string builderCookedRelativePath in builderCookedRelativePaths) {
            cancellationToken.ThrowIfCancellationRequested();
            PlanOne(builderCookedRelativePath, sourceRoot, stagingRoot, paths, stagedPaths, copies);
        }
        // Complete validation before replacing a previous valid packaging tree.
        cancellationToken.ThrowIfCancellationRequested();
        if (Directory.Exists(stagingRoot)) Directory.Delete(stagingRoot, true);
        Directory.CreateDirectory(stagingRoot);
        foreach (var copy in copies) {
            cancellationToken.ThrowIfCancellationRequested();
            Directory.CreateDirectory(Path.GetDirectoryName(copy.Destination) ?? stagingRoot);
            File.Copy(copy.Source, copy.Destination, true);
        }
        return stagedPaths;
    }

    /// <summary>
    /// Validates one cooked file and records its copy into the DragonFS input tree.
    /// </summary>
    /// <param name="candidateRelativePath">Runtime-relative path of the cooked file.</param>
    /// <param name="sourceRoot">Absolute package source root.</param>
    /// <param name="stagingRoot">Absolute DragonFS input tree root.</param>
    /// <param name="paths">Paths staged so far, used to reject collisions.</param>
    /// <param name="stagedPaths">Accumulated staged path list.</param>
    /// <param name="copies">Copies to execute after all inputs pass validation.</param>
    static void PlanOne(
        string candidateRelativePath,
        string sourceRoot,
        string stagingRoot,
        HashSet<string> paths,
        List<string> stagedPaths,
        List<(string Source, string Destination)> copies) {
        string relativePath = NormalizeRelativePath(candidateRelativePath);
        if (!paths.Add(relativePath)) throw new InvalidOperationException($"N64 cooked artifact path collision: '{relativePath}'.");
        string sourcePath = ResolveUnderRoot(sourceRoot, relativePath);
        EnsureNoReparsePoints(sourcePath);
        if (!File.Exists(sourcePath)) throw new FileNotFoundException($"N64 cooked artifact '{relativePath}' was not found under package source root '{sourceRoot}'.", sourcePath);
        string destinationPath = ResolveUnderRoot(stagingRoot, relativePath);
        copies.Add((sourcePath, destinationPath));
        stagedPaths.Add(relativePath);
    }

    public static string NormalizeRelativePath(string path) {
        if (string.IsNullOrWhiteSpace(path)) throw new InvalidOperationException("N64 cooked artifact path is required.");
        string normalized = path.Replace('\\', '/');
        if (normalized.StartsWith('/') || Path.IsPathRooted(normalized) || normalized.Split('/').Any(segment => segment == "." || segment == "..")) {
            throw new InvalidOperationException($"N64 cooked artifact path must remain relative: '{path}'.");
        }
        string canonical = string.Join('/', normalized.Split('/').Where(segment => segment.Length != 0));
        if (canonical.Length == 0 || canonical.Split('/').Any(segment => segment.Contains(':') || segment.EndsWith('.') || segment.EndsWith(' '))) {
            throw new InvalidOperationException($"N64 cooked artifact path is not a portable file path: '{path}'.");
        }
        return canonical;
    }

    /// <summary>Compares complete directory segments so sibling names with a common prefix remain independent.</summary>
    static bool IsWithin(string path, string root) {
        string normalizedRoot = root.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        return string.Equals(path.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar), normalizedRoot, StringComparison.OrdinalIgnoreCase)
            || path.StartsWith(normalizedRoot + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }

    /// <summary>Rejects linked files and directory ancestors whose physical target would bypass lexical containment.</summary>
    static void EnsureNoReparsePoints(string path) {
        for (string current = path; current != null; current = Path.GetDirectoryName(current)) {
            if ((Directory.Exists(current) || File.Exists(current)) && (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0) {
                throw new InvalidOperationException($"N64 staging paths must not traverse symbolic links or junctions: '{current}'.");
            }
        }
    }

    static string ResolveUnderRoot(string root, string relativePath) {
        string fullRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
        string fullPath = Path.GetFullPath(Path.Combine(fullRoot, relativePath.Replace('/', Path.DirectorySeparatorChar)));
        if (!fullPath.StartsWith(fullRoot, StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException($"N64 path escapes staging root: '{relativePath}'.");
        return fullPath;
    }
}
