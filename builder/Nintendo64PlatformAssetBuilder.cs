using helengine.baseplatform.Builders;
using helengine.baseplatform.Definitions;
using helengine.baseplatform.Descriptors;
using helengine.baseplatform.Manifest;
using helengine.baseplatform.Reporting;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Results;

namespace helengine.n64.builder;

/// <summary>Platform discovery and build contract for the Nintendo 64 target.</summary>
public sealed class Nintendo64PlatformAssetBuilder : IPlatformAssetBuilder, IPlatformLayoutTargetProvider {
    /// <summary>Returns the framebuffer initialized by Nintendo64Video with libdragon's RESOLUTION_320x240.</summary>
    public PlatformLayoutTarget ResolveLayoutTarget(PlatformLayoutTargetRequest request) {
        ArgumentNullException.ThrowIfNull(request);
        return new PlatformLayoutTarget(320, 240, 1d, false, 0d, 1d);
    }
    readonly Nintendo64MaterialCooker MaterialCooker = new();

    /// <summary>Executes builder-owned platform cook work items declared by the manifest.</summary>
    readonly Nintendo64PlatformCookWorkItemExecutor CookWorkItemExecutor = new();

    readonly INintendo64NativeBuildExecutor NativeBuildExecutor;

    public Nintendo64PlatformAssetBuilder()
        : this(new Nintendo64NativeBuildExecutor()) {
    }

    public Nintendo64PlatformAssetBuilder(INintendo64NativeBuildExecutor nativeBuildExecutor) {
        NativeBuildExecutor = nativeBuildExecutor ?? throw new ArgumentNullException(nameof(nativeBuildExecutor));
        Descriptor = new PlatformBuilderDescriptor(
            "helengine.n64.builder",
            "0.2.0",
            "n64",
            new EngineCompatibilityRange("1.0.0", "999.0.0"),
            new ManifestCompatibilityRange(1, 1),
            ["n64"],
            ["n64"]);
        Definition = Nintendo64PlatformDefinitionFactory.Create();
    }

    public PlatformBuilderDescriptor Descriptor { get; }
    public PlatformDefinition Definition { get; }

    public PlatformMaterialCookResult CookMaterial(PlatformMaterialCookRequest request) => MaterialCooker.Cook(request);

