param(
    [Parameter(Mandatory)] [string]$GeneratedCoreRoot
)

# Exercise the actual generated Stopwatch and configuration, not a copied runtime.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generated = (Resolve-Path -LiteralPath $GeneratedCoreRoot).Path
if (-not (Test-Path -LiteralPath (Join-Path $generated 'system/diagnostics/stopwatch.hpp'))) {
    throw 'Generated Core must contain the Stopwatch runtime.'
}
New-Item -ItemType Directory -Force -Path (Join-Path $root 'build') | Out-Null
& docker run --rm -v "${root}:/workspace" -v "${generated}:/generated:ro" -w /workspace `
    helengine-n64-build g++ -std=c++20 -Wall -Wextra -Werror -pedantic `
    -Itests/stubs -Isrc -I/generated tests/Nintendo64StopwatchTests.cpp -o build/n64-stopwatch-tests
if ($LASTEXITCODE -ne 0) { throw 'N64 stopwatch test compilation failed.' }
& docker run --rm -v "${root}:/workspace" -w /workspace helengine-n64-build ./build/n64-stopwatch-tests
if ($LASTEXITCODE -ne 0) { throw 'N64 stopwatch tests failed.' }
