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

    public Nintendo64PlatformAssetBuilder() {
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

    public Task<PlatformBuildReport> BuildAsync(
        PlatformBuildRequest request,
        IPlatformBuildProgressReporter progressReporter,
        IPlatformBuildDiagnosticReporter diagnosticReporter,
        CancellationToken cancellationToken) {
        if (request == null) throw new ArgumentNullException(nameof(request));
        if (progressReporter == null) throw new ArgumentNullException(nameof(progressReporter));
        if (diagnosticReporter == null) throw new ArgumentNullException(nameof(diagnosticReporter));
        PlatformBuildDiagnostic diagnostic = new(
            PlatformBuildDiagnosticSeverity.Error,
            "N64BUILD_NOT_IMPLEMENTED",
            "N64 generated-runtime packaging is not available in this bootstrap checkpoint.",
            request.Manifest.StartupSceneId,
            string.Empty,
            "n64-builder");
        diagnosticReporter.Report(diagnostic);
        return Task.FromResult(new PlatformBuildReport(
            false,
            [diagnostic],
            request.Manifest.Scenes.Select(scene => new PlatformBuildItemOutcome(scene.SceneId, PlatformBuildItemOutcomeKind.Failed)).ToArray(),
            request.Manifest.LooseAssets.Select(asset => new PlatformBuildItemOutcome(asset.AssetId, PlatformBuildItemOutcomeKind.Failed)).ToArray()));
    }
}
