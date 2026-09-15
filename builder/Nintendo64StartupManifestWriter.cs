using System.Text;
using helengine.baseplatform.Manifest;

namespace helengine.n64.builder;

public sealed record Nintendo64StartupManifestEntry(string SceneId, string CookedRelativePath);

/// <summary>Writes a small builder-owned catalog consumed by the N64 generated runtime host.</summary>
public sealed class Nintendo64StartupManifestWriter {
    public string Write(string generatedCoreCppRootPath, string startupSceneId, IReadOnlyList<Nintendo64StartupManifestEntry> entries) {
        if (string.IsNullOrWhiteSpace(generatedCoreCppRootPath)) throw new ArgumentException("Generated Core root is required.", nameof(generatedCoreCppRootPath));
        if (string.IsNullOrWhiteSpace(startupSceneId)) throw new ArgumentException("Startup scene id is required.", nameof(startupSceneId));
        if (entries == null || entries.Count == 0) throw new InvalidOperationException("N64 startup catalog requires at least one scene.");
        if (!entries.Any(entry => string.Equals(entry.SceneId, startupSceneId, StringComparison.Ordinal))) throw new InvalidOperationException($"N64 startup scene '{startupSceneId}' is missing from the catalog.");
        HashSet<string> ids = new(StringComparer.Ordinal);
        string root = Path.GetFullPath(generatedCoreCppRootPath);
        Directory.CreateDirectory(root);
        StringBuilder builder = new();
        builder.AppendLine("#pragma once");
        builder.AppendLine("#include <cstddef>");
        builder.AppendLine("struct HERuntimeSceneCatalogEntry { const char* SceneId; const char* CookedRelativePath; };");
        builder.AppendLine("extern \"C\" const char* he_get_runtime_startup_scene_id();");
        builder.AppendLine("extern \"C\" const HERuntimeSceneCatalogEntry* he_runtime_scene_catalog_entries(std::size_t* count);");
        string headerPath = Path.Combine(root, "n64_runtime_scene_catalog.hpp");
        File.WriteAllText(headerPath, builder.ToString(), new UTF8Encoding(false));
        StringBuilder source = new();
        source.AppendLine("#include \"n64_runtime_scene_catalog.hpp\"");
        source.AppendLine("namespace {");
        source.AppendLine("const HERuntimeSceneCatalogEntry kEntries[] = {");
        foreach (Nintendo64StartupManifestEntry entry in entries) {
            if (entry == null || string.IsNullOrWhiteSpace(entry.SceneId) || string.IsNullOrWhiteSpace(entry.CookedRelativePath)) throw new InvalidOperationException("N64 startup catalog contains an incomplete entry.");
            if (!ids.Add(entry.SceneId)) throw new InvalidOperationException($"N64 startup catalog contains duplicate scene id '{entry.SceneId}'.");
            source.Append("{\"").Append(Escape(entry.SceneId)).Append("\", \"").Append(Escape(Nintendo64ContentStager.NormalizeRelativePath(entry.CookedRelativePath))).AppendLine("\"},");
        }
        source.AppendLine("};");
        source.Append("const char kStartupSceneId[] = \"").Append(Escape(startupSceneId)).AppendLine("\";");
        source.AppendLine("}");
        source.AppendLine("extern \"C\" const char* he_get_runtime_startup_scene_id() { return kStartupSceneId; }");
        source.AppendLine("extern \"C\" const HERuntimeSceneCatalogEntry* he_runtime_scene_catalog_entries(std::size_t* count) { if (count != nullptr) *count = sizeof(kEntries) / sizeof(kEntries[0]); return kEntries; }");
        string sourcePath = Path.Combine(root, "n64_runtime_scene_catalog.cpp");
        File.WriteAllText(sourcePath, source.ToString(), new UTF8Encoding(false));
        return headerPath;
    }

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
            if (artifact == null && sceneArtifacts.Length == 1) artifact = sceneArtifacts[0];
            if (artifact == null) throw new InvalidOperationException($"N64 startup scene '{scene.SceneId}' has no cooked scene artifact in the manifest.");
            return new Nintendo64StartupManifestEntry(scene.SceneId, artifact.RelativePath);
        }).ToArray();
        return Write(generatedCoreCppRootPath, manifest.StartupSceneId, entries);
    }

    static string Escape(string value) => value.Replace("\\", "\\\\", StringComparison.Ordinal).Replace("\"", "\\\"", StringComparison.Ordinal).Replace("\r", "\\r", StringComparison.Ordinal).Replace("\n", "\\n", StringComparison.Ordinal);
}
