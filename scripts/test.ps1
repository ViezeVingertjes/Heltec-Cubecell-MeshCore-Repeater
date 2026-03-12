param(
    [ValidateSet("native", "embedded", "embedded-build", "all")]
    [string]$Target = "native"
)

$ErrorActionPreference = "Stop"

$mingwPath = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
if (Test-Path $mingwPath) { $env:Path += ";$mingwPath" }

Push-Location (Split-Path $PSScriptRoot -Parent)

try {
    switch ($Target) {
        "native" {
            Write-Host "Running native tests..." -ForegroundColor Cyan
            python -m platformio test -e native
        }
        "embedded-build" {
            Write-Host "Building embedded tests (no upload)..." -ForegroundColor Cyan
            python -m platformio test -e cubecell_board_test --without-uploading --without-testing
        }
        "embedded" {
            Write-Host "Building and uploading embedded tests..." -ForegroundColor Cyan
            Write-Host "NOTE: Device must be connected. Press reset when prompted." -ForegroundColor Yellow
            python -m platformio test -e cubecell_board_test
        }
        "all" {
            Write-Host "Running native tests..." -ForegroundColor Cyan
            python -m platformio test -e native
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "Building and uploading embedded tests..." -ForegroundColor Cyan
                Write-Host "NOTE: Device must be connected. Press reset when prompted." -ForegroundColor Yellow
                python -m platformio test -e cubecell_board_test
            }
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Tests failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host "All tests passed!" -ForegroundColor Green
} finally {
    Pop-Location
}
