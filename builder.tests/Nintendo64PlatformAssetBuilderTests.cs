using helengine.baseplatform.Builders;
using helengine.baseplatform.Manifest;
using helengine.baseplatform.Profiles;
using helengine.baseplatform.Reporting;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Targets;
using helengine.editor;
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
            [new PlatformCookProfile("release", "release", new PlatformCookProfileCapabilities("n64", "n64-standard", "pcm-buffered", "n64-scene-v1", PlatformSerializationEndianness.LittleEndian))],
            outputRoot,
            workingRoot,
            "release",
            "n64-standard",
            "default",
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
        Assert.False(nativeBuildExecutor.CpuProfile);
        Assert.NotNull(nativeBuildExecutor.StagedFilesystemRootPath);
        Assert.Equal("scene", File.ReadAllText(Path.Combine(nativeBuildExecutor.StagedFilesystemRootPath!, "cooked", "scenes", "cube_test.hasset")));
    }

    /// <summary>
    /// The editor keeps builder-owned cook outputs out of the manifest's cooked artifacts, so the builder has
    /// to carry them into staging itself. Staging only the manifest is what shipped a ROM whose sprite
    /// textures were missing from its filesystem, and the stager's own tests cannot catch that because they
    /// are handed the builder-cooked list directly.
    /// </summary>
    [Fact]
    public async Task BuildAsync_StagesTheTexturesItCookedEvenThoughTheManifestOmitsThem() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        string workingRoot = Path.Combine(root, "builder");
        string packageSourceRoot = Path.Combine(workingRoot, "package-source");
        string outputRoot = Path.Combine(root, "output");
        string generatedCoreRoot = Path.Combine(root, "generated-core");
        string cookedScenePath = Path.Combine(packageSourceRoot, "cooked", "scenes", "cube_test.hasset");
        Directory.CreateDirectory(Path.GetDirectoryName(cookedScenePath)!);
        File.WriteAllText(cookedScenePath, "scene");
        string sourceIconPath = Path.Combine(root, "icon.png");
        string sourceBadgePath = Path.Combine(root, "badge.png");
        Directory.CreateDirectory(root);
        WriteSourcePng(sourceIconPath, 8, 8);
        WriteSourcePng(sourceBadgePath, 4, 4);

        // Two work items, not one: with a single item a staging loop that only ever carried work item zero
        // would still pass, which is the bug this test exists to catch.
        const string cookedIconRelativePath = "cooked/imported/icon";
        const string cookedBadgeRelativePath = "cooked/imported/nested/badge";
        PlatformCookWorkItem iconWorkItem = CreateTextureWorkItem(sourceIconPath, cookedIconRelativePath);
        PlatformCookWorkItem badgeWorkItem = CreateTextureWorkItem(sourceBadgePath, cookedBadgeRelativePath);
        // The manifest declares only the scene, exactly as the editor emits it: the cooked icons are absent.
        PlatformBuildRequest request = CreateRequest(
            [new PlatformBuildArtifact("cooked/scenes/cube_test.hasset", "cube_test", "sha", "scene", "n64")],
            [iconWorkItem, badgeWorkItem],
            outputRoot,
            workingRoot,
            generatedCoreRoot);
        RecordingNativeBuildExecutor nativeBuildExecutor = new();

        PlatformBuildReport report = await new Nintendo64PlatformAssetBuilder(nativeBuildExecutor).BuildAsync(
            request,
            new NoOpProgressReporter(),
            new NoOpDiagnosticReporter(),
            CancellationToken.None);

        Assert.True(report.Succeeded, string.Join(Environment.NewLine, report.Diagnostics.Select(diagnostic => diagnostic.Message)));
        Assert.False(nativeBuildExecutor.CpuProfile);
        string stagedIconPath = Path.Combine(nativeBuildExecutor.StagedFilesystemRootPath!, "cooked", "imported", "icon");
        Assert.True(File.Exists(stagedIconPath), $"The cooked texture never reached the staging tree at '{stagedIconPath}'.");
        helengine.TextureAsset stagedIcon = Assert.IsType<helengine.TextureAsset>(
            helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(stagedIconPath)));
        Assert.Equal(8, stagedIcon.Width);
        Assert.Equal(8, stagedIcon.Height);
        Assert.Equal("runtime-texture:" + cookedIconRelativePath, stagedIcon.Id);

        string stagedBadgePath = Path.Combine(nativeBuildExecutor.StagedFilesystemRootPath!, "cooked", "imported", "nested", "badge");
        Assert.True(File.Exists(stagedBadgePath), $"The second cooked texture never reached the staging tree at '{stagedBadgePath}'.");
        helengine.TextureAsset stagedBadge = Assert.IsType<helengine.TextureAsset>(
            helengine.files.AssetSerializer.DeserializeFromBytes(File.ReadAllBytes(stagedBadgePath)));
        Assert.Equal(4, stagedBadge.Width);
        Assert.Equal(4, stagedBadge.Height);
        Assert.Equal("runtime-texture:" + cookedBadgeRelativePath, stagedBadge.Id);

        Assert.True(File.Exists(Path.Combine(nativeBuildExecutor.StagedFilesystemRootPath!, "cooked", "scenes", "cube_test.hasset")));
    }

    /// <summary>
    /// Builds one builder-owned N64 texture cook work item for the supplied source image and cooked output path.
    /// </summary>
    /// <param name="sourcePath">Absolute path of the source image to cook.</param>
    /// <param name="cookedRelativePath">Content-relative path the cooked texture is written to.</param>
    /// <returns>Platform cook work item the N64 builder can execute.</returns>
    static PlatformCookWorkItem CreateTextureWorkItem(string sourcePath, string cookedRelativePath) {
        return new PlatformCookWorkItem(
            "n64:texture:" + cookedRelativePath,
            sourcePath,
            "texture",
            "n64",
            "runtime-texture",
            cookedRelativePath,
            "runtime-texture:" + cookedRelativePath,
            "source-hash",
            "settings-hash",
            Nintendo64TextureCookSettingsSerializer.Serialize(new TextureAssetProcessorSettings {
                MaxResolution = 64,
                ColorFormat = TextureAssetColorFormat.Rgba32,
                AlphaPrecision = TextureAssetAlphaPrecision.A8
            }),
            []);
    }

    /// <summary>
    /// Builds one N64 build request around the supplied cooked artifacts and builder-owned cook work items.
    /// </summary>
    /// <param name="cookedArtifacts">Manifest-declared cooked artifacts.</param>
    /// <param name="platformCookWorkItems">Builder-owned platform cook work items.</param>
    /// <param name="outputRoot">Absolute build output root.</param>
    /// <param name="workingRoot">Absolute builder working root.</param>
    /// <param name="generatedCoreRoot">Absolute generated Core root.</param>
    /// <returns>Platform build request.</returns>
    static PlatformBuildRequest CreateRequest(
        PlatformBuildArtifact[] cookedArtifacts,
        PlatformCookWorkItem[] platformCookWorkItems,
        string outputRoot,
        string workingRoot,
        string generatedCoreRoot,
        string selectedBuildProfileId = "release",
        string selectedCodegenProfileId = "default") {
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
            cookedArtifacts,
            [],
            [],
            new PlatformContainerWritePlan("n64", []),
            platformCookWorkItems,
            PlatformBuildRuntimeFeatureManifest.Empty);
        return new PlatformBuildRequest(
            manifest,
            [new PlatformBuildTargetVariant("release", "n64", "n64", "release")],
            [new PlatformCookProfile("release", "release", new PlatformCookProfileCapabilities("n64", "n64-standard", "pcm-buffered", "n64-scene-v1", PlatformSerializationEndianness.LittleEndian))],
            outputRoot,
            workingRoot,
            selectedBuildProfileId,
            "n64-standard",
            selectedCodegenProfileId,
            new Dictionary<string, string>(),
            new Dictionary<string, string>(),
            new Dictionary<string, string>(),
            generatedCoreRoot,
            "release",
            "dragonfs",
            "release");
    }

    [Fact]
    public async Task BuildAsync_EnablesNativeCpuProfileForCpuProfileBuild() {
        string root = Path.Combine("C:\\dev\\helworks\\builds\\helengine-n64\\v02\\tests", Guid.NewGuid().ToString("N"));
        string workingRoot = Path.Combine(root, "builder");
        string packageSourceRoot = Path.Combine(workingRoot, "package-source");
        string cookedScenePath = Path.Combine(packageSourceRoot, "cooked", "scenes", "cube_test.hasset");
        Directory.CreateDirectory(Path.GetDirectoryName(cookedScenePath)!);
        File.WriteAllText(cookedScenePath, "scene");
        RecordingNativeBuildExecutor nativeBuildExecutor = new();
        PlatformBuildRequest request = CreateRequest(
            [new PlatformBuildArtifact("cooked/scenes/cube_test.hasset", "cube_test", "sha", "scene", "n64")],
            [], Path.Combine(root, "output"), workingRoot, Path.Combine(root, "generated-core"), "cpu-profile", "cpu-profile");

        PlatformBuildReport report = await new Nintendo64PlatformAssetBuilder(nativeBuildExecutor).BuildAsync(
            request, new NoOpProgressReporter(), new NoOpDiagnosticReporter(), CancellationToken.None);

        Assert.True(report.Succeeded, string.Join(Environment.NewLine, report.Diagnostics.Select(diagnostic => diagnostic.Message)));
        Assert.True(nativeBuildExecutor.CpuProfile);
    }

    /// <summary>
    /// Writes one opaque test PNG the builder-owned texture cook can decode.
    /// </summary>
    /// <param name="path">Absolute destination path.</param>
    /// <param name="width">Source width in pixels.</param>
    /// <param name="height">Source height in pixels.</param>
    static void WriteSourcePng(string path, int width, int height) {
        using System.Drawing.Bitmap bitmap = new(width, height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bitmap.SetPixel(x, y, System.Drawing.Color.FromArgb(255, 10, 20, 30));
            }
        }
        bitmap.Save(path, System.Drawing.Imaging.ImageFormat.Png);
    }

    sealed class RecordingNativeBuildExecutor : INintendo64NativeBuildExecutor {
        public string StagedFilesystemRootPath { get; private set; } = string.Empty;
        public bool CpuProfile { get; private set; }

        public void Build(string generatedCoreCppRootPath, string stagedFilesystemRootPath, string outputRootPath, string nativeObjectCacheRootPath, bool cpuProfile, CancellationToken cancellationToken) {
            CpuProfile = cpuProfile;
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
