namespace helengine.n64.builder;

/// <summary>Builds the native N64 image from generated Core and staged DragonFS inputs.</summary>
public interface INintendo64NativeBuildExecutor {
    void Build(string generatedCoreCppRootPath, string stagedFilesystemRootPath, string outputRootPath, CancellationToken cancellationToken);
}
