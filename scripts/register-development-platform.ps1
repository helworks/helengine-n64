param(
    [Parameter(Mandatory)] [string]$EngineUserSettingsRoot,
    [Parameter(Mandatory)] [string]$EngineVersion,
    [Parameter(Mandatory)] [string]$N64RepositoryRoot,
    [Parameter(Mandatory)] [string]$BuilderAssemblyPath,
    [Parameter(Mandatory)] [string]$GeneratedCoreCppRoot
)

$ErrorActionPreference = 'Stop'
foreach ($value in @($EngineUserSettingsRoot, $EngineVersion, $N64RepositoryRoot, $BuilderAssemblyPath, $GeneratedCoreCppRoot)) {
    if ([string]::IsNullOrWhiteSpace($value)) { throw 'All registration parameters are required.' }
}
if ($EngineVersion -match '^(0\.0\.0|1\.0\.0-placeholder|unknown)$') {
    throw "EngineVersion must come from the selected engine build; placeholder '$EngineVersion' is not allowed."
}
$repo = (Resolve-Path -LiteralPath $N64RepositoryRoot).Path
$builder = (Resolve-Path -LiteralPath $BuilderAssemblyPath).Path
$generated = (Resolve-Path -LiteralPath $GeneratedCoreCppRoot).Path
$settingsRoot = [IO.Path]::GetFullPath($EngineUserSettingsRoot)
if (-not (Test-Path -LiteralPath $repo -PathType Container)) { throw "N64 repository root does not exist: $repo" }
if (-not (Test-Path -LiteralPath $builder -PathType Leaf)) { throw "Builder assembly does not exist: $builder" }
if (-not (Test-Path -LiteralPath $generated -PathType Container)) { throw "Generated Core root does not exist: $generated" }

New-Item -ItemType Directory -Force -Path $settingsRoot | Out-Null
$manifestPath = Join-Path $settingsRoot 'platforms.json'
$document = if (Test-Path -LiteralPath $manifestPath) {
    Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
} else {
    [pscustomobject]@{ platforms = @() }
}
if ($null -eq $document.platforms) { $document | Add-Member -NotePropertyName platforms -NotePropertyValue @() }
$entries = @($document.platforms | Where-Object { $_.platformId -ne 'n64' })
$entries += [pscustomobject][ordered]@{
    engineVersion = $EngineVersion
    platformId = 'n64'
    displayName = 'Nintendo 64'
    builderAssemblyPath = $builder
    playerSourceRootPath = $repo
    generatedCoreCppRootPath = $generated
    codegenToolPath = ''
    pluginManifestPath = ''
}
$document.platforms = $entries
$tempPath = "$manifestPath.$([Guid]::NewGuid().ToString('N')).tmp"
$document | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $tempPath -Encoding UTF8
Move-Item -LiteralPath $tempPath -Destination $manifestPath -Force
Write-Output "Registered n64 in $manifestPath"
