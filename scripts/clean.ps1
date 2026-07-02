param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64')][string]$Platform = 'x64'
)

$root = Split-Path -Parent $PSScriptRoot
$sln = Join-Path $root 'dosbox-pure-unleashed-uwp.sln'

$msbuild = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $msbuild = Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\*\*\MSBuild\*\Bin\MSBuild.exe" |
        Select-Object -First 1 -ExpandProperty Path
}

if (-not $msbuild) {
    Write-Error "MSBuild not found."
    exit 1
}

Write-Host "Cleaning $Configuration|$Platform ..." -ForegroundColor Cyan
& $msbuild $sln /t:Clean /p:Configuration=$Configuration /p:Platform=$Platform

$appDir = Join-Path $root 'AppPackages'
if (Test-Path $appDir) {
    Remove-Item -Recurse -Force $appDir
    Write-Host "Removed AppPackages/" -ForegroundColor DarkGray
}

$outDir = Join-Path $root 'x64'
if (Test-Path $outDir) {
    Remove-Item -Recurse -Force $outDir
    Write-Host "Removed x64/" -ForegroundColor DarkGray
}

Write-Host "Clean done." -ForegroundColor Green
