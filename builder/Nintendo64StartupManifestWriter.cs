using System.Globalization;
using System.Text;
using helengine.baseplatform.Manifest;

namespace helengine.n64.builder;

public sealed record Nintendo64StartupManifestEntry(string SceneId, string CookedRelativePath);

/// <summary>Writes a small builder-owned catalog consumed by the N64 generated runtime host.</summary>
public sealed class Nintendo64StartupManifestWriter {
    /// <summary>Writes an explicit catalog with the default cooperative budget and no loading overlay.</summary>
    public string Write(string generatedCoreCppRootPath, string startupSceneId, IReadOnlyList<Nintendo64StartupManifestEntry> entries) {
        return Write(generatedCoreCppRootPath, startupSceneId, entries, 8d, string.Empty);
    }

    /// <summary>Validates the complete catalog before writing native startup and loading-policy getters.</summary>
    string Write(string generatedCoreCppRootPath, string startupSceneId, IReadOnlyList<Nintendo64StartupManifestEntry> entries, double sceneLoadingBudgetMilliseconds, string loadingScreenSceneId) {
        if (string.IsNullOrWhiteSpace(generatedCoreCppRootPath)) throw new ArgumentException("Generated Core root is required.", nameof(generatedCoreCppRootPath));
        if (string.IsNullOrWhiteSpace(startupSceneId)) throw new ArgumentException("Startup scene id is required.", nameof(startupSceneId));
        if (entries == null || entries.Count == 0) throw new InvalidOperationException("N64 startup catalog requires at least one scene.");
        if (!double.IsFinite(sceneLoadingBudgetMilliseconds) || sceneLoadingBudgetMilliseconds <= 0d) throw new InvalidOperationException("N64 scene loading requires a finite, positive millisecond budget.");
        if (loadingScreenSceneId == null || (loadingScreenSceneId.Length > 0 && string.IsNullOrWhiteSpace(loadingScreenSceneId))) throw new InvalidOperationException("N64 loading-screen scene id must be empty or a valid packaged scene id.");
        HashSet<string> ids = new(StringComparer.Ordinal);
        foreach (Nintendo64StartupManifestEntry entry in entries) {
            if (entry == null || string.IsNullOrWhiteSpace(entry.SceneId) || string.IsNullOrWhiteSpace(entry.CookedRelativePath)) throw new InvalidOperationException("N64 startup catalog contains an incomplete entry.");
            if (!ids.Add(entry.SceneId)) throw new InvalidOperationException($"N64 startup catalog contains duplicate scene id '{entry.SceneId}'.");
            Nintendo64ContentStager.NormalizeRelativePath(entry.CookedRelativePath);
        }
        if (!ids.Contains(startupSceneId)) throw new InvalidOperationException($"N64 startup scene '{startupSceneId}' is missing from the catalog.");
        if (loadingScreenSceneId.Length > 0) {
            if (!ids.Contains(loadingScreenSceneId)) throw new InvalidOperationException($"N64 loading-screen scene '{loadingScreenSceneId}' must identify exactly one packaged scene.");
        }
        string root = Path.GetFullPath(generatedCoreCppRootPath);
        Directory.CreateDirectory(root);
        StringBuilder builder = new();
        builder.AppendLine("#pragma once");
        builder.AppendLine("#include <cstddef>");
        builder.AppendLine("struct HERuntimeSceneCatalogEntry { const char* SceneId; const char* CookedRelativePath; };");
        builder.AppendLine("extern \"C\" const char* he_get_runtime_startup_scene_id();");
        builder.AppendLine("extern \"C\" double he_get_runtime_scene_loading_budget_milliseconds();");
        builder.AppendLine("extern \"C\" const char* he_get_runtime_loading_screen_scene_id();");
        builder.AppendLine("extern \"C\" const HERuntimeSceneCatalogEntry* he_runtime_scene_catalog_entries(std::size_t* count);");
        string headerPath = Path.Combine(root, "n64_runtime_scene_catalog.hpp");
        WriteIfChanged(headerPath, builder.ToString());
        StringBuilder source = new();
        source.AppendLine("#include \"n64_runtime_scene_catalog.hpp\"");
        source.AppendLine("namespace {");
        source.AppendLine("const HERuntimeSceneCatalogEntry kEntries[] = {");
        foreach (Nintendo64StartupManifestEntry entry in entries) {
            source.Append("{\"").Append(Escape(entry.SceneId)).Append("\", \"").Append(Escape(Nintendo64ContentStager.NormalizeRelativePath(entry.CookedRelativePath))).AppendLine("\"},");
        }
        source.AppendLine("};");
        source.Append("const char kStartupSceneId[] = \"").Append(Escape(startupSceneId)).AppendLine("\";");
        source.Append("const double kSceneLoadingBudgetMilliseconds = ").Append(sceneLoadingBudgetMilliseconds.ToString("R", CultureInfo.InvariantCulture)).AppendLine(";");
        source.Append("const char kLoadingScreenSceneId[] = \"").Append(Escape(loadingScreenSceneId)).AppendLine("\";");
        source.AppendLine("}");
        source.AppendLine("extern \"C\" const char* he_get_runtime_startup_scene_id() { return kStartupSceneId; }");
        source.AppendLine("extern \"C\" double he_get_runtime_scene_loading_budget_milliseconds() { return kSceneLoadingBudgetMilliseconds; }");
        source.AppendLine("extern \"C\" const char* he_get_runtime_loading_screen_scene_id() { return kLoadingScreenSceneId; }");
        source.AppendLine("extern \"C\" const HERuntimeSceneCatalogEntry* he_runtime_scene_catalog_entries(std::size_t* count) { if (count != nullptr) *count = sizeof(kEntries) / sizeof(kEntries[0]); return kEntries; }");
        string sourcePath = Path.Combine(root, "n64_runtime_scene_catalog.cpp");
        WriteIfChanged(sourcePath, source.ToString());
        return headerPath;
    }

