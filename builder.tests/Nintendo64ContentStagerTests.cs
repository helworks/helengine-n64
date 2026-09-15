using helengine.baseplatform.Manifest;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64ContentStagerTests {
    [Fact]
    public void Stage_CopiesDeclaredArtifactsAndRejectsTraversal() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        string sourceRoot = Path.Combine(root, "source");
        string stagingRoot = Path.Combine(root, "staged");
        Directory.CreateDirectory(sourceRoot);
        File.WriteAllText(Path.Combine(sourceRoot, "scene.heasset"), "scene");
        PlatformBuildArtifact artifact = new("scene.heasset", "scene", "sha", "scene", "n64");

        IReadOnlyList<string> staged = new Nintendo64ContentStager().Stage([artifact], sourceRoot, stagingRoot, CancellationToken.None);

        Assert.Single(staged);
        Assert.Equal("scene", File.ReadAllText(Path.Combine(stagingRoot, "scene.heasset")));
        PlatformBuildArtifact escaping = new("../outside.heasset", "outside", "sha", "scene", "n64");
        Assert.Throws<InvalidOperationException>(() => new Nintendo64ContentStager().Stage([escaping], sourceRoot, stagingRoot, CancellationToken.None));
    }

    [Fact]
    public void Stage_RejectsCaseOnlyCollisions() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        string sourceRoot = Path.Combine(root, "source");
        string stagingRoot = Path.Combine(root, "staged");
        Directory.CreateDirectory(sourceRoot);
        File.WriteAllText(Path.Combine(sourceRoot, "a"), "a");
        File.WriteAllText(Path.Combine(sourceRoot, "b"), "b");
        PlatformBuildArtifact first = new("Assets/Cube.heasset", "first", "sha1", "scene", "n64");
        PlatformBuildArtifact second = new("assets/cube.heasset", "second", "sha2", "scene", "n64");
        Directory.CreateDirectory(Path.Combine(sourceRoot, "Assets"));
        Directory.CreateDirectory(Path.Combine(sourceRoot, "assets"));
        File.WriteAllText(Path.Combine(sourceRoot, "Assets", "Cube.heasset"), "a");
        File.WriteAllText(Path.Combine(sourceRoot, "assets", "cube.heasset"), "b");

        Assert.Throws<InvalidOperationException>(() => new Nintendo64ContentStager().Stage([first, second], sourceRoot, stagingRoot, CancellationToken.None));
    }
}
