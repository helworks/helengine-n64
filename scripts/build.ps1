
param(
    [switch]$Clean,
    [switch]$Debug,
    [switch]$CpuProfile,
    [ValidateSet('diagnostic', 'authored')]
    [string]$Mode = 'diagnostic',
    [string]$GeneratedCoreRoot = 'generated/core',
    [string]$FilesystemRoot = '',
    [string]$NativeObjectCacheRoot = '',
    [string]$OutputRoot = '',
    [switch]$UseExistingBuildImage
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$image = 'helengine-n64-build'
$buildPath = Join-Path $root 'build'
$romPath = Join-Path $root 'helengine_n64.z64'
$elfPath = Join-Path $buildPath 'helengine_n64.elf'
$mapPath = Join-Path $buildPath 'helengine_n64.map'
$nativeSourceRoot = $root
if (-not [string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
    $nativeSourceRoot = Join-Path $OutputRoot 'native-source'
    New-Item -ItemType Directory -Path $nativeSourceRoot -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'Makefile') -Destination $nativeSourceRoot -Force
    Copy-Item -LiteralPath (Join-Path $root 'src') -Destination $nativeSourceRoot -Recurse -Force
    $buildPath = Join-Path $nativeSourceRoot 'build'
    $romPath = Join-Path $nativeSourceRoot 'helengine_n64.z64'
}

function Get-TextSha256([string]$Text) {
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($algorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text)))).Replace('-', '').ToLowerInvariant()
    } finally {
        $algorithm.Dispose()
    }
}

function Get-RelativePath([string]$BasePath, [string]$TargetPath) {
    $baseFullPath = [IO.Path]::GetFullPath($BasePath).TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    $relativeUri = ([Uri]$baseFullPath).MakeRelativeUri([Uri][IO.Path]::GetFullPath($TargetPath))
    return [Uri]::UnescapeDataString($relativeUri.ToString()).Replace('/', [IO.Path]::DirectorySeparatorChar)
}

function Get-InputTreeFingerprint([string]$BasePath, [switch]$CompileInputsOnly) {
    $lines = @(
        foreach ($file in Get-ChildItem -LiteralPath $BasePath -File -Recurse | Sort-Object FullName) {
            if ($CompileInputsOnly -and $file.Extension -notin @('.cpp', '.hpp', '.h', '.inl')) { continue }
            $relative = (Get-RelativePath $BasePath $file.FullName).Replace('\', '/')
            "$relative=$((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant())"
        }
    )
    return [PSCustomObject]@{ Lines = $lines; Hash = (Get-TextSha256 ($lines -join "`n")) }
}

if ($CpuProfile -and $Mode -ne 'authored') {
    throw 'CPU profiling is available only for authored builds.'
}
if ($CpuProfile) {
    $profileCoreRoot = $GeneratedCoreRoot
    if (-not [IO.Path]::IsPathRooted($profileCoreRoot)) {
        $profileCoreRoot = Join-Path $root $profileCoreRoot
    }
    if (-not (Test-Path -LiteralPath $profileCoreRoot -PathType Container)) {
        throw "CPU profile generated Core root was not found: $profileCoreRoot"
    }
    foreach ($required in @('IRuntimeCpuProfileSink.hpp', 'RuntimeCpuProfileStage.hpp', 'CoreInitializationOptions.hpp')) {
        if (-not (Test-Path -LiteralPath (Join-Path $profileCoreRoot $required))) {
            throw "CPU profile build requires generated Core profiling header '$required'."
        }
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $root '.deps\libdragon'))) {
    throw 'Pinned dependencies are missing. Run scripts/setup-toolchain.ps1 first.'
}

if ($UseExistingBuildImage) {
    & docker image inspect $image --format '{{.Id}}'
    if ($LASTEXITCODE -ne 0) { throw 'Requested existing N64 build image is unavailable.' }
} else {
    & docker build -t $image $root
    if ($LASTEXITCODE -ne 0) { throw 'N64 build image creation failed.' }
}

