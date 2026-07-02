$root = Split-Path -Parent $PSScriptRoot

Write-Host "=== Git ===" -ForegroundColor Cyan
git -C $root status
git -C $root submodule status

Write-Host "`n=== Packages ===" -ForegroundColor Cyan
$pkg = Get-AppxPackage -Name '81ef2129-2f08-4d22-9816-050b2b62b308' -ErrorAction SilentlyContinue
if ($pkg) {
    Write-Host "  Installed: $($pkg.PackageFullName)" -ForegroundColor Green
} else {
    Write-Host "  Not installed." -ForegroundColor DarkGray
}

Write-Host "`n=== Build Outputs ===" -ForegroundColor Cyan
$exe = Join-Path $root 'x64\Debug\dosbox-uwp\dosbox-uwp.exe'
if (Test-Path $exe) {
    $ver = (Get-Item $exe).VersionInfo
    Write-Host "  dosbox-uwp.exe $($ver.FileVersion)" -ForegroundColor Green
} else {
    Write-Host "  No Debug build." -ForegroundColor DarkGray
}

$exe = Join-Path $root 'x64\Release\dosbox-uwp\dosbox-uwp.exe'
if (Test-Path $exe) {
    $ver = (Get-Item $exe).VersionInfo
    Write-Host "  dosbox-uwp.exe (Release) $($ver.FileVersion)" -ForegroundColor Green
} else {
    Write-Host "  No Release build." -ForegroundColor DarkGray
}

Write-Host "`n=== Environment ===" -ForegroundColor Cyan
Write-Host "  Solution: $root\dosbox-pure-unleashed-uwp.sln"
Write-Host "  SDK: 10.0.26100.0"
Write-Host "  Toolset: v143 (VS2022)"
