<#
.SYNOPSIS
Sets only the staged menu logo's N64 texture resolution limit.
.DESCRIPTION
Preserves importer identity, color/alpha/indexing settings and every other scope.
When no N64 override exists, copies the registered builder's effective defaults
before changing MaxResolution. The typed texture format uses platform keys rather
than scene group scopes. ValidateOnly round-trips the plan without writing files.
#>
param(
    [Parameter(Mandatory=$true)][string]$StagedProjectRoot,
    [Parameter(Mandatory=$true)][string]$PublisherRoot,
    [Parameter(Mandatory=$true)][string]$ReceiptPath,
    [ValidateRange(1,256)][int]$MaxResolution=256,
    [switch]$ValidateOnly
)
$ErrorActionPreference='Stop'
$buildRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../builds/helengine-n64'))
function Get-ContainedPath([string]$root,[string]$relative){
    $path=[IO.Path]::GetFullPath((Join-Path $root $relative))
    if(!$path.StartsWith($root.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Menu logo configuration requires a stage inside N64 builds.'}
    $cursor=$path
    while($cursor.Length -ge $root.Length){
        if((Test-Path -LiteralPath $cursor) -and (((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)){throw "Reparse path: $cursor"}
        if($cursor.Equals($root,[StringComparison]::OrdinalIgnoreCase)){break}
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
    return $path
}
$stage=Get-ContainedPath $buildRoot ([IO.Path]::GetRelativePath($buildRoot,[IO.Path]::GetFullPath($StagedProjectRoot)))
if(!(Test-Path -LiteralPath (Join-Path $stage 'project.heproj'))){throw 'An existing N64 project stage is required.'}
$settingsPath=Get-ContainedPath $stage 'assets/textures/menu/helengine-logo.png.hasset'
$receiptSafe=Get-ContainedPath $buildRoot ([IO.Path]::GetRelativePath($buildRoot,[IO.Path]::GetFullPath($ReceiptPath)))
$scenePath=Get-ContainedPath $stage 'assets/scenes/DemoDiscMainMenu.helen'
$fontPath=Get-ContainedPath $stage 'assets/fonts/DemoDiscBody.ttf.hasset'
$imagePath=Get-ContainedPath $stage 'assets/textures/menu/helengine-logo.png'
$beforeScene=(Get-FileHash -LiteralPath $scenePath).Hash
$beforeFont=(Get-FileHash -LiteralPath $fontPath).Hash
$beforeImage=(Get-FileHash -LiteralPath $imagePath).Hash
foreach($name in @('helengine.core.dll','helengine.files.dll','helengine.editor.dll','helengine.baseplatform.dll','helengine.n64.builder.dll')){[Reflection.Assembly]::LoadFrom((Join-Path $PublisherRoot $name))|Out-Null}
function Read-Settings([byte[]]$bytes){
    $stream=[IO.MemoryStream]::new($bytes,$false)
    try{return [helengine.editor.TextureAssetImportSettingsBinarySerializer]::Deserialize($stream)}finally{$stream.Dispose()}
}
function Write-Settings($settings){
    $stream=[IO.MemoryStream]::new()
    try{[helengine.editor.TextureAssetImportSettingsBinarySerializer]::Serialize($stream,$settings);return ,$stream.ToArray()}finally{$stream.Dispose()}
}
function Hash-Bytes([byte[]]$bytes){return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()}
$beforeBytes=[IO.File]::ReadAllBytes($settingsPath)
$settings=Read-Settings $beforeBytes
$canonicalBefore=Write-Settings $settings
$hadN64=$settings.Processor.Platforms.ContainsKey('n64')
if($hadN64){$n64=$settings.Processor.Platforms['n64']}else{
    $definition=[helengine.n64.builder.Nintendo64PlatformDefinitionFactory]::Create()
    $capability=@($definition.AssetCookCapabilities|Where-Object SourceAssetKind -eq 'texture')
    if($capability.Count -ne 1){throw 'Registered N64 builder must expose one texture cook capability.'}
    $n64=[helengine.n64.builder.Nintendo64TextureCookSettingsSerializer]::Deserialize($capability[0].DefaultSerializedPlatformSettings)
    $settings.Processor.Platforms.Add('n64',$n64)
}
$previousMax=$n64.MaxResolution;$color=$n64.ColorFormatId;$alpha=$n64.AlphaPrecision;$indexing=$n64.IndexingMethodId
if($settings.Processor.Environments.ContainsKey('n64')){
    foreach($environment in $settings.Processor.Environments['n64'].GetEnumerator()){
        if($environment.Value.MaxResolution -ne $MaxResolution){throw "N64 environment $($environment.Key) overrides the requested logo cap; preserving that scope requires review."}
    }
}
$n64.MaxResolution=$MaxResolution
$bytes=Write-Settings $settings;$roundTrip=Read-Settings $bytes
$actual=$roundTrip.Processor.Platforms['n64']
if($actual.MaxResolution -ne $MaxResolution -or $actual.ColorFormatId -ne $color -or $actual.AlphaPrecision -ne $alpha -or $actual.IndexingMethodId -ne $indexing){throw 'N64 logo settings did not round-trip with unchanged color/alpha/indexing.'}
# Restore the sole changed scalar (or remove the newly explicit default scope)
# to prove every other serialized setting equals the original document.
if($hadN64){$actual.MaxResolution=$previousMax}else{$roundTrip.Processor.Platforms.Remove('n64')|Out-Null}
if((Hash-Bytes (Write-Settings $roundTrip)) -ne (Hash-Bytes $canonicalBefore)){throw 'Importer, common or another scope changed.'}
$afterHash=Hash-Bytes $bytes
if(!$ValidateOnly -and (Hash-Bytes $beforeBytes) -ne $afterHash){[IO.File]::WriteAllBytes($settingsPath,$bytes)}
if((Get-FileHash -LiteralPath $scenePath).Hash -ne $beforeScene -or (Get-FileHash -LiteralPath $fontPath).Hash -ne $beforeFont -or (Get-FileHash -LiteralPath $imagePath).Hash -ne $beforeImage){throw 'Logo preparation changed scene, font settings or source pixels.'}
$receipt=[ordered]@{platform='n64';asset='textures/menu/helengine-logo.png';settingsPath=$settingsPath;maxResolution=$MaxResolution;previousMaxResolution=$previousMax;colorFormat=$color;alphaPrecision=$alpha.ToString();indexingMethod=$indexing;beforeSha256=(Hash-Bytes $beforeBytes);afterSha256=$afterHash;importerAndOtherScopesPreserved=$true;colorAndAlphaPreserved=$true;sceneSha256=$beforeScene.ToLowerInvariant();fontSettingsSha256=$beforeFont.ToLowerInvariant();sourceImageSha256=$beforeImage.ToLowerInvariant();sceneAndFontsUnchanged=$true;roundTripPassed=$true;expectedSourceAssetId=$settings.Importer.AssetId;cookedBindingMustBeResolvedFromPackagedScene=$true;nativeRuntimeFormat='RGBA16';expectedSurfaceBytes=($MaxResolution*$MaxResolution*2);validateOnly=[bool]$ValidateOnly}
if(!$ValidateOnly){
    if((Get-FileHash -LiteralPath $settingsPath).Hash.ToLowerInvariant() -ne $afterHash){throw 'Settings file differs from validated plan.'}
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($receiptSafe))|Out-Null
    $receipt|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $receiptSafe
}
$receipt|ConvertTo-Json -Depth 5
