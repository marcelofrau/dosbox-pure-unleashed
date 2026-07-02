param(
    [string]$SubjectName = 'Marcelo Frau',
    [string]$Email = 'marcelofrau@gmail.com',
    [string]$CertDir = (Join-Path (Split-Path -Parent $PSScriptRoot) 'certs')
)

if (-not (Test-Path $CertDir)) {
    New-Item -ItemType Directory -Path $CertDir -Force | Out-Null
}

$pfxPath = Join-Path $CertDir 'dosbox-uwp.pfx'
$cerPath = Join-Path $CertDir 'dosbox-uwp.cer'

$existing = Get-ChildItem -Path Cert:\CurrentUser\My -CodeSigningCert |
    Where-Object { $_.Subject -match [regex]::Escape($SubjectName) } |
    Select-Object -First 1

if ($existing) {
    Write-Host "Found existing cert: $($existing.Thumbprint)" -ForegroundColor Cyan
    $cert = $existing
} else {
    Write-Host "Creating new code-signing certificate..." -ForegroundColor Cyan
    $cert = New-SelfSignedCertificate `
        -Subject "CN=$SubjectName, E=$Email, https://github.com/marcelofrau" `
        -FriendlyName "dosbox-uwp ($SubjectName)" `
        -Type CodeSigningCert `
        -KeyUsage DigitalSignature `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3") `
        -CertStoreLocation Cert:\CurrentUser\My

    Write-Host "  Created: $($cert.Thumbprint)" -ForegroundColor Green
}

$plain = 'dev'
$password = $plain | ConvertTo-SecureString -AsPlainText -Force
Write-Host "  Using fixed password: $plain" -ForegroundColor Yellow

Export-PfxCertificate -Cert $cert -FilePath $pfxPath -Password $password -Force
Write-Host "  PFX: $pfxPath" -ForegroundColor Green

Export-Certificate -Cert $cert -FilePath $cerPath -Type CERT -Force
Write-Host "  CER: $cerPath" -ForegroundColor Green

Write-Host "`nCert ready. Install .cer on target device (Xbox dev mode) to trust the package." -ForegroundColor Cyan
