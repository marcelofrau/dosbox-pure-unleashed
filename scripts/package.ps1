param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$CertPassword = $env:DOSBOX_CERT_PASSWORD,
    [switch]$SkipBuild
)

$root = Split-Path -Parent $PSScriptRoot
$pfx = Join-Path $root 'certs\dosbox-uwp.pfx'
$cer = Join-Path $root 'certs\dosbox-uwp.cer'

# ensure certificate
if (-not (Test-Path $pfx)) {
    Write-Host "No cert found. Creating ..." -ForegroundColor Cyan
    if (-not $CertPassword) {
        $CertPassword = 'dev'
        Write-Host "  Using fixed password: $CertPassword" -ForegroundColor Yellow
    }
    $secPass = $CertPassword | ConvertTo-SecureString -AsPlainText -Force
    $cert = New-SelfSignedCertificate `
        -Subject "CN=Marcelo Frau, E=marcelofrau@gmail.com, https://github.com/marcelofrau" `
        -FriendlyName "dosbox-uwp (Marcelo Frau)" `
        -Type CodeSigningCert `
        -KeyUsage DigitalSignature `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3") `
        -CertStoreLocation Cert:\CurrentUser\My
    Export-PfxCertificate -Cert $cert -FilePath $pfx -Password $secPass -Force | Out-Null
    Export-Certificate -Cert $cert -FilePath $cer -Type CERT -Force | Out-Null
    Write-Host "  Cert created: $pfx" -ForegroundColor Green
} elseif (-not $CertPassword) {
    $CertPassword = 'dev'
    Write-Host "  Using fixed password: dev" -ForegroundColor Yellow
}

# pass password to build via env var and param
$env:DOSBOX_CERT_PASSWORD = $CertPassword

# build (MSBuild handles signing automatically via vcxproj settings)
if (-not $SkipBuild) {
    & "$PSScriptRoot\build.ps1" -Configuration $Configuration -Platform $Platform -CertPassword $CertPassword
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# locate msix
$pkgDir = Join-Path $root "AppPackages\dosbox-uwp\dosbox-uwp_1.0.0.0_${Platform}_${Configuration}_Test"
$msix = Join-Path $pkgDir "dosbox-uwp_1.0.0.0_${Platform}_${Configuration}.msix"

if (-not (Test-Path $msix)) {
    # build may have placed it at the x64\Debug\ layout — check there
    $msixAlt = Join-Path $root "x64\$Configuration\dosbox-uwp\dosbox-uwp.msix"
    if (Test-Path $msixAlt) { $msix = $msixAlt }
    else {
        Write-Error "MSIX not found."
        exit 1
    }
}

# verify signature
Write-Host "Verifying signature ..." -ForegroundColor Cyan
$signtool = Resolve-Path "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe" |
    Select-Object -First 1 -ExpandProperty Path
if ($signtool) {
    & $signtool verify /pa /v $msix 2>&1 | Select-String "Successfully verified|Signed"
}

Write-Host "`n=== Generated files ===" -ForegroundColor Green
Write-Host "  MSIX:       $msix" -ForegroundColor Green
Write-Host "  CERT:       $cer" -ForegroundColor Green
Write-Host "  INSTALL:    $pkgDir\Install.ps1" -ForegroundColor Green

$depDir = Join-Path $pkgDir 'Dependencies'
if (Test-Path $depDir) {
    Write-Host "`n=== Dependencies (VC++ UWP runtimes) ===" -ForegroundColor Cyan
    Get-ChildItem $depDir -Recurse -File | ForEach-Object {
        $size = [math]::Round($_.Length / 1KB)
        Write-Host "  $($_.Name)  ($size KB)" -ForegroundColor Gray
    }
}

Write-Host "`n=== How to deploy on Xbox dev mode ===" -ForegroundColor Cyan
Write-Host "  1. Copy this folder to Xbox:" -ForegroundColor Gray
Write-Host "     $pkgDir" -ForegroundColor White
Write-Host "  2. Install CER certificate in Xbox Device Portal > Certificates" -ForegroundColor Gray
Write-Host "     File: $cer" -ForegroundColor White
Write-Host "  3. Run Install.ps1 on Xbox, or deploy via Device Portal > Apps" -ForegroundColor Gray
Write-Host ""
