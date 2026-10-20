param(
    [Parameter(Mandatory = $true)] [string]$OutputRoot,
    [string]$Compiler = 'g++'
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($OutputRoot)
$scratch = [IO.Path]::GetFullPath('C:\temp')
if ($output -ne $scratch -and -not $output.StartsWith($scratch + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Model-test outputs must be inside C:\temp.'
}
New-Item -ItemType Directory -Force -Path $output | Out-Null
$packingSource = Join-Path $repo 'src/platform/n64/Nintendo64VertexPacking.cpp'
$packingExecutable = Join-Path $output 'vertex-packing-tests.exe'
& $Compiler -std=c++17 -Wall -Wextra -Werror -pedantic `
    ("-I" + (Join-Path $repo 'tests/stubs')) ("-I" + $repo) `
    (Join-Path $repo 'tests/Nintendo64VertexPackingTests.cpp') $packingSource -o $packingExecutable
if ($LASTEXITCODE -ne 0) { throw "N64 vertex-packing compilation failed: $LASTEXITCODE" }
& $packingExecutable
if ($LASTEXITCODE -ne 0) { throw "N64 vertex-packing tests failed: $LASTEXITCODE" }

$modelExecutable = Join-Path $output 'runtime-model-tests.exe'
& $Compiler -std=c++17 -Wall -Wextra -Werror -pedantic -DHELENGINE_N64_HAS_GENERATED_CORE=1 `
    ("-I" + (Join-Path $repo 'tests/model_stubs')) ("-I" + (Join-Path $repo 'src')) `
    (Join-Path $repo 'tests/Nintendo64RuntimeModelTests.cpp') `
    (Join-Path $repo 'src/platform/n64/Nintendo64RuntimeModel.cpp') $packingSource -o $modelExecutable
if ($LASTEXITCODE -ne 0) { throw "N64 runtime-model compilation failed: $LASTEXITCODE" }
& $modelExecutable
if ($LASTEXITCODE -ne 0) { throw "N64 runtime-model tests failed: $LASTEXITCODE" }