    public async Task<PlatformBuildReport> BuildAsync(
        PlatformBuildRequest request,
        IPlatformBuildProgressReporter progressReporter,
        IPlatformBuildDiagnosticReporter diagnosticReporter,
        CancellationToken cancellationToken) {
        if (request == null) throw new ArgumentNullException(nameof(request));
        if (progressReporter == null) throw new ArgumentNullException(nameof(progressReporter));
        if (diagnosticReporter == null) throw new ArgumentNullException(nameof(diagnosticReporter));
        List<PlatformBuildDiagnostic> diagnostics = [];
        string stagingRoot = Path.Combine(request.WorkingRoot, "n64-dragonfs");
        string outputRomPath = Path.Combine(request.OutputRoot, "helengine_n64.z64");
        try {
            cancellationToken.ThrowIfCancellationRequested();
            Directory.CreateDirectory(request.OutputRoot);
            // Prevent a failed native invocation from being mistaken for a
            // successful build because an older ROM is still in the output tree.
            if (File.Exists(outputRomPath)) File.Delete(outputRomPath);
            if (!string.Equals(request.Manifest.PlatformName, "n64", StringComparison.OrdinalIgnoreCase)) {
                throw new InvalidOperationException($"N64 builder received manifest for platform '{request.Manifest.PlatformName}'.");
            }
            new Nintendo64SceneCompatibilityValidator().Validate(request.Manifest);
            string packageSourceRoot = ResolvePackageSourceRoot(request.WorkingRoot);

            PlatformCookWorkItem[] cookWorkItems = request.Manifest.PlatformCookWorkItems ?? [];
            // The editor leaves builder-owned outputs out of the manifest's cooked artifacts, so collect them here:
            // they still have to reach the DragonFS tree or the runtime cannot open its sprite textures.
            List<string> builderCookedRelativePaths = new(cookWorkItems.Length);
            for (int workItemIndex = 0; workItemIndex < cookWorkItems.Length; workItemIndex++) {
                cancellationToken.ThrowIfCancellationRequested();
                CookWorkItemExecutor.Execute(cookWorkItems[workItemIndex], packageSourceRoot, request.NumericPolicy);
                builderCookedRelativePaths.Add(cookWorkItems[workItemIndex].OutputRelativePath);
                progressReporter.Report(new PlatformBuildProgressUpdate(
                    "Cook Platform Artifacts",
                    cookWorkItems[workItemIndex].OutputLogicalArtifactId,
                    workItemIndex + 1,
                    cookWorkItems.Length,
                    $"Cooked platform-owned artifact '{cookWorkItems[workItemIndex].OutputRelativePath}'."));
            }

            progressReporter.Report(new PlatformBuildProgressUpdate("Stage DragonFS", "manifest", 0, 1, "Staging declared N64 cooked artifacts."));
            new Nintendo64ContentStager().Stage(request.Manifest.CookedArtifacts, builderCookedRelativePaths, packageSourceRoot, stagingRoot, cancellationToken);
            new Nintendo64StartupManifestWriter().Write(request.GeneratedCoreCppRootPath, request.Manifest);
            progressReporter.Report(new PlatformBuildProgressUpdate("Build Native ROM", "helengine_n64.z64", 0, 1, "Building authored N64 runtime."));
            bool cpuProfile = string.Equals(request.SelectedCodegenProfileId, Nintendo64PlatformDefinitionFactory.CpuProfileCodegenProfileId, StringComparison.Ordinal);
            NativeBuildExecutor.Build(request.GeneratedCoreCppRootPath, stagingRoot, request.OutputRoot, request.NativeObjectCacheRoot, cpuProfile, cancellationToken);
            progressReporter.Report(new PlatformBuildProgressUpdate("Build Native ROM", "helengine_n64.z64", 1, 1, "Built authored N64 runtime."));
        } catch (OperationCanceledException) {
            throw;
        } catch (Exception exception) {
            PlatformBuildDiagnostic diagnostic = new(
                PlatformBuildDiagnosticSeverity.Error,
                "N64BUILD_FAILED",
                exception.Message,
                request.Manifest.StartupSceneId,
                string.Empty,
                "n64-builder");
            diagnostics.Add(diagnostic);
            diagnosticReporter.Report(diagnostic);
        }

        bool succeeded = diagnostics.Count == 0 && File.Exists(outputRomPath);
        return new PlatformBuildReport(
            succeeded,
            [.. diagnostics],
            request.Manifest.Scenes.Select(scene => new PlatformBuildItemOutcome(scene.SceneId, succeeded ? PlatformBuildItemOutcomeKind.Succeeded : PlatformBuildItemOutcomeKind.Failed)).ToArray(),
            request.Manifest.LooseAssets.Select(asset => new PlatformBuildItemOutcome(asset.AssetId, succeeded ? PlatformBuildItemOutcomeKind.Succeeded : PlatformBuildItemOutcomeKind.Failed)).ToArray());
    }

    static string ResolvePackageSourceRoot(string workingRootPath) {
        if (string.IsNullOrWhiteSpace(workingRootPath)) {
            throw new ArgumentException("Builder working root is required.", nameof(workingRootPath));
        }

        string packageSourceRoot = Path.Combine(Path.GetFullPath(workingRootPath), "package-source");
        if (!Directory.Exists(packageSourceRoot)) {
            throw new DirectoryNotFoundException($"N64 builder package source root '{packageSourceRoot}' was not found.");
        }

        return packageSourceRoot;
    }
}
