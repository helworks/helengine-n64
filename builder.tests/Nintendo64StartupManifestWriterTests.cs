using helengine.n64.builder;
using helengine.baseplatform.Manifest;
using System.Globalization;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64StartupManifestWriterTests {
    [Fact]
    public void Write_EmitsEscapedCatalogAndStartupId() {
        string root = TestRoot("escaped-catalog");
        string header = new Nintendo64StartupManifestWriter().Write(
            root,
            "scene\"one",
            [new Nintendo64StartupManifestEntry("scene\"one", "scenes/cube.heasset")]);

        Assert.True(File.Exists(header));
        string source = File.ReadAllText(Path.Combine(root, "n64_runtime_scene_catalog.cpp"));
        Assert.Contains("scene\\\"one", source, StringComparison.Ordinal);
        Assert.Contains("scenes/cube.heasset", source, StringComparison.Ordinal);
        Assert.Contains("kSceneLoadingBudgetMilliseconds = 8;", source, StringComparison.Ordinal);
        Assert.Contains("kLoadingScreenSceneId[] = \"\";", source, StringComparison.Ordinal);
    }

    [Fact]
    public void Write_RejectsMissingStartupScene() {
        string root = TestRoot("missing-startup");
        Assert.Throws<InvalidOperationException>(() => new Nintendo64StartupManifestWriter().Write(
            root,
            "missing",
            [new Nintendo64StartupManifestEntry("scene", "scenes/cube.heasset")]));
    }

    /// <summary>The shared budget and distinct overlay survive catalog generation independently of the current locale.</summary>
    [Fact]
    public void Write_TransportsLoadingPolicyAndPreservesSceneMapping() {
        PlatformBuildManifest manifest = CreateManifest(["menu", "cube", "loading"], ["menu", "cube", "loading"]);
        manifest.SceneLoadingBudgetMilliseconds = 2.5d;
        manifest.LoadingScreenSceneId = "loading";
        string root = TestRoot("loading-policy");
        CultureInfo previousCulture = CultureInfo.CurrentCulture;
        try {
            CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("fr-FR");
            new Nintendo64StartupManifestWriter().Write(root, manifest);
        } finally {
            CultureInfo.CurrentCulture = previousCulture;
        }
        string source = File.ReadAllText(Path.Combine(root, "n64_runtime_scene_catalog.cpp"));
        string header = File.ReadAllText(Path.Combine(root, "n64_runtime_scene_catalog.hpp"));
        Assert.Contains("kSceneLoadingBudgetMilliseconds = 2.5;", source, StringComparison.Ordinal);
        Assert.Contains("kLoadingScreenSceneId[] = \"loading\";", source, StringComparison.Ordinal);
        Assert.Contains("kStartupSceneId[] = \"menu\";", source, StringComparison.Ordinal);
        foreach (string id in new[] { "menu", "cube", "loading" }) {
            Assert.Contains($"{{\"{id}\", \"cooked/{id}.hasset\"}}", source, StringComparison.Ordinal);
        }
        Assert.Contains("double he_get_runtime_scene_loading_budget_milliseconds();", header, StringComparison.Ordinal);
        Assert.Contains("const char* he_get_runtime_loading_screen_scene_id();", header, StringComparison.Ordinal);
    }

    /// <summary>An invalid budget must not replace the previous native catalog inputs.</summary>
    [Theory]
    [InlineData(0d)]
    [InlineData(-1d)]
    [InlineData(double.NaN)]
    [InlineData(double.PositiveInfinity)]
    [InlineData(double.NegativeInfinity)]
    public void Write_RejectsInvalidBudgetBeforeWriting(double budget) {
        PlatformBuildManifest manifest = CreateManifest(["menu"], ["menu"]);
        manifest.SceneLoadingBudgetMilliseconds = budget;
        AssertRejectedWithoutWriting(manifest, "invalid-budget");
    }

    /// <summary>Only a valid packaged overlay may be advertised to the startup host.</summary>
    [Theory]
    [InlineData("missing")]
    [InlineData(" ")]
    [InlineData(null)]
    public void Write_RejectsInvalidLoadingSceneBeforeWriting(string loadingSceneId) {
        PlatformBuildManifest manifest = CreateManifest(["menu", "loading"], ["menu", "loading"]);
        manifest.LoadingScreenSceneId = loadingSceneId;
        AssertRejectedWithoutWriting(manifest, "invalid-loading");
    }

    /// <summary>Booting the configured loading scene itself preserves the host's synchronous Single branch.</summary>
    [Fact]
    public void Write_AllowsLoadingSceneAsStartup() {
        PlatformBuildManifest manifest = CreateManifest(["menu"], ["menu"]);
        manifest.LoadingScreenSceneId = "menu";
        string root = TestRoot("loading-startup");
        new Nintendo64StartupManifestWriter().Write(root, manifest);
        string source = File.ReadAllText(Path.Combine(root, "n64_runtime_scene_catalog.cpp"));
        Assert.Contains("kStartupSceneId[] = \"menu\";", source, StringComparison.Ordinal);
        Assert.Contains("kLoadingScreenSceneId[] = \"menu\";", source, StringComparison.Ordinal);
    }

    /// <summary>A single cooked artifact cannot silently become the payload of several declared scenes.</summary>
    [Fact]
    public void Write_RejectsMissingPayloadInMultiSceneCatalog() {
        PlatformBuildManifest manifest = CreateManifest(["menu", "loading"], ["menu"]);
        manifest.LoadingScreenSceneId = "loading";
        AssertRejectedWithoutWriting(manifest, "missing-loading-payload");
    }

    /// <summary>The legacy fallback remains valid when there is exactly one scene and one cooked scene payload.</summary>
    [Fact]
    public void Write_PreservesLegacySingleSceneArtifactFallback() {
        PlatformBuildManifest manifest = CreateManifest(["menu"], ["legacy-id"]);
        string root = TestRoot("single-scene-fallback");
        new Nintendo64StartupManifestWriter().Write(root, manifest);
        Assert.Contains("{\"menu\", \"cooked/legacy-id.hasset\"}", File.ReadAllText(Path.Combine(root, "n64_runtime_scene_catalog.cpp")), StringComparison.Ordinal);
    }

    /// <summary>Repeated overlay identifiers are rejected instead of selecting an ambiguous scene.</summary>
    [Fact]
    public void Write_RejectsDuplicateLoadingSceneBeforeWriting() {
        PlatformBuildManifest manifest = CreateManifest(["menu", "loading", "loading"], ["menu", "loading"]);
        manifest.LoadingScreenSceneId = "loading";
        AssertRejectedWithoutWriting(manifest, "duplicate-loading");
    }

    /// <summary>Creates actual shared build metadata with stable cooked scene identities for writer validation.</summary>
    static PlatformBuildManifest CreateManifest(string[] sceneIds, string[] artifactIds) {
        return new PlatformBuildManifest(1, "project", "1.0.0", "1.0.0", "n64", "1.0.0", "menu",
            sceneIds.Select(id => new PlatformBuildScene(id, id, $"scenes/{id}.helen", [], [])).ToArray(), [],
            artifactIds.Select(id => new PlatformBuildArtifact($"cooked/{id}.hasset", id, "sha", "scene", "n64")).ToArray(),
            [], [], new PlatformContainerWritePlan("n64", []), [], PlatformBuildRuntimeFeatureManifest.Empty);
    }

    /// <summary>Asserts both native files remain byte-for-byte untouched when policy validation fails.</summary>
    static void AssertRejectedWithoutWriting(PlatformBuildManifest manifest, string fixtureName) {
        string root = TestRoot(fixtureName);
        Directory.CreateDirectory(root);
        string header = Path.Combine(root, "n64_runtime_scene_catalog.hpp");
        string source = Path.Combine(root, "n64_runtime_scene_catalog.cpp");
        File.WriteAllText(header, "previous header");
        File.WriteAllText(source, "previous source");
        Assert.Throws<InvalidOperationException>(() => new Nintendo64StartupManifestWriter().Write(root, manifest));
        Assert.Equal("previous header", File.ReadAllText(header));
        Assert.Equal("previous source", File.ReadAllText(source));
    }

    /// <summary>Uses a fixed visible test directory instead of creating a dated or temporary artifact history.</summary>
    static string TestRoot(string fixtureName) => Path.Combine("C:/dev/helworks/builds/helengine-n64/menu-20261007/loading-policy-tests/fixtures", fixtureName);
}
