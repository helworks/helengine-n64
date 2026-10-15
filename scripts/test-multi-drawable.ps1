param([Parameter(Mandatory=$true)][string]$OutputRoot,[string]$Compiler='g++')
$ErrorActionPreference='Stop'
$Repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
New-Item -ItemType Directory -Force -Path $OutputRoot|Out-Null
$Executable=Join-Path $OutputRoot 'multi-drawable-tests.exe'
$Sources=@('Nintendo64RenderManager3D.cpp','Nintendo64Renderer.cpp','Nintendo64RuntimeMaterial.cpp')|ForEach-Object {Join-Path $Repo ('src/platform/n64/'+$_)}
& $Compiler -std=c++20 -Wall -Wextra -Werror -UNDEBUG -DHELENGINE_N64_HAS_GENERATED_CORE=1 ("-I"+(Join-Path $Repo 'tests/multi_drawable_stubs')) ("-I"+(Join-Path $Repo 'src')) (Join-Path $Repo 'tests/Nintendo64MultiDrawableTests.cpp') @Sources -o $Executable
if($LASTEXITCODE -ne 0){throw "Strict multi-drawable compilation failed: $LASTEXITCODE"}
foreach($Mode in @('','--matrix','--draw')){
 & $Executable $Mode
 if($LASTEXITCODE -ne 0){throw "Multi-drawable test failed in mode '$Mode': $LASTEXITCODE"}
}
