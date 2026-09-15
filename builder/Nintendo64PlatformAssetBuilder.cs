using helengine.baseplatform.Builders;
using helengine.baseplatform.Definitions;
using helengine.baseplatform.Descriptors;
using helengine.baseplatform.Reporting;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Results;

namespace helengine.n64.builder;

/// <summary>Platform discovery and build contract for the Nintendo 64 target.</summary>
public sealed class Nintendo64PlatformAssetBuilder : IPlatformAssetBuilder {
    readonly Nintendo64MaterialCooker MaterialCooker = new();
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
            progressReporter.Report(new PlatformBuildProgressUpdate("Stage DragonFS", "manifest", 0, 1, "Staging declared N64 cooked artifacts."));
            new Nintendo64ContentStager().Stage(request.Manifest.CookedArtifacts, request.WorkingRoot, stagingRoot, cancellationToken);
            new Nintendo64StartupManifestWriter().Write(request.GeneratedCoreCppRootPath, request.Manifest);
            progressReporter.Report(new PlatformBuildProgressUpdate("Build Native ROM", "helengine_n64.z64", 0, 1, "Building authored N64 runtime."));
            NativeBuildExecutor.Build(request.GeneratedCoreCppRootPath, stagingRoot, request.OutputRoot, cancellationToken);
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
}
