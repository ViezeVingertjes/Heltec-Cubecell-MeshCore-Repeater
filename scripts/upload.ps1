#!/usr/bin/env pwsh
Write-Host "Building and uploading to device..." -ForegroundColor Green
python -m platformio run -e cubecell_board -t upload
if ($LASTEXITCODE -eq 0) {
    Write-Host "Upload successful!" -ForegroundColor Green
} else {
    Write-Host "Upload failed!" -ForegroundColor Red
    exit 1
}
