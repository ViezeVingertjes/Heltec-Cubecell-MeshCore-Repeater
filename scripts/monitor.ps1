#!/usr/bin/env pwsh
Write-Host "Starting serial monitor..." -ForegroundColor Green
python -m platformio device monitor --baud 115200
