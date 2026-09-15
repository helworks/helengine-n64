param(
    [Parameter(Mandatory = $true)]
    [string]$GeneratedCoreRoot,
    [Parameter(Mandatory = $true)]
    [string]$FilesystemRoot,
    [string]$DfsPath = '',
    [string]$ExpectedScenePath = 'cooked/scenes/authored.hasset'
)

$ErrorActionPreference = 'Stop'

function Require-Condition {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "Authored-scene verification failed: $Message"
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$coreRoot = (Resolve-Path -LiteralPath $GeneratedCoreRoot).Path
$filesystemRoot = (Resolve-Path -LiteralPath $FilesystemRoot).Path

$unityPath = Join-Path $coreRoot 'helengine_core_unity.cpp'
$registrationPath = Join-Path $coreRoot 'GeneratedRuntimeComponentDeserializerRegistration.cpp'
$catalogPath = Join-Path $coreRoot 'n64_runtime_scene_catalog.cpp'
$loadServicePath = Join-Path $coreRoot 'RuntimeSceneLoadService.cpp'
$makefilePath = Join-Path $repoRoot 'Makefile'
$renderManagerPath = Join-Path $repoRoot 'src/platform/n64/Nintendo64RenderManager3D.cpp'
$sampleGeneratorPath = Join-Path $repoRoot 'tools\helengine.n64.samplegen\Program.cs'

foreach ($path in @($unityPath, $registrationPath, $catalogPath, $loadServicePath, $makefilePath, $renderManagerPath, $sampleGeneratorPath)) {
    Require-Condition (Test-Path -LiteralPath $path -PathType Leaf) "missing required source $path"
}

$unity = Get-Content -LiteralPath $unityPath -Raw
$registration = Get-Content -LiteralPath $registrationPath -Raw
$catalog = Get-Content -LiteralPath $catalogPath -Raw
$loadService = Get-Content -LiteralPath $loadServicePath -Raw
$makefile = Get-Content -LiteralPath $makefilePath -Raw
$renderManager = Get-Content -LiteralPath $renderManagerPath -Raw
$sampleGenerator = Get-Content -LiteralPath $sampleGeneratorPath -Raw

Require-Condition ($unity.Contains('#include "GeneratedRuntimeCameraComponentDeserializer.cpp"')) 'final unity source omits the generated camera deserializer'
Require-Condition ($unity.Contains('#include "GeneratedRuntimeMeshComponentDeserializer.cpp"')) 'final unity source omits the generated mesh deserializer'
Require-Condition (-not $unity.Contains('n64_runtime_scene_catalog.cpp')) 'final unity source embeds the separately linked scene catalog'
Require-Condition ($registration.Contains('Register(new ::GeneratedRuntimeCameraComponentDeserializer())')) 'component registry omits the camera deserializer'
Require-Condition ($registration.Contains('Register(new ::GeneratedRuntimeMeshComponentDeserializer())')) 'component registry omits the mesh deserializer'
Require-Condition ($catalog.Contains('"' + $ExpectedScenePath + '"')) "startup catalog does not map to $ExpectedScenePath"
Require-Condition (($sampleGenerator.Split('LayerMask = 0b00000001').Count - 1) -eq 2) 'sample generator must assign render layer 1 to camera and mesh entities'
Require-Condition ($loadService.Contains('set_LayerMask(entityAsset->LayerMask)')) 'generated scene loading does not preserve serialized entity layer masks'
Require-Condition ($makefile.Contains('$(HELENGINE_N64_GENERATED_CORE_ROOT)/helengine_core_unity.cpp')) 'Makefile does not select the final generated unity source'
Require-Condition ($renderManager.Contains('materialAsset->RendererFamilyId != kN64RendererFamilyId')) 'N64 material runtime does not validate the graphics profile renderer family'

$scenePath = Join-Path $filesystemRoot ($ExpectedScenePath -replace '/', '\')
Require-Condition (Test-Path -LiteralPath $scenePath -PathType Leaf) "staged scene is missing at $scenePath"
$sceneBytes = [System.IO.File]::ReadAllBytes($scenePath)
Require-Condition ($sceneBytes.Length -ge 32) "staged scene is only $($sceneBytes.Length) bytes"
Require-Condition ([System.Text.Encoding]::ASCII.GetString($sceneBytes, 0, 4) -eq 'HELE') 'staged scene is not a serialized Helengine asset'

if (-not [string]::IsNullOrWhiteSpace($DfsPath)) {
    $dfsFile = (Resolve-Path -LiteralPath $DfsPath).Path
    $dfsBytes = [System.IO.File]::ReadAllBytes($dfsFile)
    $dfsText = [System.Text.Encoding]::ASCII.GetString($dfsBytes)
    Require-Condition ($dfsText.Contains($ExpectedScenePath)) "DragonFS image omits $ExpectedScenePath"
}

Write-Output "authored-scene-source=PASS"
Write-Output "authored-scene-bytes=$($sceneBytes.Length)"
if (-not [string]::IsNullOrWhiteSpace($DfsPath)) {
    Write-Output "authored-scene-dfs=PASS"
}