$toolchainId = (& docker image inspect helengine-n64-toolchain --format '{{.Id}}').Trim()
$buildId = (& docker image inspect $image --format '{{.Id}}').Trim()
$libdragonPath = Join-Path $root '.deps/libdragon'
$tiny3dPath = Join-Path $root '.deps/tiny3d'
$libdragonRevision = (& git -c "safe.directory=$libdragonPath" -C $libdragonPath rev-parse HEAD).Trim()
$tiny3dRevision = (& git -c "safe.directory=$tiny3dPath" -C $tiny3dPath rev-parse HEAD).Trim()
$cacheInputs = @($toolchainId, $libdragonRevision, $tiny3dRevision, $Mode, [bool]$Debug, [bool]$CpuProfile) -join [Environment]::NewLine
$cacheInputs += [Environment]::NewLine + "Dockerfile=$((Get-FileHash -LiteralPath (Join-Path $root 'Dockerfile') -Algorithm SHA256).Hash.ToLowerInvariant())"
$cacheInputs += "`nMakefile=$((Get-FileHash -LiteralPath (Join-Path $root 'Makefile') -Algorithm SHA256).Hash.ToLowerInvariant())"
$cacheInputs += "`nbuild-script=$((Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash.ToLowerInvariant())"
if ($Mode -eq 'authored') {
    $resolvedGeneratedCoreForFingerprint = (Resolve-Path -LiteralPath $GeneratedCoreRoot).Path
    $generatedLayout = @(
        foreach ($inputFile in Get-ChildItem -LiteralPath $resolvedGeneratedCoreForFingerprint -File -Recurse | Sort-Object FullName) {
            if ($inputFile.Extension -in @('.cpp', '.hpp', '.h', '.inl')) {
                $relative = (Get-RelativePath $resolvedGeneratedCoreForFingerprint $inputFile.FullName).Replace('\', '/')
                "$relative=$((Get-FileHash -LiteralPath $inputFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant())"
            }
        }
    )
    $generatedCoreInputHash = Get-TextSha256 ($generatedLayout -join "`n")
    $cacheInputs += [Environment]::NewLine + "GeneratedCoreInputSha256=$generatedCoreInputHash"
}
$cacheFingerprint = Get-TextSha256 $cacheInputs
$buildDirectoryArgument = 'build'
if (-not [string]::IsNullOrWhiteSpace($NativeObjectCacheRoot)) {
    $NativeObjectCacheRoot = [IO.Path]::GetFullPath($NativeObjectCacheRoot)
    $repoPrefix = $root.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    if ($NativeObjectCacheRoot.Equals($root, [StringComparison]::OrdinalIgnoreCase) -or $NativeObjectCacheRoot.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "NativeObjectCacheRoot must be outside the source checkout: $NativeObjectCacheRoot"
    }
    $buildPath = Join-Path (Join-Path $NativeObjectCacheRoot 'n64') $cacheFingerprint
    New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
    $buildDirectoryArgument = '/n64-native-cache'
}
$elfPath = Join-Path $buildPath 'helengine_n64.elf'
$mapPath = Join-Path $buildPath 'helengine_n64.map'

$mount = "${nativeSourceRoot}:/workspace"
$makePrefix = @('run', '--rm', '-v', $mount)
$filesystemArgument = 'filesystem'
$generatedCoreArgument = $GeneratedCoreRoot
if ([System.IO.Path]::IsPathRooted($GeneratedCoreRoot)) {
    $resolvedGeneratedCoreRoot = (Resolve-Path -LiteralPath $GeneratedCoreRoot).Path
    if (-not (Test-Path -LiteralPath $resolvedGeneratedCoreRoot -PathType Container)) {
        throw "Generated Core root must be an existing directory: $GeneratedCoreRoot"
    }
    $makePrefix += @('-v', "${resolvedGeneratedCoreRoot}:/n64-generated-core")
    $generatedCoreArgument = '/n64-generated-core'
}
if (-not [string]::IsNullOrWhiteSpace($FilesystemRoot)) {
    $resolvedFilesystemRoot = (Resolve-Path -LiteralPath $FilesystemRoot).Path
    if (-not (Test-Path -LiteralPath $resolvedFilesystemRoot -PathType Container)) {
        throw "Filesystem root must be an existing directory: $FilesystemRoot"
    }
    $makePrefix += @('-v', "${resolvedFilesystemRoot}:/n64-staged-filesystem")
    $filesystemArgument = '/n64-staged-filesystem'
}
$buildDirArgument = "BUILD_DIR=$buildDirectoryArgument"
$filesystemBefore = if ($Mode -eq 'authored' -and -not [string]::IsNullOrWhiteSpace($FilesystemRoot)) {
    Get-InputTreeFingerprint $resolvedFilesystemRoot
} else { [PSCustomObject]@{ Lines = @(); Hash = '' } }
if (-not [string]::IsNullOrWhiteSpace($NativeObjectCacheRoot)) {
    $makePrefix += @('-v', "${buildPath}:/n64-native-cache")
}
$makePrefix += @('-w', '/workspace', $image, 'make')
if ($Clean) {
    & docker @($makePrefix + @($buildDirArgument, 'clean'))
    if ($LASTEXITCODE -ne 0) { throw 'N64 clean failed.' }
}
$makeArgs = @($makePrefix, $buildDirArgument, "HELENGINE_N64_MODE=$Mode", "HELENGINE_N64_GENERATED_CORE_ROOT=$generatedCoreArgument", "HELENGINE_N64_FILESYSTEM_ROOT=$filesystemArgument")
if ($Debug) { $makeArgs += 'D=1' }
if ($CpuProfile) { $makeArgs += 'N64_CPU_PROFILE=1' }
# DragonFS is package output and must reflect this invocation's fresh staged files.
$staleDfsPath = Join-Path $buildPath 'helengine_n64.dfs'
if (Test-Path -LiteralPath $staleDfsPath -PathType Leaf) { Remove-Item -LiteralPath $staleDfsPath -Force }
& docker @makeArgs
if ($LASTEXITCODE -ne 0) { throw 'N64 ROM build failed.' }

if ($Mode -eq 'authored') {
    $generatedCoreAfter = Get-InputTreeFingerprint $resolvedGeneratedCoreForFingerprint -CompileInputsOnly
    if ($generatedCoreAfter.Hash -ne $generatedCoreInputHash) { throw 'Generated Core inputs changed during native build; refusing success receipt.' }
    if ($filesystemBefore.Hash) {
        $filesystemAfter = Get-InputTreeFingerprint $resolvedFilesystemRoot
        if ($filesystemAfter.Hash -ne $filesystemBefore.Hash) { throw 'Cooked content changed during native build; refusing success receipt.' }
    }
}

foreach ($path in @($romPath, $elfPath, $mapPath)) {
    if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -le 0) {
        throw "Expected non-empty build artifact is missing: $path"
    }
}

