param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Debug',
    [ValidateSet('x64')][string]$Platform = 'x64'
)

$root = Split-Path -Parent $PSScriptRoot

$pkgName = '81ef2129-2f08-4d22-9816-050b2b62b308'

$pkgDir = Join-Path $root "AppPackages\dosbox-uwp\dosbox-uwp_1.0.0.0_${Platform}_${Configuration}_Test"
$msix = Join-Path $pkgDir "dosbox-uwp_1.0.0.0_${Platform}_${Configuration}.msix"

if (-not (Test-Path $msix)) {
    Write-Error "Package not found at $msix. Build first."
    exit 1
}

Write-Host "Uninstalling old package (if any)..." -ForegroundColor Cyan
$pkg = Get-AppxPackage -Name $pkgName -ErrorAction SilentlyContinue
if ($pkg) {
    Remove-AppxPackage -Package $pkg.PackageFullName -ErrorAction Stop
    Write-Host "  Removed: $($pkg.PackageFullName)" -ForegroundColor DarkGray
}

Write-Host "Installing $msix ..." -ForegroundColor Cyan
Add-AppxPackage -Path $msix -ErrorAction Stop
Write-Host "Install done." -ForegroundColor Green
