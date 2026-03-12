$ErrorActionPreference = "Stop"

$pioPath = "$env:USERPROFILE\.platformio\penv\Scripts"
if (Test-Path $pioPath) { $env:Path += ";$pioPath" }

Push-Location (Split-Path $PSScriptRoot -Parent)

try {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Cyan
    
    $pioDir = Join-Path (Get-Location) ".pio"
    if (Test-Path $pioDir) {
        Remove-Item $pioDir -Recurse -Force
        Write-Host "Removed .pio directory" -ForegroundColor Green
    } else {
        Write-Host "Nothing to clean" -ForegroundColor Yellow
    }
    
    Write-Host "Clean complete!" -ForegroundColor Green
} finally {
    Pop-Location
}
