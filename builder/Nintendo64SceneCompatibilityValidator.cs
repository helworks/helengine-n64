using helengine.baseplatform.Manifest;

namespace helengine.n64.builder;

/// <summary>Rejects build manifests outside the N64 v0.2 subset: one camera, one mesh, and authored sprite textures.</summary>
public sealed class Nintendo64SceneCompatibilityValidator {
    static readonly HashSet<string> SupportedFeatures = new(StringComparer.OrdinalIgnoreCase) { "scene", "mesh", "camera", "material", "texture", "sprite" };

    public void Validate(PlatformBuildManifest manifest) {
        if (manifest == null) throw new ArgumentNullException(nameof(manifest));
        if (manifest.Scenes.Length != 1) throw new InvalidOperationException($"N64 v0.2 requires exactly one authored scene; received {manifest.Scenes.Length}.");
        if (string.IsNullOrWhiteSpace(manifest.StartupSceneId) || !manifest.Scenes.Any(scene => scene.SceneId == manifest.StartupSceneId)) throw new InvalidOperationException($"N64 startup scene '{manifest.StartupSceneId}' is missing from the scene manifest.");
        foreach (PlatformBuildRequiredRuntimeFeature feature in manifest.RuntimeFeatureManifest.RequiredFeatures) {
            if (!SupportedFeatures.Contains(feature.FeatureId)) throw new InvalidOperationException($"N64 scene '{manifest.StartupSceneId}' requires unsupported runtime feature '{feature.FeatureId}' from '{feature.SourceId}'.");
        }
    }
}
