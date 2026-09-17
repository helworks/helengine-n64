[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$deps = Join-Path $root '.deps'
$libdragon = Join-Path $deps 'libdragon'
$tiny3d = Join-Path $deps 'tiny3d'
$manifestPath = Join-Path $root 'dependencies.json'

function Invoke-Native([string]$FilePath, [string[]]$Arguments, [string]$WorkingDirectory) {
    Push-Location $WorkingDirectory
    try {
        & $FilePath @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
        }
    } finally {
        Pop-Location
    }
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Force -Path $deps | Out-Null

function Assert-SubmoduleCheckout([string]$Path, [string]$Name) {
    if (-not (Test-Path -LiteralPath (Join-Path $Path '.git'))) {
        throw "Submodule '$Name' is not checked out at $Path. Run: git submodule update --init --recursive"
    }
    $dirty = (& git -C $Path status --porcelain | Out-String).Trim()
    if ($dirty -and -not $Force) {
        throw "Submodule checkout is locally modified; use -Force only after reviewing it: $Path"
    }
}

Assert-SubmoduleCheckout $libdragon 'libdragon'
Assert-SubmoduleCheckout $tiny3d 'tiny3d'

$baseImage = $manifest.toolchain.baseImage
if ($baseImage -notmatch '^ubuntu@sha256:[0-9a-f]{64}$') {
    throw "dependencies.json must contain an immutable Ubuntu base image digest."
}

$dockerArgs = @('build', '--build-arg', "BASE_IMAGE=$baseImage", '-t', 'helengine-n64-toolchain', (Join-Path $libdragon '.'))
& docker @dockerArgs
if ($LASTEXITCODE -ne 0) { throw 'N64 toolchain build failed.' }

$toolchainImageId = (& docker image inspect helengine-n64-toolchain --format '{{.Id}}').Trim()
$compilerVersion = (& docker run --rm helengine-n64-toolchain /n64_toolchain/bin/mips64-elf-g++ --version | Select-Object -First 1).Trim()
if (-not $toolchainImageId -or -not $compilerVersion) {
    throw 'Toolchain image identity or compiler version could not be recorded.'
}

$manifest.toolchain.toolchainImage = $toolchainImageId
$manifest.toolchain.compilerVersion = $compilerVersion
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8
Write-Output "toolchain image: $toolchainImageId"
Write-Output "compiler: $compilerVersion"


