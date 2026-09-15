using helengine.baseplatform.Builders;
using helengine.baseplatform.Manifest;
using helengine.baseplatform.Profiles;
using helengine.baseplatform.Reporting;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Targets;
using helengine.n64.builder;

namespace helengine.n64.builder.tests;

public sealed class Nintendo64PlatformAssetBuilderTests {
    [Fact]
    public async Task BuildAsync_StagesCookedArtifactsFromBuilderPackageSourceRoot() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        string workingRoot = Path.Combine(root, "builder");
        string packageSourceRoot = Path.Combine(workingRoot, "package-source");
        string outputRoot = Path.Combine(root, "output");
        string generatedCoreRoot = Path.Combine(root, "generated-core");
        string cookedScenePath = Path.Combine(packageSourceRoot, "cooked", "scenes", "cube_test.hasset");
        Directory.CreateDirectory(Path.GetDirectoryName(cookedScenePath)!);
        File.WriteAllText(cookedScenePath, "scene");

        PlatformBuildManifest manifest = new(
            1,
            "project",
            "1.0.0",
            "1.0.0",
            "n64",
            "1.0.0",
            "cube_test",
            [new PlatformBuildScene("cube_test", "Cube Test", "scenes/cube_test.helen", [], [])],
            [],
            [new PlatformBuildArtifact("cooked/scenes/cube_test.hasset", "cube_test", "sha", "scene", "n64")],
            [],
            [],
            new PlatformContainerWritePlan("n64", []),
            [],
            PlatformBuildRuntimeFeatureManifest.Empty);
        PlatformBuildRequest request = new(
            manifest,
            [new PlatformBuildTargetVariant("release", "n64", "n64", "release")],
            [new PlatformCookProfile("release", "release", new PlatformCookProfileCapabilities("n64", "n64-unlit", "pcm-buffered", "n64-scene-v1", PlatformSerializationEndianness.LittleEndian))],
            outputRoot,
            workingRoot,
            "release",
            "n64-unlit",
            "release",
            new Dictionary<string, string>(),
            new Dictionary<string, string>(),
            new Dictionary<string, string>(),
            generatedCoreRoot,
            "release",
            "dragonfs",
            "release");
        RecordingNativeBuildExecutor nativeBuildExecutor = new();

        PlatformBuildReport report = await new Nintendo64PlatformAssetBuilder(nativeBuildExecutor).BuildAsync(
            request,
            new NoOpProgressReporter(),
            new NoOpDiagnosticReporter(),
            CancellationToken.None);

        Assert.True(report.Succeeded, string.Join(Environment.NewLine, report.Diagnostics.Select(diagnostic => diagnostic.Message)));
        Assert.NotNull(nativeBuildExecutor.StagedFilesystemRootPath);
        Assert.Equal("scene", File.ReadAllText(Path.Combine(nativeBuildExecutor.StagedFilesystemRootPath!, "cooked", "scenes", "cube_test.hasset")));
    }

    sealed class RecordingNativeBuildExecutor : INintendo64NativeBuildExecutor {
        public string StagedFilesystemRootPath { get; private set; } = string.Empty;

        public void Build(string generatedCoreCppRootPath, string stagedFilesystemRootPath, string outputRootPath, CancellationToken cancellationToken) {
            StagedFilesystemRootPath = stagedFilesystemRootPath;
            Assert.True(File.Exists(Path.Combine(stagedFilesystemRootPath, "cooked", "scenes", "cube_test.hasset")));
            Directory.CreateDirectory(outputRootPath);
            File.WriteAllText(Path.Combine(outputRootPath, "helengine_n64.z64"), "rom");
        }
    }

    sealed class NoOpProgressReporter : IPlatformBuildProgressReporter {
        public void Report(PlatformBuildProgressUpdate update) { }
    }

    sealed class NoOpDiagnosticReporter : IPlatformBuildDiagnosticReporter {
        public void Report(PlatformBuildDiagnostic diagnostic) { }
    }
}
