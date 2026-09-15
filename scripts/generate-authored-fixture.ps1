param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,
    [Parameter(Mandatory = $true)]
    [string]$PackageRoot,
    [string]$GeneratedCoreRoot = ''
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generatorProject = Join-Path $repoRoot 'tools\helengine.n64.samplegen\helengine.n64.samplegen.csproj'
if (-not (Test-Path -LiteralPath $generatorProject -PathType Leaf)) {
    throw "Authored fixture generator is missing at $generatorProject"
}

$generatorArguments = @(
    'run',
    '--project', $generatorProject,
    '--no-restore',
    '--',
    [IO.Path]::GetFullPath($ProjectRoot),
    [IO.Path]::GetFullPath($PackageRoot)
)
if (-not [string]::IsNullOrWhiteSpace($GeneratedCoreRoot)) {
    $generatorArguments += [IO.Path]::GetFullPath($GeneratedCoreRoot)
}

& dotnet @generatorArguments
if ($LASTEXITCODE -ne 0) {
    throw "Authored fixture generation failed with exit code $LASTEXITCODE."
}
