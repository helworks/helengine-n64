using helengine.baseplatform.Manifest;

namespace helengine.n64.builder;

/// <summary>Stages only manifest-declared cooked files into the DragonFS input tree.</summary>
public sealed class Nintendo64ContentStager {
    public IReadOnlyList<string> Stage(
        IReadOnlyList<PlatformBuildArtifact> artifacts,
        string sourceRootPath,
        string stagingRootPath,
        CancellationToken cancellationToken) {
        if (artifacts == null) throw new ArgumentNullException(nameof(artifacts));
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
        List<string> stagedPaths = new(artifacts.Count);
        foreach (PlatformBuildArtifact artifact in artifacts) {
            cancellationToken.ThrowIfCancellationRequested();
            string relativePath = NormalizeRelativePath(artifact.RelativePath);
            if (!paths.Add(relativePath)) throw new InvalidOperationException($"N64 cooked artifact path collision: '{relativePath}'.");
            string sourcePath = ResolveUnderRoot(sourceRoot, relativePath);
            if (!File.Exists(sourcePath)) throw new FileNotFoundException($"N64 cooked artifact '{relativePath}' was not found under package source root '{sourceRoot}'.", sourcePath);
            string destinationPath = ResolveUnderRoot(stagingRoot, relativePath);
            string destinationDirectory = Path.GetDirectoryName(destinationPath) ?? stagingRoot;
            Directory.CreateDirectory(destinationDirectory);
            File.Copy(sourcePath, destinationPath, true);
            stagedPaths.Add(relativePath);
        }
        return stagedPaths;
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
