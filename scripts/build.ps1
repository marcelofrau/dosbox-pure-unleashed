param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [ValidateSet('x64')][string]$Platform = 'x64'
)

$root = Split-Path -Parent $PSScriptRoot
$sln = Join-Path $root 'dosbox-pure-unleashed-uwp.sln'

$msbuild = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not $msbuild) {
    Write-Error "MSBuild not found. Run from Developer Command Prompt or install VS2022."
    exit 1
}

Write-Host "Building $Configuration|$Platform ..." -ForegroundColor Cyan
& $msbuild $sln "/p:Configuration=$Configuration" "/p:Platform=$Platform" '/m'

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded." -ForegroundColor Green
