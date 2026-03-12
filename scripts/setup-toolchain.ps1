$ErrorActionPreference = "Stop"

function Test-Command {
    param([string]$Command)
    $null = Get-Command $Command -ErrorAction SilentlyContinue
    return $?
}

Write-Host "MiniCore Toolchain Setup" -ForegroundColor Cyan
Write-Host "========================" -ForegroundColor Cyan
Write-Host ""

$hasGcc = Test-Command "gcc"
$mingwPath = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"

if (-not $hasGcc -and (Test-Path $mingwPath)) {
    $env:Path += ";$mingwPath"
    $hasGcc = Test-Command "gcc"
}

if ($hasGcc) {
    Write-Host "[OK] GCC is available" -ForegroundColor Green
    & gcc --version | Select-Object -First 1
} else {
    Write-Host "[MISSING] GCC not found" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Install via winget (run as admin):" -ForegroundColor Cyan
    Write-Host "  winget install BrechtSanders.WinLibs.POSIX.UCRT" -ForegroundColor White
    Write-Host ""
}

$hasPio = Test-Command "pio"
$pioPath = "$env:USERPROFILE\.platformio\penv\Scripts"

if (-not $hasPio -and (Test-Path "$pioPath\pio.exe")) {
    $env:Path += ";$pioPath"
    $hasPio = Test-Command "pio"
}

if ($hasPio) {
    Write-Host "[OK] PlatformIO is available" -ForegroundColor Green
} else {
    Write-Host "[MISSING] PlatformIO not found" -ForegroundColor Red
    Write-Host "  Install: pip install platformio" -ForegroundColor White
}

Write-Host ""
Write-Host "Commands:" -ForegroundColor Cyan
Write-Host "  .\scripts\build.ps1           Build release firmware" -ForegroundColor White
Write-Host "  .\scripts\test.ps1            Run native tests" -ForegroundColor White
Write-Host "  .\scripts\test.ps1 -Target embedded  Run on-device tests" -ForegroundColor White
Write-Host "  .\scripts\clean.ps1           Clean build artifacts" -ForegroundColor White
