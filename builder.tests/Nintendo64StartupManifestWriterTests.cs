using helengine.n64.builder;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64StartupManifestWriterTests {
    [Fact]
    public void Write_EmitsEscapedCatalogAndStartupId() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        string header = new Nintendo64StartupManifestWriter().Write(
            root,
            "scene\"one",
            [new Nintendo64StartupManifestEntry("scene\"one", "scenes/cube.heasset")]);

        Assert.True(File.Exists(header));
        string source = File.ReadAllText(Path.Combine(root, "n64_runtime_scene_catalog.cpp"));
        Assert.Contains("scene\\\"one", source, StringComparison.Ordinal);
        Assert.Contains("scenes/cube.heasset", source, StringComparison.Ordinal);
    }

    [Fact]
    public void Write_RejectsMissingStartupScene() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        Assert.Throws<InvalidOperationException>(() => new Nintendo64StartupManifestWriter().Write(
            root,
            "missing",
            [new Nintendo64StartupManifestEntry("scene", "scenes/cube.heasset")]));
    }
}