$hash = (Get-FileHash -LiteralPath $romPath -Algorithm SHA256).Hash.ToLowerInvariant()
$manifest = Get-Content -LiteralPath (Join-Path $root 'dependencies.json') -Raw | ConvertFrom-Json
# The library pins live in the submodule gitlinks, so read them from git rather
# than from the manifest, which now records only the toolchain image.
$record = [ordered]@{
    generatedUtc = [DateTime]::UtcNow.ToString('o')
    command = "docker run ... make$(if ($Clean) { ' clean && make' })$(if ($Debug) { ' D=1' })$(if ($CpuProfile) { ' N64_CPU_PROFILE=1' })"
    cpuProfile = [bool]$CpuProfile
    generatedCoreInputSha256 = if ($Mode -eq 'authored') { $generatedCoreInputHash } else { '' }
    cookedContentInputSha256 = $filesystemBefore.Hash
    rom = [ordered]@{ path = 'helengine_n64.z64'; sha256 = $hash; bytes = (Get-Item -LiteralPath $romPath).Length }
    elf = [ordered]@{ path = 'build/helengine_n64.elf'; bytes = (Get-Item -LiteralPath $elfPath).Length }
    map = [ordered]@{ path = 'build/helengine_n64.map'; bytes = (Get-Item -LiteralPath $mapPath).Length }
    docker = [ordered]@{ toolchainImage = $toolchainId; buildImage = $buildId }
    dependencies = [ordered]@{
        toolchain = $manifest.toolchain
        libdragon = $libdragonRevision
        tiny3d = $tiny3dRevision
    }
    video = [ordered]@{ resolution = '320x240'; color = '16-bit'; buffers = 2; depth = '16-bit'; gamma = 'GAMMA_NONE'; filters = 'FILTERS_RESAMPLE'; aa = 'AA_NONE' }
}
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $buildPath 'build-record.json') -Encoding utf8
# Keep a success receipt beside the ROM as well as the object cache. Removing
# old object caches must not erase the identity of a preserved current ROM.
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath ($romPath + '.build.json') -Encoding utf8
if ($Mode -eq 'authored') { Set-Content -LiteralPath ($romPath + '.core-inputs.txt') -Value $generatedCoreAfter.Lines -Encoding utf8 }
Set-Content -LiteralPath ($romPath + '.content-inputs.txt') -Value $filesystemBefore.Lines -Encoding utf8
Write-Output "ROM SHA-256: $hash"
