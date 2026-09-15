[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ArtifactPath,
    [string]$EmulatorPath,
    [switch]$Visible
)

$ErrorActionPreference = 'Stop'
$artifact = (Resolve-Path -LiteralPath $ArtifactPath -ErrorAction Stop).Path
if ((Get-Item -LiteralPath $artifact).Length -le 0) {
    throw "ROM artifact is empty: $artifact"
}

if (-not $EmulatorPath) {
    $EmulatorPath = $env:HELENGINE_N64_ARES_PATH
}
if (-not $EmulatorPath) {
    throw 'Set -EmulatorPath or HELENGINE_N64_ARES_PATH to an ares executable.'
}
$emulator = (Resolve-Path -LiteralPath $EmulatorPath -ErrorAction Stop).Path
if (-not (Test-Path -LiteralPath $emulator -PathType Leaf)) {
    throw "Configured emulator executable does not exist: $emulator"
}

# ares accepts the ROM path as its positional input. Homebrew behavior is
# encoded in the ROM header by n64.mk; no global profile is modified here.
$startArgs = @($artifact)
$windowStyle = if ($Visible) { 'Normal' } else { 'Hidden' }
Start-Process -FilePath $emulator -ArgumentList $startArgs -WindowStyle $windowStyle
