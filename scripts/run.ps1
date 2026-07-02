param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$CertPassword = $env:DOSBOX_CERT_PASSWORD
)

$root = Split-Path -Parent $PSScriptRoot

& "$PSScriptRoot\build.ps1" -Configuration $Configuration -Platform $Platform -CertPassword $CertPassword
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& "$PSScriptRoot\install.ps1" -Configuration $Configuration -Platform $Platform
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$pkgName = '81ef2129-2f08-4d22-9816-050b2b62b308'
$aumid = "${pkgName}!App"

Write-Host "Launching $aumid ..." -ForegroundColor Cyan
Start-Process "shell:AppsFolder\$aumid"
Write-Host "App launched." -ForegroundColor Green
