using System.Diagnostics;

namespace helengine.n64.builder;

/// <summary>Invokes the repository's Docker build wrapper for authored mode.</summary>
public sealed class Nintendo64NativeBuildExecutor : INintendo64NativeBuildExecutor {
    public void Build(string generatedCoreCppRootPath, string stagedFilesystemRootPath, string outputRootPath, CancellationToken cancellationToken) {
        if (string.IsNullOrWhiteSpace(generatedCoreCppRootPath)) throw new ArgumentException("Generated Core root is required.", nameof(generatedCoreCppRootPath));
        if (string.IsNullOrWhiteSpace(stagedFilesystemRootPath)) throw new ArgumentException("Staged filesystem root is required.", nameof(stagedFilesystemRootPath));
        if (string.IsNullOrWhiteSpace(outputRootPath)) throw new ArgumentException("Output root is required.", nameof(outputRootPath));
        cancellationToken.ThrowIfCancellationRequested();
        string repositoryRoot = Environment.GetEnvironmentVariable("HELENGINE_N64_REPOSITORY_ROOT") ?? string.Empty;
        if (string.IsNullOrWhiteSpace(repositoryRoot) || !Directory.Exists(repositoryRoot)) throw new InvalidOperationException("HELENGINE_N64_REPOSITORY_ROOT must identify the N64 implementation checkout.");
        Directory.CreateDirectory(outputRootPath);
        ProcessStartInfo info = new() {
            FileName = "pwsh",
            WorkingDirectory = repositoryRoot,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        info.ArgumentList.Add("-NoProfile");
        info.ArgumentList.Add("-File");
        info.ArgumentList.Add(Path.Combine(repositoryRoot, "scripts", "build.ps1"));
        // The generated Core root and authored filesystem are build inputs, but
        // Make's object/dependency files live in the shared N64 checkout. A
        // clean native pass prevents a previous generated profile from leaving
        // stale header dependencies in helengine_core_unity.o.
        info.ArgumentList.Add("-Clean");
        info.ArgumentList.Add("-Mode");
        info.ArgumentList.Add("authored");
        info.ArgumentList.Add("-GeneratedCoreRoot");
        info.ArgumentList.Add(generatedCoreCppRootPath);
        info.ArgumentList.Add("-FilesystemRoot");
        info.ArgumentList.Add(stagedFilesystemRootPath);
        using Process process = Process.Start(info) ?? throw new InvalidOperationException("Could not start the N64 build wrapper.");
        Task<string> standardOutput = process.StandardOutput.ReadToEndAsync(cancellationToken);
        Task<string> standardError = process.StandardError.ReadToEndAsync(cancellationToken);
        process.WaitForExit();
        cancellationToken.ThrowIfCancellationRequested();
        string output = standardOutput.GetAwaiter().GetResult();
        string error = standardError.GetAwaiter().GetResult();
        if (process.ExitCode != 0) throw new InvalidOperationException($"N64 native build failed (exit {process.ExitCode}).{Environment.NewLine}{output}{Environment.NewLine}{error}");
        string romPath = Path.Combine(repositoryRoot, "helengine_n64.z64");
        if (!File.Exists(romPath)) throw new FileNotFoundException("N64 native build completed without producing a ROM.", romPath);
        File.Copy(romPath, Path.Combine(outputRootPath, "helengine_n64.z64"), true);
    }
}
