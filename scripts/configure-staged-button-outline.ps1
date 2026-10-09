<#
.SYNOPSIS
Authors an N64-only one-pixel outline for staged menu button labels.
.DESCRIPTION
Retains the immutable common scene and other platform scopes. Only OutlineScale
on item-label-* TextComponents receives the effective group/platform override:
three reference pixels resolve to one pixel at the 320x240 menu viewport.
ValidateOnly validates the complete serialized plan without writing any file.
#>
param(
    [Parameter(Mandatory=$true)][string]$StagedProjectRoot,
    [Parameter(Mandatory=$true)][string]$PublisherRoot,
    [Parameter(Mandatory=$true)][string]$ReceiptPath,
    [switch]$ValidateOnly
)
$ErrorActionPreference='Stop'
$buildRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../builds/helengine-n64'))
$stageRoot=[IO.Path]::GetFullPath($StagedProjectRoot)

# Validate containment and every existing ancestor before reading or writing.
function Get-ContainedPath([string]$root,[string]$relative){
    $path=[IO.Path]::GetFullPath((Join-Path $root $relative))
    if(!$path.StartsWith($root.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw "Path escapes N64 builds: $path"}
    $cursor=$path
    while($cursor.Length -ge $root.Length){
        if((Test-Path -LiteralPath $cursor) -and (((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)){throw "Reparse path: $cursor"}
        if($cursor.Equals($root,[StringComparison]::OrdinalIgnoreCase)){break}
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
    return $path
}
$stageRoot=Get-ContainedPath $buildRoot ([IO.Path]::GetRelativePath($buildRoot,$stageRoot))
if(!(Test-Path -LiteralPath (Join-Path $stageRoot 'project.heproj'))){throw 'An existing N64 project stage is required.'}
$receiptSafe=Get-ContainedPath $buildRoot ([IO.Path]::GetRelativePath($buildRoot,[IO.Path]::GetFullPath($ReceiptPath)))
$baselinePath=Get-ContainedPath $buildRoot ([IO.Path]::GetRelativePath($buildRoot,(Join-Path ([IO.Path]::GetDirectoryName($stageRoot)) 'button-outline-baseline.helen')))
$scenePath=Get-ContainedPath $stageRoot 'assets/scenes/DemoDiscMainMenu.helen'
$fontPath=Get-ContainedPath $stageRoot 'assets/fonts/DemoDiscBody.ttf.hasset'
# The authored snapshot guard is deliberately independent of previous receipts.
$originalHash='c32d9d0befd95a1c717b4d75c7775495555b7366641e945ea12994770dab3201'
function Hash-Bytes([byte[]]$bytes){return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()}
$currentBytes=[IO.File]::ReadAllBytes($scenePath)
$baselineBytes=if(Test-Path -LiteralPath $baselinePath){[IO.File]::ReadAllBytes($baselinePath)}else{$currentBytes}
if((Hash-Bytes $baselineBytes) -ne $originalHash){throw 'Immutable menu baseline does not match the authored C32 snapshot.'}
$fontBefore=(Get-FileHash -LiteralPath $fontPath).Hash.ToLowerInvariant()
foreach($name in @('helengine.core.dll','helengine.files.dll','helengine.editor.dll')){[Reflection.Assembly]::LoadFrom((Join-Path $PublisherRoot $name))|Out-Null}
$resolver=[helengine.editor.EditorOverrideScopeResolver]::Load($stageRoot)
$wrappers=[helengine.editor.ComponentPlatformOverridePayloadService]::new()

# Retain tagged field order and unknown fields while replacing one scalar.
function Read-Fields([byte[]]$payload){
    if($payload.Length -lt 5 -or $payload[0] -ne 1){throw 'Unsupported tagged component payload.'}
    $fields=[Collections.Generic.Dictionary[string,byte[]]]::new([StringComparer]::Ordinal)
    $position=5;$count=[BitConverter]::ToInt32($payload,1)
    if($count -lt 0){throw 'Negative field count.'}
    for($index=0;$index -lt $count;$index++){
        if($position+4 -gt $payload.Length){throw 'Truncated field name.'}
        $length=[BitConverter]::ToInt32($payload,$position);$position+=4
        if($length -lt 1 -or $position+$length+4 -gt $payload.Length){throw 'Invalid field name.'}
        $name=[Text.Encoding]::UTF8.GetString($payload,$position,$length);$position+=$length
        $length=[BitConverter]::ToInt32($payload,$position);$position+=4
        if($length -lt 0 -or $position+$length -gt $payload.Length){throw 'Invalid field length.'}
        $bytes=[byte[]]::new($length);[Array]::Copy($payload,$position,$bytes,0,$length);$position+=$length
        $fields.Add($name,$bytes)
    }
    if($position -ne $payload.Length){throw 'Unexpected component tail.'}
    return ,$fields
}
function Write-Fields($fields){
    $stream=[IO.MemoryStream]::new();$writer=[IO.BinaryWriter]::new($stream)
    try{
        $writer.Write([byte]1);$writer.Write([int]$fields.Count)
        foreach($field in $fields.GetEnumerator()){
            $name=[Text.Encoding]::UTF8.GetBytes($field.Key);$writer.Write([int]$name.Length);$writer.Write($name)
            $writer.Write([int]$field.Value.Length);$writer.Write([byte[]]$field.Value)
        }
        $writer.Flush();return ,$stream.ToArray()
    }finally{$writer.Dispose();$stream.Dispose()}
}
function Read-Scene([byte[]]$bytes){
    $stream=[IO.MemoryStream]::new($bytes,$false)
    try{return [helengine.files.AssetSerializer]::Deserialize($stream)}finally{$stream.Dispose()}
}
function Write-Scene($scene){
    $stream=[IO.MemoryStream]::new()
    try{[helengine.files.AssetSerializer]::Serialize($stream,$scene);return ,$stream.ToArray()}finally{$stream.Dispose()}
}
function Resolve-LevelOrder($entity,$inherited){
    if($entity.HasOverrideLevelOrder){return ,[helengine.SceneOverrideScopeStepKind[]]$resolver.ResolveLevelOrder($entity.OverrideLevelOrder)}
    return ,[helengine.SceneOverrideScopeStepKind[]]$resolver.ResolveLevelOrder($inherited)
}
function Resolve-N64Scope($order){
    $scope=$resolver.BuildTargetPath($order,'n64',$null);$platform=$null
    if(!$scope.TryGetStepId([helengine.SceneOverrideScopeStepKind]::Platform,[ref]$platform) -or $platform -ne 'n64'){throw 'N64 scope must include the platform before BuildConfig.'}
    foreach($configuration in @('release','debug')){if(!$scope.IsPrefixOf($resolver.BuildTargetPath($order,'n64',$configuration))){throw "N64 scope is not selected for $configuration."}}
    return $scope
}
$scene=Read-Scene $baselineBytes
if($scene.SceneSettings.CanvasProfile.Width -ne 1280 -or $scene.SceneSettings.CanvasProfile.Height -ne 720){throw 'Unexpected authored canvas.'}
$labels=[Collections.Generic.List[object]]::new()
$replaced=[Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
function Configure-Labels($entities,$inherited){
    foreach($entity in $entities){
        $order=Resolve-LevelOrder $entity $inherited
        if($entity.Name.StartsWith('item-label-',[StringComparison]::Ordinal)){
            $scope=Resolve-N64Scope $order;$found=0
            for($index=0;$index -lt $entity.Components.Length;$index++){
                $record=$entity.Components[$index]
                if($record.ComponentTypeId -ne 'helengine.TextComponent'){continue}
                $found++;$base=$wrappers.UnwrapBaseRecord($record);$baseFields=Read-Fields $base.Payload
                if([BitConverter]::ToSingle($baseFields['OutlineScale']) -ne 2 -or [BitConverter]::ToSingle($baseFields['FontScale']) -ne 2){throw "Unexpected common label metrics: $($entity.Name)"}
                $state=[helengine.EntityComponentSaveState]::new();$state.ComponentKey=$record.ComponentKey
                foreach($existing in $wrappers.ReadOverrideStates($record)){$state.SetScopedPlatformOverride($existing.Scope,$existing)}
                $override=$state.GetOrCreateScopedPlatformOverride($scope)
                $fields=if($override.Payload.Length -gt 0){Read-Fields $override.Payload}else{Read-Fields $base.Payload}
                $fields['OutlineScale']=[BitConverter]::GetBytes([single]3)
                $override.SetPropertyOverride('OutlineScale')
                $stale=[Collections.Generic.List[string]]::new()
                foreach($member in $override.EnumerateMemberValues().GetEnumerator()){
                    if($member.Key -eq 'OutlineScale' -or $member.Key.StartsWith('OutlineScale.',[StringComparison]::Ordinal)){$stale.Add($member.Key)}
                }
                foreach($member in $stale){$override.RemoveMemberValue($member)}
                $override.Payload=Write-Fields $fields
                $entity.Components[$index]=$wrappers.Wrap($base,$state)
                $key=$entity.Name+'|'+$record.ComponentKey
                $replaced.Add($key,$record)
                $labels.Add([ordered]@{entity=$entity.Name;componentKey=$record.ComponentKey;scope=$scope.ToString();commonFontScale=2;commonOutlineScale=2;n64OutlineScale=3;expectedRuntimeOutlineScale=1})
            }
            if($found -ne 1){throw "Expected one text component on $($entity.Name)."}
        }
        Configure-Labels $entity.Children $order
    }
}
Configure-Labels $scene.RootEntities $null
if($labels.Count -eq 0){throw 'No authored button labels found.'}
$plannedBytes=Write-Scene $scene;$plannedHash=Hash-Bytes $plannedBytes
if((Hash-Bytes $currentBytes) -notin @($originalHash,$plannedHash)){throw 'Staged menu is neither the immutable baseline nor this exact outline plan.'}
$roundTrip=Read-Scene $plannedBytes

# Validate selected serialized payloads, then restore only the changed records.
# Comparing the entire remaining scene proves all other authored data untouched.
function Verify-AndRestore($entities,$inherited){
    foreach($entity in $entities){
        $order=Resolve-LevelOrder $entity $inherited
        for($index=0;$index -lt $entity.Components.Length;$index++){
            $record=$entity.Components[$index];$key=$entity.Name+'|'+$record.ComponentKey
            if(!$replaced.ContainsKey($key)){continue}
            $original=$replaced[$key];$scope=Resolve-N64Scope $order
            $states=@($wrappers.ReadOverrideStates($record));$originalStates=@($wrappers.ReadOverrideStates($original))
            $originalState=[helengine.EntityComponentSaveState]::new();$currentState=[helengine.EntityComponentSaveState]::new()
            foreach($candidate in $originalStates){if(!$candidate.Scope.Equals($scope)){$originalState.SetScopedPlatformOverride($candidate.Scope,$candidate)}}
            foreach($candidate in $states){if(!$candidate.Scope.Equals($scope)){$currentState.SetScopedPlatformOverride($candidate.Scope,$candidate)}}
            $a=$wrappers.Wrap($wrappers.UnwrapBaseRecord($original),$originalState).Payload
            $b=$wrappers.Wrap($wrappers.UnwrapBaseRecord($record),$currentState).Payload
            if([Convert]::ToBase64String($a) -ne [Convert]::ToBase64String($b)){throw 'Common payload or another scope changed.'}
            foreach($configuration in @('release','debug')){
                $target=$resolver.BuildTargetPath($order,'n64',$configuration);$selected=$null
                foreach($candidate in $states){if($candidate.Scope.IsPrefixOf($target) -and ($null -eq $selected -or $candidate.Scope.Depth -gt $selected.Scope.Depth)){$selected=$candidate}}
                if($null -eq $selected -or !$selected.Scope.Equals($scope) -or !$selected.HasPropertyOverride('OutlineScale')){throw "Outline override is not selected for $configuration."}
                $actual=Read-Fields $selected.Payload
                if([BitConverter]::ToSingle($actual['OutlineScale']) -ne 3){throw 'Serialized outline is not three reference pixels.'}
                $originalSelected=$null
                foreach($candidate in $originalStates){if($candidate.Scope.Equals($scope)){$originalSelected=$candidate}}
                $expected=if($null -ne $originalSelected -and $originalSelected.Payload.Length -gt 0){Read-Fields $originalSelected.Payload}else{Read-Fields ($wrappers.UnwrapBaseRecord($original).Payload)}
                if($actual.Count -ne $expected.Count){throw 'Label payload field count changed.'}
                foreach($field in $actual.GetEnumerator()){
                    if($field.Key -eq 'OutlineScale'){continue}
                    if(!$expected.ContainsKey($field.Key) -or [Convert]::ToBase64String($field.Value) -ne [Convert]::ToBase64String($expected[$field.Key])){throw "Another label property changed: $($field.Key)"}
                }
            }
            $entity.Components[$index]=$original
        }
        Verify-AndRestore $entity.Children $order
    }
}
Verify-AndRestore $roundTrip.RootEntities $null
if((Hash-Bytes (Write-Scene $roundTrip)) -ne (Hash-Bytes (Write-Scene (Read-Scene $baselineBytes)))){throw 'Other scene data changed.'}
$receipt=[ordered]@{platform='n64';authoredSceneSha256=$originalHash;stagedSceneSha256=$plannedHash;stagedProjectRoot=$stageRoot;baselinePath=$baselinePath;labelCount=$labels.Count;labels=$labels;originalCommonAndOtherScopesPreserved=$true;selectedForReleaseAndDebug=$true;roundTripPassed=$true;fontSettingsSha256Before=$fontBefore;fontSettingsSha256After=$fontBefore;fontSettingsUnchanged=$true;validateOnly=[bool]$ValidateOnly}
if(!$ValidateOnly){
    if(!(Test-Path -LiteralPath $baselinePath)){[IO.File]::WriteAllBytes($baselinePath,$baselineBytes)}
    if((Hash-Bytes $currentBytes) -ne $plannedHash){[IO.File]::WriteAllBytes($scenePath,$plannedBytes)}
    $fontAfter=(Get-FileHash -LiteralPath $fontPath).Hash.ToLowerInvariant()
    if($fontAfter -ne $fontBefore){throw 'Font settings changed during outline preparation.'}
    if((Get-FileHash -LiteralPath $scenePath).Hash.ToLowerInvariant() -ne $plannedHash){throw 'Staged scene write did not match the validated plan.'}
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($receiptSafe))|Out-Null
    $receipt|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $receiptSafe
}
$receipt|ConvertTo-Json -Depth 8
