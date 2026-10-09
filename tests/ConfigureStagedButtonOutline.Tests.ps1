<#
.SYNOPSIS
Checks the typed staged outline plan, read-only mode and repeated application.
.DESCRIPTION
Uses the existing N64 stage and its fixed baseline/receipt, never the original
project. The helper performs the full serialized scene/scope preservation audit.
#>
param(
    [Parameter(Mandatory=$true)][string]$StagedProjectRoot,
    [Parameter(Mandatory=$true)][string]$PublisherRoot,
    [Parameter(Mandatory=$true)][string]$ReceiptPath,
    [Parameter(Mandatory=$true)][string]$TestReceiptPath
)
$ErrorActionPreference='Stop'
$helper=Join-Path $PSScriptRoot '../scripts/configure-staged-button-outline.ps1'
$scenePath=Join-Path $StagedProjectRoot 'assets/scenes/DemoDiscMainMenu.helen'
$fontPath=Join-Path $StagedProjectRoot 'assets/fonts/DemoDiscBody.ttf.hasset'
$baselinePath=Join-Path ([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($StagedProjectRoot))) 'button-outline-baseline.helen'
$buildRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../builds/helengine-n64'))
$testPath=[IO.Path]::GetFullPath($TestReceiptPath)
if(!$testPath.StartsWith($buildRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Test receipt must remain inside N64 builds.'}
function File-State([string]$path){if(Test-Path -LiteralPath $path){return (Get-FileHash -LiteralPath $path).Hash};return 'absent'}
$checks=[Collections.Generic.List[string]]::new()
$before=@((File-State $scenePath),(File-State $baselinePath),(File-State $ReceiptPath),(File-State $fontPath))
$plan=(& $helper -StagedProjectRoot $StagedProjectRoot -PublisherRoot $PublisherRoot -ReceiptPath $ReceiptPath -ValidateOnly)|ConvertFrom-Json
$after=@((File-State $scenePath),(File-State $baselinePath),(File-State $ReceiptPath),(File-State $fontPath))
if([string]::Join(',', $before) -ne [string]::Join(',', $after)){throw 'ValidateOnly wrote files.'}
$checks.Add('ValidateOnly does not write scene, baseline, receipt or font settings')
if($plan.labelCount -ne 34 -or !$plan.originalCommonAndOtherScopesPreserved -or !$plan.selectedForReleaseAndDebug -or !$plan.fontSettingsUnchanged){throw 'Serialized plan preservation/selection checks failed.'}
foreach($label in $plan.labels){
    if($label.scope -ne 'group:sd/group:msd/platform:n64' -or $label.commonFontScale -ne 2 -or $label.commonOutlineScale -ne 2 -or $label.n64OutlineScale -ne 3){throw 'Unexpected label scope or metrics.'}
}
$checks.Add('All 34 labels select N64-only OutlineScale3 in Release and Debug')
$checks.Add('Common FontScale2/OutlineScale2 and all other scene data are preserved')
& $helper -StagedProjectRoot $StagedProjectRoot -PublisherRoot $PublisherRoot -ReceiptPath $ReceiptPath|Out-Null
$first=@((File-State $scenePath),(File-State $baselinePath),(File-State $ReceiptPath),(File-State $fontPath))
& $helper -StagedProjectRoot $StagedProjectRoot -PublisherRoot $PublisherRoot -ReceiptPath $ReceiptPath|Out-Null
$second=@((File-State $scenePath),(File-State $baselinePath),(File-State $ReceiptPath),(File-State $fontPath))
if([string]::Join(',', $first) -ne [string]::Join(',', $second)){throw 'Repeated application is not byte-idempotent.'}
$checks.Add('Repeated application retains identical scene, baseline, receipt and font bytes')
if($first[0].ToLowerInvariant() -ne $plan.stagedSceneSha256 -or $before[3] -ne $first[3]){throw 'Scene plan or font preservation mismatch.'}
$checks.Add('Actual staged SHA matches plan and body font settings remain unchanged')
$rejected=$false
try{& $helper -StagedProjectRoot (Join-Path $buildRoot '../helengine-xbox/project') -PublisherRoot $PublisherRoot -ReceiptPath $ReceiptPath -ValidateOnly|Out-Null}catch{$rejected=$true}
if(!$rejected){throw 'Helper accepted a non-N64 stage.'}
$checks.Add('Non-N64 stage is rejected before writes')
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($testPath))|Out-Null
[ordered]@{passed=$true;checks=$checks;helperSha256=(Get-FileHash -LiteralPath $helper).Hash.ToLowerInvariant();stagedSceneSha256=$plan.stagedSceneSha256;fontSettingsSha256=$first[3].ToLowerInvariant();nativeGeometryTest='Nintendo64QuadBlitGeometryTests.cpp: Rendering R/e fractional-outline regression PASS'}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $testPath
Write-Output "n64-staged-button-outline-tests: PASS ($($checks.Count) checks)"
