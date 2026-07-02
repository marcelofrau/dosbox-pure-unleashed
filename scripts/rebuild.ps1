param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$CertPassword = $env:DOSBOX_CERT_PASSWORD
)

& "$PSScriptRoot\clean.ps1" -Configuration $Configuration -Platform $Platform
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& "$PSScriptRoot\build.ps1" -Configuration $Configuration -Platform $Platform -CertPassword $CertPassword
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Rebuild done." -ForegroundColor Green
