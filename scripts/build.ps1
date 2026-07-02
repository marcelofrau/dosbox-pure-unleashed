param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$CertPassword = $env:DOSBOX_CERT_PASSWORD
)

$root = Split-Path -Parent $PSScriptRoot
$sln = Join-Path $root 'dosbox-pure-unleashed-uwp.sln'

$msbuild = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $msbuild = Resolve-Path "$env:ProgramFiles\Microsoft Visual Studio\*\*\MSBuild\*\Bin\MSBuild.exe" |
        Select-Object -First 1 -ExpandProperty Path
}

if (-not $msbuild) {
    Write-Error "MSBuild not found. Run from Developer Command Prompt or install VS2022."
    exit 1
}

$msbuildArgs = @(
    $sln
    "/p:Configuration=$Configuration"
    "/p:Platform=$Platform"
    '/m'
)
if ($CertPassword) {
    $msbuildArgs += "/p:PackageCertificatePassword=$CertPassword"
}

Write-Host "Building $Configuration|$Platform ..." -ForegroundColor Cyan
& $msbuild $msbuildArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

Write-Host "Build succeeded." -ForegroundColor Green
