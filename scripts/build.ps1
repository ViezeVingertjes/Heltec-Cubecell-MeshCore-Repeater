param(
    [ValidateSet("release", "debug", "all")]
    [string]$Target = "release"
)

$ErrorActionPreference = "Stop"

$pioPath = "$env:USERPROFILE\.platformio\penv\Scripts"
if (Test-Path $pioPath) { $env:Path += ";$pioPath" }

Push-Location (Split-Path $PSScriptRoot -Parent)

try {
    switch ($Target) {
        "release" {
            Write-Host "Building release..." -ForegroundColor Cyan
            & pio run -e cubecell_board
        }
        "debug" {
            Write-Host "Building debug..." -ForegroundColor Cyan
            & pio run -e cubecell_board_debug
        }
        "all" {
            Write-Host "Building all targets..." -ForegroundColor Cyan
            & pio run -e cubecell_board -e cubecell_board_debug
        }
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host "Build successful!" -ForegroundColor Green
} finally {
    Pop-Location
}
