<#
install_windows.ps1
Copies built VST3 and presets into system locations on Windows.
Usage: run in an elevated PowerShell session or it will attempt to write to user locations.
#>
param(
    [string]$ProjectRoot = "$(Resolve-Path "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)")"
)

$projectRoot = Resolve-Path "$ProjectRoot".Path
$vst3Src = Join-Path $projectRoot "build\crit_artefacts\Release\VST3\crit.vst3"
$presetsSrc = Join-Path $projectRoot "Source\Presets"

$vst3Dst = Join-Path ${Env:ProgramFiles} "Common Files\VST3"
$presetsDst = Join-Path ${Env:ProgramData} "crit\presets"

Write-Host "Installing VST3 -> $vst3Dst"
if (Test-Path $vst3Src) {
    if (-not (Test-Path $vst3Dst)) { New-Item -ItemType Directory -Path $vst3Dst -Force | Out-Null }
    Copy-Item -Path $vst3Src -Destination $vst3Dst -Recurse -Force
} else {
    Write-Warning "VST3 bundle not found at $vst3Src"
}

Write-Host "Installing presets -> $presetsDst"
if (Test-Path $presetsSrc) {
    if (-not (Test-Path $presetsDst)) { New-Item -ItemType Directory -Path $presetsDst -Force | Out-Null }
    Copy-Item -Path (Join-Path $presetsSrc "*") -Destination $presetsDst -Force
} else {
    Write-Warning "Presets folder not found at $presetsSrc"
}

Write-Host "Install script finished."