    /// <summary>Resolves the manifest's packaged scenes and transports its shared loading policy without changing startup order.</summary>
    public string Write(string generatedCoreCppRootPath, PlatformBuildManifest manifest) {
        if (manifest == null) throw new ArgumentNullException(nameof(manifest));
        PlatformBuildArtifact[] sceneArtifacts = manifest.CookedArtifacts
            .Where(artifact => string.Equals(artifact.ArtifactKind, "scene", StringComparison.OrdinalIgnoreCase))
            .ToArray();
        Nintendo64StartupManifestEntry[] entries = manifest.Scenes.Select(scene => {
            PlatformBuildArtifact artifact = sceneArtifacts.FirstOrDefault(candidate => string.Equals(candidate.LogicalArtifactId, scene.SceneId, StringComparison.Ordinal))!;
            if (artifact == null) {
                string normalizedSourceIdentity = Nintendo64ContentStager.NormalizeRelativePath(scene.SourceIdentity);
                artifact = sceneArtifacts.FirstOrDefault(candidate => string.Equals(Nintendo64ContentStager.NormalizeRelativePath(candidate.RelativePath), normalizedSourceIdentity, StringComparison.Ordinal));
            }
            if (artifact == null && sceneArtifacts.Length == 1 && manifest.Scenes.Length == 1) artifact = sceneArtifacts[0];
            if (artifact == null) throw new InvalidOperationException($"N64 startup scene '{scene.SceneId}' has no cooked scene artifact in the manifest.");
            return new Nintendo64StartupManifestEntry(scene.SceneId, artifact.RelativePath);
        }).ToArray();
        return Write(generatedCoreCppRootPath, manifest.StartupSceneId, entries, manifest.SceneLoadingBudgetMilliseconds, manifest.LoadingScreenSceneId);
    }

    /// <summary>Escapes scene identifiers and paths for native C string literals.</summary>
    static string Escape(string value) => value.Replace("\\", "\\\\", StringComparison.Ordinal).Replace("\"", "\\\"", StringComparison.Ordinal).Replace("\r", "\\r", StringComparison.Ordinal).Replace("\n", "\\n", StringComparison.Ordinal);

    /// <summary>Keeps native inputs unchanged when their complete generated contents are already current.</summary>
    static void WriteIfChanged(string path, string content) {
        if (File.Exists(path) && string.Equals(File.ReadAllText(path), content, StringComparison.Ordinal)) return;
        File.WriteAllText(path, content, new UTF8Encoding(false));
    }
}
