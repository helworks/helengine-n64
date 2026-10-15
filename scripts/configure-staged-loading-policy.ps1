<#
.SYNOPSIS
Stages the canonical persistent loading scene for the N64 build.
.DESCRIPTION
Validates its overlay camera through the engine serializer and preserves texture, font,
other-platform and project settings. A zero platform budget inherits the project budget.
#>
param(
 [Parameter(Mandatory=$true)][string]$StagedProjectRoot,
 [Parameter(Mandatory=$true)][string]$SourceProjectRoot,
 [Parameter(Mandatory=$true)][string]$PublisherRoot,
 [Parameter(Mandatory=$true)][string]$ReceiptPath,
 [switch]$ValidateOnly
)
$ErrorActionPreference='Stop'
$BuildRoot=[IO.Path]::GetFullPath('C:/dev/helworks/builds/helengine-n64')
$Stage=[IO.Path]::GetFullPath($StagedProjectRoot)
$Source=[IO.Path]::GetFullPath($SourceProjectRoot)
function Get-SafePath([string]$Root,[string]$Relative) {
 $Root=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
 $Path=[IO.Path]::GetFullPath((Join-Path $Root $Relative))
 if(!$Path.StartsWith($Root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Path escapes expected root.' }
 $Cursor=$Path
 while($Cursor.Length -ge $Root.Length) {
  if((Test-Path -LiteralPath $Cursor) -and ((Get-Item -LiteralPath $Cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Reparse paths are not supported.' }
  if($Cursor -eq $Root) { break }
  $Cursor=[IO.Path]::GetDirectoryName($Cursor)
 }
 return $Path
}
$null=Get-SafePath $BuildRoot ([IO.Path]::GetRelativePath($BuildRoot,$Stage))
$Receipt=Get-SafePath $BuildRoot ([IO.Path]::GetRelativePath($BuildRoot,[IO.Path]::GetFullPath($ReceiptPath)))
foreach($Name in @('helengine.core.dll','helengine.files.dll','helengine.editor.dll','AssimpNetter.dll')) {
 [Reflection.Assembly]::LoadFrom((Join-Path $PublisherRoot $Name)) | Out-Null
}
$env:PATH=(Join-Path $PublisherRoot 'runtimes/win-x64/native')+[IO.Path]::PathSeparator+$env:PATH
$Wrappers=[helengine.editor.ComponentPlatformOverridePayloadService]::new()
$ScenePath=Get-SafePath $Stage 'assets/scenes/SceneLoadingScreen.helen'
$SourceScene=Get-SafePath $Source 'assets/scenes/SceneLoadingScreen.helen'
$SourceHash=(Get-FileHash -LiteralPath $SourceScene).Hash
if($SourceHash -ne '9D3B27AE5579BAE1E04D45060AB4BC085F259B1357453B786114BF0180B8C502') { throw 'Unreviewed canonical loading scene.' }
if((Test-Path -LiteralPath $ScenePath) -and (Get-FileHash -LiteralPath $ScenePath).Hash -ne $SourceHash) { throw 'Unreviewed staged loading scene; refusing to replace it.' }
$SceneBytes=[IO.File]::ReadAllBytes($SourceScene)
$Stream=[IO.MemoryStream]::new($SceneBytes,$false)
try { $Scene=[helengine.files.AssetSerializer]::Deserialize($Stream) } finally { $Stream.Dispose() }
if(!$Scene.SceneSettings.DontUnload) { throw 'The authored loading scene is not persistent.' }
$Descriptor=[helengine.editor.AutomaticScriptComponentPersistenceDescriptor]::new([helengine.editor.ScriptComponentReflectionSchemaBuilder]::new())
$Cameras=0
function Validate-Camera($Entity) {
 foreach($Record in $Entity.Components) {
  $Base=$Wrappers.UnwrapBaseRecord($Record)
  if($Base.ComponentTypeId -ne 'helengine.CameraComponent') { continue }
  $script:Cameras++
  $Camera=$Descriptor.DeserializeComponent($Base,[helengine.EntitySaveComponent]::new(),$null)
  if($Camera.ClearSettings.ClearColorEnabled -or $Camera.CameraDrawOrder -ne 254) { throw 'Canonical loading camera must draw last without clearing the scene.' }
  if(@($Wrappers.ReadOverrideStates($Record)).Count -ne 0) { throw 'Unexpected canonical loading camera overrides.' }
 }
 foreach($Child in $Entity.Children) { Validate-Camera $Child }
}
foreach($Entity in $Scene.RootEntities) { Validate-Camera $Entity }
if($Cameras -ne 1) { throw 'Expected exactly one canonical loading camera.' }
$ConfigPath=Get-SafePath $Stage 'user_settings/build_config.json'
$Config=Get-Content -LiteralPath $ConfigPath -Raw | ConvertFrom-Json -AsHashtable
$Platform=@($Config.platforms | Where-Object platformId -eq 'n64')
if($Platform.Count -ne 1) { throw 'Expected one N64 build configuration.' }
$Platform=$Platform[0]
$OtherPlatforms=@($Config.platforms | Where-Object platformId -ne 'n64') | ConvertTo-Json -Depth 50 -Compress
$Catalog=[helengine.editor.EditorProjectSceneCatalogService]::new($Stage)
$LoadingReference=$Catalog.CreateSceneReference('SceneLoadingScreen')
$Reference=[ordered]@{sourceKind=[int]$LoadingReference.SourceKind;relativePath=$LoadingReference.RelativePath;providerId=$LoadingReference.ProviderId;assetId=$LoadingReference.AssetId;contentHash='sha256:'+$SourceHash.ToLowerInvariant()}
$Existing=@($Platform.selectedSceneReferences | Where-Object { $_.relativePath -cne $LoadingReference.RelativePath })
$Platform.selectedSceneReferences=@($Existing)+@($Reference)
$Platform.overridesProjectScenes=$true
$Platform.overridesLoadingScreenSceneId=$true
$Platform.loadingScreenSceneId='scenes/SceneLoadingScreen.helen'
if(!$Platform.ContainsKey('sceneLoadingBudgetMilliseconds')) { $Platform.sceneLoadingBudgetMilliseconds=0 }
if($OtherPlatforms -ne (@($Config.platforms | Where-Object platformId -ne 'n64') | ConvertTo-Json -Depth 50 -Compress)) { throw 'Another platform configuration changed.' }
$ComponentRelative='assets/codebase/menu/SceneLoadingScreenComponent.cs'
$ComponentSource=Get-SafePath $Source $ComponentRelative
$ComponentHash=(Get-FileHash -LiteralPath $ComponentSource).Hash
if($ComponentHash -ne 'BFD7697A9267F43F1D12ED98C3CDB55EDA2D34E4056C82948F951C71B9BB8C67') { throw 'Unreviewed loading component source.' }
$Plan=[ordered]@{platform='n64';canonicalSceneSha256=$SourceHash;canonicalComponentSha256=$ComponentHash;dontUnload=$true;cameraCount=$Cameras;loadingCameraDrawOrder=254;cameraColorClear=$false;otherPlatformSettingsPreserved=$true;canonicalScenePreservedByteExactly=$true;loadingAuthoredPath=$Platform.loadingScreenSceneId;loadingRuntimeSceneId='SceneLoadingScreen';budgetOverride=$Platform.sceneLoadingBudgetMilliseconds;validateOnly=[bool]$ValidateOnly}
if(!$ValidateOnly) {
 [IO.File]::WriteAllBytes($ScenePath,$SceneBytes)
 [IO.File]::WriteAllBytes((Get-SafePath $Stage $ComponentRelative),[IO.File]::ReadAllBytes($ComponentSource))
 $Config | ConvertTo-Json -Depth 50 | Set-Content -LiteralPath $ConfigPath -Encoding utf8
 $Settings=[helengine.editor.EditorRuntimeLoadingSettingsResolver]::Load($Stage,'n64')
 $Plan.effectiveBudgetMilliseconds=$Settings.SceneLoadingBudgetMilliseconds
 if((Get-FileHash -LiteralPath $SourceScene).Hash -ne $SourceHash -or (Get-FileHash -LiteralPath $ComponentSource).Hash -ne $ComponentHash) { throw 'Source changed during preparation.' }
 $Plan | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath $Receipt -Encoding utf8
}
$Plan | ConvertTo-Json -Depth 15
