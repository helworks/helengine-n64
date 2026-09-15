
param(
    [switch]$Clean,
    [switch]$Debug,
    [ValidateSet('diagnostic', 'authored')]
    [string]$Mode = 'diagnostic',
    [string]$GeneratedCoreRoot = 'generated/core'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$image = 'helengine-n64-build'
$buildPath = Join-Path $root 'build'
$romPath = Join-Path $root 'helengine_n64.z64'
$elfPath = Join-Path $buildPath 'helengine_n64.elf'
$mapPath = Join-Path $buildPath 'helengine_n64.map'

if (-not (Test-Path -LiteralPath (Join-Path $root '.deps\libdragon'))) {
    throw 'Pinned dependencies are missing. Run scripts/setup-toolchain.ps1 first.'
}

& docker build -t $image $root
if ($LASTEXITCODE -ne 0) { throw 'N64 build image creation failed.' }

$mount = "${root}:/workspace"
$makePrefix = @('run', '--rm', '-v', $mount, '-w', '/workspace', $image, 'make')
if ($Clean) {
    & docker @($makePrefix + @('clean'))
    if ($LASTEXITCODE -ne 0) { throw 'N64 clean failed.' }
}
$makeArgs = @($makePrefix, "HELENGINE_N64_MODE=$Mode", "HELENGINE_N64_GENERATED_CORE_ROOT=$GeneratedCoreRoot")
if ($Debug) { $makeArgs += 'D=1' }
& docker @makeArgs
if ($LASTEXITCODE -ne 0) { throw 'N64 ROM build failed.' }

foreach ($path in @($romPath, $elfPath, $mapPath)) {
    if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -le 0) {
        throw "Expected non-empty build artifact is missing: $path"
    }
}

$hash = (Get-FileHash -LiteralPath $romPath -Algorithm SHA256).Hash.ToLowerInvariant()
$manifest = Get-Content -LiteralPath (Join-Path $root 'dependencies.json') -Raw | ConvertFrom-Json
$toolchainId = (& docker image inspect helengine-n64-toolchain --format '{{.Id}}').Trim()
$buildId = (& docker image inspect $image --format '{{.Id}}').Trim()
$record = [ordered]@{
    generatedUtc = [DateTime]::UtcNow.ToString('o')
    command = "docker run ... make$(if ($Clean) { ' clean && make' })$(if ($Debug) { ' D=1' })"
    rom = [ordered]@{ path = 'helengine_n64.z64'; sha256 = $hash; bytes = (Get-Item -LiteralPath $romPath).Length }
    elf = [ordered]@{ path = 'build/helengine_n64.elf'; bytes = (Get-Item -LiteralPath $elfPath).Length }
    map = [ordered]@{ path = 'build/helengine_n64.map'; bytes = (Get-Item -LiteralPath $mapPath).Length }
    docker = [ordered]@{ toolchainImage = $toolchainId; buildImage = $buildId }
    dependencies = $manifest
    video = [ordered]@{ resolution = '320x240'; color = '16-bit'; buffers = 2; depth = '16-bit'; gamma = 'GAMMA_NONE'; filters = 'FILTERS_RESAMPLE'; aa = 'AA_NONE' }
}
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $buildPath 'build-record.json') -Encoding utf8
Write-Output "ROM SHA-256: $hash"



